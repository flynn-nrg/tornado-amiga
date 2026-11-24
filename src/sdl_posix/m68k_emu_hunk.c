/*
Copyright (c) 2026, Miguel Mendez. All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list
of conditions and the following disclaimer.

Redistributions in binary form must reproduce the above copyright notice, this
list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

// Hunk object loader and linker for the emulated 68k.
//
// Loads the objects vasm writes with -Fhunk (one HUNK_UNIT each) into the 68k
// address space, the way vlink would link them into one executable:
//  - CODE, DATA and BSS sections, _C (chip) ones into the chip arena.
//  - RELOC32/RELOC32SHORT (absolute) and RELOC16/RELOC8/RELRELOC32
//    (pc-relative between sections) relocations.
//  - EXT: exported definitions (xdef), absolute symbols, commons, and
//    references to externals (xref), absolute or pc-relative.
//  - SYMBOL hunks: local labels, for the crash reports.
//
// Externals are resolved against every loaded module. An unresolved one points
// at its thunk in the trap page, so using it is a clean UNRESOLVED fault, until
// it is bound to a host function or to data. A pc-relative reference
// (bsr _ext) can't reach the trap page, so every module gets a slot of
// "jmp thunk.l" per pc-relative external, right after its code.
//
// Small data (a4-relative, HUNK_DREL*/EXT_DEXT*) is not supported.

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "m68k_emu_internal.h"
#include "tndo_m68k.h"

#define HUNK_UNIT 0x3E7
#define HUNK_NAME 0x3E8
#define HUNK_CODE 0x3E9
#define HUNK_DATA 0x3EA
#define HUNK_BSS 0x3EB
#define HUNK_RELOC32 0x3EC
#define HUNK_RELOC16 0x3ED
#define HUNK_RELOC8 0x3EE
#define HUNK_EXT 0x3EF
#define HUNK_SYMBOL 0x3F0
#define HUNK_DEBUG 0x3F1
#define HUNK_END 0x3F2
#define HUNK_DREL32 0x3F7
#define HUNK_DREL16 0x3F8
#define HUNK_DREL8 0x3F9
#define HUNK_RELOC32SHORT 0x3FC
#define HUNK_RELRELOC32 0x3FD
#define HUNK_ABSRELOC16 0x3FE

#define EXT_DEF 1
#define EXT_ABS 2
#define EXT_RES 3
#define EXT_REF32 129
#define EXT_COMMON 130
#define EXT_REF16 131
#define EXT_REF8 132
#define EXT_DEXT32 133
#define EXT_DEXT16 134
#define EXT_DEXT8 135
#define EXT_RELREF32 136
#define EXT_RELCOMMON 137
#define EXT_ABSREF16 138
#define EXT_ABSREF8 139

#define MAX_MODULES 256
#define MAX_SECTIONS 64
#define MAX_NAME 256
#define SLOT_SIZE 8 // jmp xxx.l + nop

// Registered modules survive tndo_m68k_shutdown(): they are loaded again by
// the next tndo_m68k_init().
static struct {
  const char *name;
  const uint8_t *data;
  uint32_t len;
} modules[MAX_MODULES];
static int nmodules;

typedef enum { IMP_UNRESOLVED, IMP_DEFINED, IMP_HOST, IMP_DATA } imp_state;

typedef struct {
  char *name;
  imp_state state;
  uint32_t target; // IMP_DEFINED / IMP_DATA
  tndo_m68k_host_fn fn;
  void *user;
  uint32_t common_size;
} import;

typedef enum { FX_ABS32, FX_ABS16, FX_ABS8, FX_PC32, FX_PC16, FX_PC8 } fx_kind;

typedef struct {
  int imp;
  fx_kind kind;
  uint32_t addr;   // 68k address of the field.
  uint32_t slot;   // Jump slot, for pc-relative references.
  int32_t addend;  // Field contents in the object file.
  const char *module;
} fixup;

static struct {
  import *imp;
  int nimp, imp_cap;
  fixup *fx;
  int nfx, fx_cap;
} L;

// ---------------------------------------------------------------------------
// Helpers.
// ---------------------------------------------------------------------------

typedef struct {
  const uint8_t *d;
  uint32_t len;
  uint32_t pos;
  int err;
} reader;

static uint32_t rd32(reader *r) {
  if (r->err || r->len - r->pos < 4 || r->pos > r->len) {
    r->err = 1;
    return 0;
  }
  uint32_t v = tndo_m68k_rd32(r->d + r->pos);
  r->pos += 4;
  return v;
}

static void skip(reader *r, uint32_t bytes) {
  if (r->err || bytes > r->len - r->pos) {
    r->err = 1;
    return;
  }
  r->pos += bytes;
}

// Names are padded to longwords and not always NUL terminated.
static void rd_name(reader *r, uint32_t longs, char *out) {
  uint32_t n = longs * 4;
  if (r->err || n > r->len - r->pos || n >= MAX_NAME) {
    r->err = 1;
    out[0] = 0;
    return;
  }
  memcpy(out, r->d + r->pos, n);
  out[n] = 0;
  r->pos += n;
}

static int load_error(const char *module, const char *fmt, ...)
    __attribute__((format(printf, 2, 3)));
static int load_error(const char *module, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  fprintf(stderr, "68k: %s: ", module);
  vfprintf(stderr, fmt, ap);
  fputc('\n', stderr);
  va_end(ap);
  return -1;
}

static uint32_t thunk_addr(int imp) {
  return M68K_THUNK_BASE + 4 * (uint32_t)imp;
}

static int find_import(const char *name) {
  for (int i = 0; i < L.nimp; i++) {
    if (!strcmp(L.imp[i].name, name))
      return i;
  }
  return -1;
}

static int get_import(const char *name) {
  int i = find_import(name);
  if (i >= 0)
    return i;
  if (L.nimp == M68K_MAX_IMPORTS)
    return -1;
  if (L.nimp == L.imp_cap) {
    int cap = L.imp_cap ? L.imp_cap * 2 : 64;
    import *p = realloc(L.imp, cap * sizeof(*p));
    if (!p)
      return -1;
    L.imp = p;
    L.imp_cap = cap;
  }
  import *im = &L.imp[L.nimp];
  memset(im, 0, sizeof(*im));
  size_t n = strlen(name) + 1;
  im->name = malloc(n); // No strdup(): this file is built as plain C99.
  if (!im->name)
    return -1;
  memcpy(im->name, name, n);
  return L.nimp++;
}

static int add_fixup(const fixup *f) {
  if (L.nfx == L.fx_cap) {
    int cap = L.fx_cap ? L.fx_cap * 2 : 256;
    fixup *p = realloc(L.fx, cap * sizeof(*p));
    if (!p)
      return -1;
    L.fx = p;
    L.fx_cap = cap;
  }
  L.fx[L.nfx++] = *f;
  return 0;
}

static int field_size(fx_kind k) {
  switch (k) {
  case FX_ABS32:
  case FX_PC32:
    return 4;
  case FX_ABS16:
  case FX_PC16:
    return 2;
  default:
    return 1;
  }
}

static int32_t read_field(uint8_t *p, int size) {
  switch (size) {
  case 4:
    return (int32_t)tndo_m68k_rd32(p);
  case 2:
    return (int16_t)tndo_m68k_rd16(p);
  default:
    return (int8_t)p[0];
  }
}

// Writes a value into a 1/2/4 byte field, checking it fits. 8 and 16-bit
// fields are signed: displacements and absolute short addresses are
// sign-extended by the CPU.
static int write_field(uint8_t *p, int size, int64_t v) {
  switch (size) {
  case 4:
    tndo_m68k_wr32(p, (uint32_t)v);
    return 0;
  case 2:
    if (v < -32768 || v > 32767)
      return -1;
    tndo_m68k_wr16(p, (uint16_t)v);
    return 0;
  default:
    if (v < -128 || v > 127)
      return -1;
    p[0] = (uint8_t)v;
    return 0;
  }
}

// ---------------------------------------------------------------------------
// Resolution.
// ---------------------------------------------------------------------------

static int patch(const fixup *f) {
  const import *im = &L.imp[f->imp];
  int size = field_size(f->kind);
  uint8_t *p = tndo_m68k_host(f->addr, size);
  if (!p)
    return load_error(f->module, "fixup for %s outside 68k RAM", im->name);

  int direct = im->state == IMP_DEFINED || im->state == IMP_DATA;
  uint32_t target = direct ? im->target : thunk_addr(f->imp);
  int64_t v;
  switch (f->kind) {
  case FX_ABS32:
  case FX_ABS16:
  case FX_ABS8:
    v = (int64_t)(int32_t)(f->addend + target);
    break;
  default:
    // pc-relative: unresolved and host bound externals go through the slot.
    if (!direct)
      target = f->slot;
    v = (int64_t)f->addend + target - f->addr;
    break;
  }
  if (write_field(p, size, v))
    return load_error(f->module,
                      "%d-bit %s reference to %s at $%08X is out of range",
                      size * 8,
                      f->kind >= FX_PC32 ? "pc-relative" : "absolute",
                      im->name, f->addr);
  return 0;
}

static int resolve_all(void) {
  int err = 0;
  for (int i = 0; i < L.nimp; i++) {
    import *im = &L.imp[i];
    if (im->state != IMP_UNRESOLVED)
      continue;
    int found;
    uint32_t addr = m68k_emu_find_export(im->name, &found);
    if (found) {
      im->state = IMP_DEFINED;
      im->target = addr;
    } else if (im->common_size) {
      // A common nobody defines: allocate it, as the linker would.
      uint8_t *p = tndo_m68k_alloc(im->common_size, TNDO_M68K_FAST);
      if (!p) {
        err = load_error("link", "no memory for common %s", im->name);
        continue;
      }
      im->state = IMP_DEFINED;
      im->target = tndo_m68k_addr(p);
      m68k_emu_add_symbol(im->name, im->target, im->common_size, 1);
    }
  }
  for (int i = 0; i < L.nfx; i++) {
    if (patch(&L.fx[i]))
      err = -1;
  }
  return err;
}

// ---------------------------------------------------------------------------
// Loading.
// ---------------------------------------------------------------------------

typedef struct {
  uint32_t type;
  int chip;
  uint32_t size;     // Bytes.
  uint32_t data_off; // File offset of the contents (CODE/DATA).
  uint32_t addr;
  uint8_t *host;
} section;

typedef struct {
  int imp;
  uint32_t addr;
} slot;

static int skip_relocs(reader *r) {
  for (;;) {
    uint32_t count = rd32(r);
    if (r->err || count == 0)
      return r->err ? -1 : 0;
    rd32(r);
    skip(r, count * 4);
  }
}

static int skip_relocs_short(reader *r) {
  // 16-bit words: count, hunk, offsets... terminated by a zero count, then
  // padded to a longword.
  uint32_t start = r->pos;
  for (;;) {
    if (r->len - r->pos < 2) {
      r->err = 1;
      return -1;
    }
    uint32_t count = tndo_m68k_rd16(r->d + r->pos);
    r->pos += 2;
    if (count == 0)
      break;
    skip(r, 2 + count * 2);
    if (r->err)
      return -1;
  }
  if ((r->pos - start) & 2)
    skip(r, 2);
  return r->err ? -1 : 0;
}

static int is_pcrel_ref(uint32_t t) {
  return t == EXT_REF16 || t == EXT_REF8 || t == EXT_RELREF32;
}

// Pass 1: section layout and the number of jump slots needed.
static int scan(const char *module, reader *r, section *sects, int *nsects,
                int *nslots) {
  char name[MAX_NAME];
  char pcrel_names[MAX_SECTIONS * 4][MAX_NAME];
  int npcrel = 0;
  *nsects = 0;

  while (r->pos < r->len && !r->err) {
    uint32_t raw = rd32(r);
    uint32_t type = raw & 0x3FFFFFFF;
    switch (type) {
    case HUNK_UNIT:
    case HUNK_NAME:
    case HUNK_DEBUG:
      skip(r, rd32(r) * 4);
      break;
    case HUNK_CODE:
    case HUNK_DATA:
    case HUNK_BSS: {
      if (*nsects == MAX_SECTIONS)
        return load_error(module, "too many sections");
      section *s = &sects[(*nsects)++];
      memset(s, 0, sizeof(*s));
      s->type = type;
      uint32_t memf = raw >> 30;
      if (memf == 3) {
        s->chip = (rd32(r) & 2) != 0; // MEMF_CHIP
      } else {
        s->chip = memf == 1;
      }
      s->size = (rd32(r) & 0x3FFFFFFF) * 4;
      if (type != HUNK_BSS) {
        s->data_off = r->pos;
        skip(r, s->size);
      }
      break;
    }
    case HUNK_RELOC32:
    case HUNK_RELOC16:
    case HUNK_RELOC8:
    case HUNK_RELRELOC32:
      skip_relocs(r);
      break;
    case HUNK_RELOC32SHORT:
    case HUNK_ABSRELOC16:
      skip_relocs_short(r);
      break;
    case HUNK_EXT:
      for (;;) {
        uint32_t h = rd32(r);
        if (r->err || h == 0)
          break;
        uint32_t t = h >> 24;
        rd_name(r, h & 0xFFFFFF, name);
        if (t < 128) {
          rd32(r);
          continue;
        }
        if (t == EXT_COMMON || t == EXT_RELCOMMON)
          rd32(r);
        skip(r, rd32(r) * 4);
        if (!is_pcrel_ref(t))
          continue;
        int seen = 0;
        for (int i = 0; i < npcrel && !seen; i++)
          seen = !strcmp(pcrel_names[i], name);
        if (!seen) {
          if (npcrel == (int)(sizeof(pcrel_names) / sizeof(pcrel_names[0])))
            return load_error(module, "too many pc-relative externals");
          strcpy(pcrel_names[npcrel++], name);
        }
      }
      break;
    case HUNK_SYMBOL:
      for (;;) {
        uint32_t n = rd32(r) & 0xFFFFFF;
        if (r->err || n == 0)
          break;
        skip(r, n * 4 + 4);
      }
      break;
    case HUNK_END:
      break;
    case HUNK_DREL32:
    case HUNK_DREL16:
    case HUNK_DREL8:
      return load_error(module, "small data (a4-relative) relocations are "
                                "not supported");
    default:
      return load_error(module, "unsupported hunk type $%X at offset %u",
                        type, r->pos - 4);
    }
  }
  if (r->err)
    return load_error(module, "truncated or corrupt hunk object");
  if (*nsects == 0)
    return load_error(module, "no sections");
  *nslots = npcrel;
  return 0;
}

static int place(const char *module, const uint8_t *data, section *sects,
                 int nsects, int nslots, uint32_t *slot_base) {
  // Code first, then the jump slots right behind it (in range of bsr.w),
  // then data and bss.
  for (int pass = 0; pass < 2; pass++) {
    for (int i = 0; i < nsects; i++) {
      section *s = &sects[i];
      if ((s->type == HUNK_CODE) != (pass == 0))
        continue;
      s->host = tndo_m68k_alloc(s->size ? s->size : 4,
                                s->chip ? TNDO_M68K_CHIP : TNDO_M68K_FAST);
      if (!s->host)
        return load_error(module, "out of %s memory (%u bytes)",
                          s->chip ? "chip" : "fast", s->size);
      s->addr = tndo_m68k_addr(s->host);
      if (s->type == HUNK_BSS) {
        memset(s->host, 0, s->size);
      } else {
        memcpy(s->host, data + s->data_off, s->size);
      }
      if (s->type == HUNK_CODE && s->size)
        tndo_m68k_add_code_range(s->addr, s->size);
    }
    if (pass == 0 && nslots) {
      uint8_t *p = tndo_m68k_alloc(nslots * SLOT_SIZE, TNDO_M68K_FAST);
      if (!p)
        return load_error(module, "out of memory for jump slots");
      *slot_base = tndo_m68k_addr(p);
      tndo_m68k_add_code_range(*slot_base, nslots * SLOT_SIZE);
    }
  }
  return 0;
}

static uint32_t get_slot(slot *slots, int *nused, int nslots,
                         uint32_t slot_base, int imp) {
  for (int i = 0; i < *nused; i++) {
    if (slots[i].imp == imp)
      return slots[i].addr;
  }
  if (*nused == nslots)
    return 0;
  uint32_t addr = slot_base + SLOT_SIZE * (*nused);
  uint8_t *p = tndo_m68k_host(addr, SLOT_SIZE);
  tndo_m68k_wr16(p, 0x4EF9); // jmp xxx.l
  tndo_m68k_wr32(p + 2, thunk_addr(imp));
  tndo_m68k_wr16(p + 6, 0x4E71); // nop
  slots[*nused].imp = imp;
  slots[*nused].addr = addr;
  (*nused)++;
  return addr;
}

static fx_kind ext_kind(uint32_t t) {
  switch (t) {
  case EXT_REF16:
    return FX_PC16;
  case EXT_REF8:
    return FX_PC8;
  case EXT_RELREF32:
    return FX_PC32;
  case EXT_ABSREF16:
    return FX_ABS16;
  case EXT_ABSREF8:
    return FX_ABS8;
  default:
    return FX_ABS32;
  }
}

// Pass 2: relocations, symbols and externals.
static int link_unit(const char *module, reader *r, section *sects,
                     int nsects, int nslots, uint32_t slot_base) {
  char name[MAX_NAME];
  slot slots[MAX_SECTIONS * 4];
  int nused = 0;
  int cur = -1;

  while (r->pos < r->len && !r->err) {
    uint32_t type = rd32(r) & 0x3FFFFFFF;
    section *s = cur >= 0 ? &sects[cur] : NULL;
    switch (type) {
    case HUNK_UNIT:
    case HUNK_NAME:
    case HUNK_DEBUG:
      skip(r, rd32(r) * 4);
      break;
    case HUNK_CODE:
    case HUNK_DATA:
    case HUNK_BSS:
      cur++;
      s = &sects[cur];
      if ((tndo_m68k_rd32(r->d + r->pos - 4) >> 30) == 3)
        rd32(r);
      rd32(r);
      if (type != HUNK_BSS)
        skip(r, s->size);
      break;
    case HUNK_RELOC32:
    case HUNK_RELOC16:
    case HUNK_RELOC8:
    case HUNK_RELRELOC32:
    case HUNK_RELOC32SHORT:
    case HUNK_ABSRELOC16: {
      int shortfmt = type == HUNK_RELOC32SHORT || type == HUNK_ABSRELOC16;
      int size = type == HUNK_RELOC16 || type == HUNK_ABSRELOC16 ? 2
                 : type == HUNK_RELOC8                         ? 1
                                                               : 4;
      int pcrel = type == HUNK_RELOC16 || type == HUNK_RELOC8 ||
                  type == HUNK_RELRELOC32;
      uint32_t start = r->pos;
      for (;;) {
        uint32_t count, target;
        if (shortfmt) {
          if (r->len - r->pos < 4)
            return load_error(module, "truncated relocation");
          count = tndo_m68k_rd16(r->d + r->pos);
          r->pos += 2;
          if (count == 0)
            break;
          target = tndo_m68k_rd16(r->d + r->pos);
          r->pos += 2;
        } else {
          count = rd32(r);
          if (r->err || count == 0)
            break;
          target = rd32(r);
        }
        if (!s || target >= (uint32_t)nsects)
          return load_error(module, "relocation to bad section %u", target);
        for (uint32_t i = 0; i < count; i++) {
          uint32_t off;
          if (shortfmt) {
            if (r->len - r->pos < 2)
              return load_error(module, "truncated relocation");
            off = tndo_m68k_rd16(r->d + r->pos);
            r->pos += 2;
          } else {
            off = rd32(r);
          }
          if (r->err || off > s->size || s->size - off < (uint32_t)size)
            return load_error(module, "relocation outside its section");
          uint8_t *p = s->host + off;
          int64_t v = read_field(p, size);
          v += sects[target].addr;
          if (pcrel)
            v -= s->addr + off;
          if (write_field(p, size, size == 4 ? (int64_t)(int32_t)v : v))
            return load_error(module,
                              "%d-bit relocation at $%08X is out of range",
                              size * 8, s->addr + off);
        }
      }
      if (shortfmt && ((r->pos - start) & 2))
        r->pos += 2;
      break;
    }
    case HUNK_EXT:
      for (;;) {
        uint32_t h = rd32(r);
        if (r->err || h == 0)
          break;
        uint32_t t = h >> 24;
        rd_name(r, h & 0xFFFFFF, name);
        if (!s)
          return load_error(module, "EXT outside a section");
        if (t == EXT_DEF || t == EXT_RES || t == EXT_ABS) {
          uint32_t value = rd32(r);
          int found;
          uint32_t prev = m68k_emu_find_export(name, &found);
          if (found)
            return load_error(module,
                              "duplicate symbol %s (already defined at "
                              "$%08X)",
                              name, prev);
          if (t == EXT_ABS) {
            m68k_emu_add_symbol(name, value, 1, 1);
          } else {
            // A label at the very end of a section (dcb.b 0) still only
            // names its own address: size 0 would mean "unbounded".
            m68k_emu_add_symbol(name, s->addr + value,
                                value < s->size ? s->size - value : 1, 1);
          }
          continue;
        }
        if (t == EXT_DEXT32 || t == EXT_DEXT16 || t == EXT_DEXT8 ||
            t == EXT_RELCOMMON)
          return load_error(module,
                            "%s: small data (a4-relative) references are not "
                            "supported",
                            name);
        if (t != EXT_REF32 && t != EXT_COMMON && t != EXT_REF16 &&
            t != EXT_REF8 && t != EXT_RELREF32 && t != EXT_ABSREF16 &&
            t != EXT_ABSREF8)
          return load_error(module, "%s: unsupported EXT type %u", name, t);

        int imp = get_import(name);
        if (imp < 0)
          return load_error(module, "too many externals");
        if (t == EXT_COMMON) {
          uint32_t csize = rd32(r);
          if (csize > L.imp[imp].common_size)
            L.imp[imp].common_size = csize;
        }
        fixup f;
        memset(&f, 0, sizeof(f));
        f.imp = imp;
        f.kind = ext_kind(t);
        f.module = module;
        if (f.kind >= FX_PC32)
          f.slot = get_slot(slots, &nused, nslots, slot_base, imp);
        uint32_t count = rd32(r);
        int size = field_size(f.kind);
        for (uint32_t i = 0; i < count && !r->err; i++) {
          uint32_t off = rd32(r);
          if (off > s->size || s->size - off < (uint32_t)size)
            return load_error(module, "reference to %s outside its section",
                              name);
          f.addr = s->addr + off;
          f.addend = read_field(s->host + off, size);
          if (add_fixup(&f))
            return load_error(module, "out of memory");
        }
      }
      break;
    case HUNK_SYMBOL:
      for (;;) {
        uint32_t n = rd32(r) & 0xFFFFFF;
        if (r->err || n == 0)
          break;
        rd_name(r, n, name);
        uint32_t value = rd32(r);
        if (s && value <= s->size)
          m68k_emu_add_symbol(name, s->addr + value,
                              value < s->size ? s->size - value : 1, 0);
      }
      break;
    case HUNK_END:
      break;
    }
  }
  if (r->err)
    return load_error(module, "truncated or corrupt hunk object");
  return 0;
}

static int load_unit(const char *module, const uint8_t *data, uint32_t len) {
  section sects[MAX_SECTIONS];
  int nsects = 0, nslots = 0;
  uint32_t slot_base = 0;

  if (len < 4 || tndo_m68k_rd32(data) != HUNK_UNIT)
    return load_error(module, "not a hunk object file (no HUNK_UNIT)");

  reader r = {data, len, 0, 0};
  if (scan(module, &r, sects, &nsects, &nslots))
    return -1;
  if (place(module, data, sects, nsects, nslots, &slot_base))
    return -1;
  reader r2 = {data, len, 0, 0};
  return link_unit(module, &r2, sects, nsects, nslots, slot_base);
}

// ---------------------------------------------------------------------------
// Public API.
// ---------------------------------------------------------------------------

void tndo_m68k_register_module(const char *name, const void *data,
                               uint32_t len) {
  if (nmodules == MAX_MODULES) {
    fprintf(stderr, "68k: too many modules, %s ignored\n", name);
    return;
  }
  modules[nmodules].name = name;
  modules[nmodules].data = data;
  modules[nmodules].len = len;
  nmodules++;
}

int m68k_hunk_load_registered(void) {
  int err = 0;
  for (int i = 0; i < nmodules; i++) {
    if (load_unit(modules[i].name, modules[i].data, modules[i].len))
      err = -1;
  }
  if (resolve_all())
    err = -1;
  return err;
}

int tndo_m68k_load_hunk(const char *name, const void *data, uint32_t len) {
  m68k_emu_ensure_init();
  if (load_unit(name, data, len))
    return -1;
  return resolve_all();
}

void m68k_hunk_reset(void) {
  for (int i = 0; i < L.nimp; i++)
    free(L.imp[i].name);
  free(L.imp);
  free(L.fx);
  memset(&L, 0, sizeof(L));
}

const char *m68k_hunk_import(int idx, tndo_m68k_host_fn *fn, void **user) {
  if (idx < 0 || idx >= L.nimp)
    return NULL;
  const import *im = &L.imp[idx];
  if (fn)
    *fn = im->state == IMP_HOST ? im->fn : NULL;
  if (user)
    *user = im->user;
  return im->name;
}

static int bind(const char *name, imp_state state, tndo_m68k_host_fn fn,
                void *user, uint32_t addr) {
  m68k_emu_ensure_init();
  int found;
  m68k_emu_find_export(name, &found);
  if (found)
    return load_error("bind", "%s is defined by a 68k module", name);
  // Binding a name no module references yet is fine: a module loaded later
  // with tndo_m68k_load_hunk() picks it up.
  int imp = get_import(name);
  if (imp < 0)
    return load_error("bind", "too many externals");
  import *im = &L.imp[imp];
  im->state = state;
  im->fn = fn;
  im->user = user;
  im->target = addr;
  int err = 0;
  for (int i = 0; i < L.nfx; i++) {
    if (L.fx[i].imp == imp && patch(&L.fx[i]))
      err = -1;
  }
  return err;
}

int tndo_m68k_bind(const char *name, tndo_m68k_host_fn fn, void *user) {
  if (!fn)
    return load_error("bind", "%s: NULL host function", name);
  return bind(name, IMP_HOST, fn, user, 0);
}

int tndo_m68k_bind_data(const char *name, uint32_t addr) {
  return bind(name, IMP_DATA, NULL, NULL, addr);
}

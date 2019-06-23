/*
Copyright (c) 2019 Luis Pons
This software is provided 'as-is', without any express or implied warranty. In
no event will the authors be held liable for any damages arising from the use of
this software.
Permission is granted to anyone to use this software for any purpose, including
commercial applications, and to alter it and redistribute it freely, subject to
the following restrictions:
    1. The origin of this software must not be misrepresented; you must not
claim that you wrote the original software. If you use this software in a
product, an acknowledgment in the product documentation would be appreciated but
is not required.
    2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"

#define BMP_HEAD_LEN (14 + 40 + 17 * 4)

int get_unsigned_bitlen(unsigned int v) {
  unsigned int bits = 0;
  unsigned int one = 1;
  while ((one << bits) <= v)
    bits++;

  return bits;
}

typedef struct {
  unsigned char type[2]; /* Magic identifier            */
  unsigned int size;     /* File size in bytes          */
  unsigned short int reserved1, reserved2;
  unsigned int offset; /* Offset to image data, bytes */
} header_t;

typedef struct {
  unsigned int size;             /* Header size in bytes      */
  int width, height;             /* Width and height of image */
  unsigned short int planes;     /* Number of colour planes   */
  unsigned short int bits;       /* Bits per pixel            */
  unsigned int compression;      /* Compression type          */
  unsigned int imagesize;        /* Image size in bytes       */
  int xresolution, yresolution;  /* Pixels per meter          */
  unsigned int ncolours;         /* Number of colours         */
  unsigned int importantcolours; /* Important colours         */
} infoheader_t;

// 14 + 40 + 4 * infoheader.ncolours = header.offset

#define MXLEN (1024 * 1024 * 20)

static unsigned char data[MXLEN];

static unsigned int _deb_pal[256];
static unsigned char _deb_pic[MXLEN];
static unsigned char _normal_order[MXLEN];

#define MODE_BPL (100)
#define MODE_SPR4 (101)
#define MODE_SPR16 (102)

#define OUT_TEXT (20)
#define OUT_BIN (21)

static void write_byte(FILE *fo, unsigned int byte, int return_carry,
                       int out_mode) {
  static int cnt = 0;
  if (out_mode == OUT_BIN) {
    unsigned char data = byte;
    fwrite(&data, 1, 1, fo);
  } else {
    fprintf(fo, "0x%x", byte);
    if (cnt == return_carry - 1) {
      cnt = -1;
      fprintf(fo, ",\n");
    } else
      fprintf(fo, ", ");

    cnt++;
  }
}

static void accu_write32(FILE *fo, unsigned int byte, int return_carry,
                         int out_mode) {
  static int byte_cnt = 0;
  static unsigned int word = 0;

  word = (word << 8) | byte;
  byte_cnt++;
  if (byte_cnt == 4) {
    byte_cnt = 0;
    static int text_cnt = 0;
    if (out_mode == OUT_BIN) {
      unsigned char data = byte;
      fputc(word >> 24, fo);
      fputc(word >> 16, fo);
      fputc(word >> 8, fo);
      fputc(word, fo);
    } else {
      fprintf(fo, "0x%x", word);
      if (text_cnt == return_carry - 1) {
        text_cnt = -1;
        fprintf(fo, ",\n");
      } else
        fprintf(fo, ", ");

      text_cnt++;
    }
  }
}

static void write_head(FILE *fo, char convert_mode, int w, int h) {
  // sizeof(head) == 8
  fwrite(&convert_mode, 1, 1, fo); // HEAD type
  fwrite(&convert_mode, 1, 1, fo); // HEAD ""
  unsigned char t = w >> 8;
  fwrite(&t, 1, 1, fo); // HEAD w
  t = w;
  fwrite(&t, 1, 1, fo); // HEAD w
  t = h >> 8;
  fwrite(&t, 1, 1, fo); // HEAD h
  t = h;
  fwrite(&t, 1, 1, fo); // HEAD h
  t = 0;
  fwrite(&t, 1, 1, fo); // HEAD fill
  fwrite(&t, 1, 1, fo); // HEAD fill
}

static void write_head_text(FILE *fo, char convert_mode, int w, int h,
                            int datasize) {

  fprintf(fo, "\nstatic const struct \n{\n");
  fprintf(fo, "    unsigned short head;\n");
  fprintf(fo, "    unsigned short w, h;\n");
  fprintf(fo, "    unsigned short PAD;\n");
  fprintf(fo, "    unsigned int   palette[256];\n");
  fprintf(fo, "    unsigned int  bpls_data[%d];\n", datasize / 4);
  fprintf(fo, "} image = {\n");
  fprintf(fo, "    %d, // HEAD type\n", convert_mode);
  fprintf(fo, "    %d,%d, // width, height\n", w, h);
  fprintf(fo, "    0, // PAD\n");
}

int main(int argc, char **argv) {
  int out_mode = OUT_BIN;

  char *fname = "../../brutalism/srcdata/scrolls/scroll_mask.bmp";
  if (argc != 3) {
    fprintf(stderr, "Usage: bmp2bpl [bpl/spr] pic.bmp > bpls.raw\n");
    exit(1);
  }
  fname = argv[2];

  int convert_mode = 0;
  if (strcmp(argv[1], "bpl") == 0)
    convert_mode = MODE_BPL;
  else if (strcmp(argv[1], "spr") == 0)
    convert_mode = MODE_SPR4;
  else {
    fprintf(stderr, "Invalid conversion mode: [bpl/spr]; requested %s\n",
            argv[1]);
    exit(1);
  }

  FILE *f = fopen(fname, "rb");
  assert(f);

  int c, len = 0;
  while ((c = fgetc(f)) != EOF) {
    data[len] = c;
    len++;
  }
  assert(len < (MXLEN));

  header_t *head = (header_t *)data;
  assert(head->type[0] == 'B' && head->type[1] == 'M');

  infoheader_t *info = (infoheader_t *)&data[14]; // UNALIGNED!

  assert(info->planes == 1);
  assert(info->compression == 0);
  assert(info->ncolours <= 256);
  assert((info->width & 0x7) == 0);

  fprintf(stderr, "Number of colors: %d\n", info->ncolours);

  unsigned int *palette =
      (unsigned int *)&data[BMP_HEAD_LEN]; // still unaligned

  if (convert_mode == MODE_SPR4)
    if (info->ncolours > 4)
      convert_mode = MODE_SPR16;

  int datasize = get_unsigned_bitlen(info->ncolours - 1) * (info->width >> 3) *
                 info->height;
  if ((out_mode == OUT_TEXT) && (convert_mode != MODE_BPL)) {
    assert(0 && "sprites output as source code not ready");
  }

  // HEADER

  if (out_mode == OUT_TEXT)
    write_head_text(stdout, convert_mode, info->width, info->height, datasize);
  else
    write_head(stdout, convert_mode, info->width, info->height);

  int i;
  unsigned char r = 0, g = 0, b = 0;
  int effective_colors = 256;
  if ((convert_mode == MODE_SPR4) || (convert_mode == MODE_SPR16))
    effective_colors = 16;

  if (((convert_mode == MODE_SPR4) || (convert_mode == MODE_SPR16)) &&
      (info->ncolours > 16)) {
    fprintf(stderr, "WARNING: you requested sprites, please notive this bitmap "
                    "has palette larger than 16\n");
  }

  // Dump palette

  if (out_mode == OUT_TEXT)
    fprintf(stdout, "\n{\n");
  for (i = 0; i < effective_colors; i++) {
    unsigned int c = (i < info->ncolours) ? palette[i] : 0;
    _deb_pal[i] = c;
    r = (c >> 16) & 0xff;
    g = (c >> 8) & 0xff;
    b = c & 0xff;

    if (out_mode == OUT_TEXT)
      fprintf(stdout, "    0x%x, //%d\n", (r << 16) | (g << 8) | b, i);
    else {
      fwrite(&r, 1, 1, stdout);
      fwrite(&g, 1, 1, stdout);
      fwrite(&b, 1, 1, stdout);
    }
  }

  if (out_mode == OUT_TEXT)
    fprintf(stdout, "\n},{\n");

  // Collect all pixels in universal form

  int x, y;
  if (info->bits == 8) // 256 colors
  {
    unsigned char *pixels = &data[BMP_HEAD_LEN + info->ncolours * 4];
    int rd = 0;
    i = 0;
    for (y = info->height - 1; y >= 0; y--) {
      for (x = 0; x < info->width; x++) {
        int pix = pixels[rd];
        rd++;
        unsigned char r = (_deb_pal[pix] >> 16) & 0xff;
        unsigned char g = (_deb_pal[pix] >> 8) & 0xff;
        unsigned char b = _deb_pal[pix] & 0xff;

        _deb_pic[3 * (x + y * info->width)] = r;
        _deb_pic[(3 * (x + y * info->width)) + 1] = g;
        _deb_pic[(3 * (x + y * info->width)) + 2] = b;

        _normal_order[x + y * info->width] = pix;
      }
    }
  } else if (info->bits == 4) // 16 colors
  {
    assert((info->width & 1) == 0);
    unsigned char *pixels = &data[BMP_HEAD_LEN + info->ncolours * 4];
    int rd = 0;
    i = 0;
    for (y = info->height - 1; y >= 0; y--) {
      for (x = 0; x < info->width; x += 2) {
        int pix = pixels[rd] >> 4;
        unsigned char r = (_deb_pal[pix] >> 16) & 0xff;
        unsigned char g = (_deb_pal[pix] >> 8) & 0xff;
        unsigned char b = _deb_pal[pix] & 0xff;
        _deb_pic[3 * (x + y * info->width)] = r;
        _deb_pic[(3 * (x + y * info->width)) + 1] = g;
        _deb_pic[(3 * (x + y * info->width)) + 2] = b;
        _normal_order[x + y * info->width] = pix;

        pix = pixels[rd] & 0xf;
        r = (_deb_pal[pix] >> 16) & 0xff;
        g = (_deb_pal[pix] >> 8) & 0xff;
        b = _deb_pal[pix] & 0xff;
        _deb_pic[3 * (x + 1 + y * info->width)] = r;
        _deb_pic[(3 * (x + 1 + y * info->width)) + 1] = g;
        _deb_pic[(3 * (x + 1 + y * info->width)) + 2] = b;
        _normal_order[x + 1 + y * info->width] = pix;

        rd++;
      }
    }
  } else {
    // MONOCOLOR
    assert(info->bits == 1);
    assert((info->width & 7) == 0);
    unsigned char *pixels = &data[BMP_HEAD_LEN + info->ncolours * 4];
    int rd = 0;
    i = 0;
    for (y = info->height - 1; y >= 0; y--) {
      for (x = 0; x < info->width; x += 8) {
        int bit;
        unsigned char byte = pixels[rd];
        rd++;
        for (bit = 0; bit < 8; bit++) {
          unsigned char pix = ((byte << bit) & 0x80) >> 7;
          unsigned char r = (_deb_pal[pix] >> 16) & 0xff;
          unsigned char g = (_deb_pal[pix] >> 8) & 0xff;
          unsigned char b = _deb_pal[pix] & 0xff;
          _deb_pic[3 * ((x + bit) + y * info->width)] = r;
          _deb_pic[(3 * ((x + bit) + y * info->width)) + 1] = g;
          _deb_pic[(3 * ((x + bit) + y * info->width)) + 2] = b;
          _normal_order[(x + bit) + y * info->width] = pix;
        }
      }
    }
  }

  // Proof pixels were correctly collected
  stbi_write_png("proof_promote.png", info->width, info->height, 3, _deb_pic,
                 info->width * 3);

  // Conversion + output

  if (convert_mode == MODE_BPL) {
    int bpl;
    int nbpl = get_unsigned_bitlen(info->ncolours - 1);
    fprintf(stderr, "Writting %d bitplanes\n", nbpl);
    for (bpl = 0; bpl < nbpl; bpl++) {
      for (y = 0; y < info->height; y++) {
        unsigned char byte = 0;
        for (x = 0; x < info->width; x++) {
          int pix = _normal_order[x + y * info->width];

          int bit = (pix >> bpl) & 1;
          byte <<= 1;
          byte |= bit;
          if ((x & 7) == 7) {
            accu_write32(stdout, byte, 8, out_mode);
            byte = 0;
          }

          int r = (_deb_pal[pix] >> 16) & 0xff;
          int g = (_deb_pal[pix] >> 8) & 0xff;
          int b = _deb_pal[pix] & 0xff;

          _deb_pic[3 * (x + y * info->width)] = r;
          _deb_pic[(3 * (x + y * info->width)) + 1] = g;
          _deb_pic[(3 * (x + y * info->width)) + 2] = b;
        }
      }
    }

    if (out_mode == OUT_TEXT)
      fprintf(stdout, "\n}\n};\n");

  } else if (convert_mode == MODE_SPR4) {
    // SPRITES 4
    assert((info->width & 63) == 0);
    assert(effective_colors <= 16);
    fprintf(stderr, "Writting %d sprites of 2 bitplanes\n", info->width / 64);
    int columns = info->width / 64;
    int dst_w = 64, dst_h = (info->height + 2) * columns;
    int bpl;
    unsigned char byte = 0;
    int col = 0;
    for (x = 0; x < info->width; x += 64) {
      for (i = 0; i < 8 * 2; i++) // 64 pixels wide, 2 bitplanes, control line
        write_byte(stdout, byte, 8, out_mode);

      for (y = 0; y < info->height; y++) {
        int spy = y + col * (info->height + 2);
        for (bpl = 0; bpl < 2; bpl++) {
          int sx;
          byte = 0;
          for (sx = 0; sx < 64; sx++) {
            int pix = _normal_order[(x + sx) + y * info->width];

            int bit = (pix >> bpl) & 1;
            byte <<= 1;
            byte |= bit;
            if ((sx & 7) == 7) {
              accu_write32(stdout, byte, 2, out_mode);
              byte = 0;
            }

            int r = (_deb_pal[pix] >> 16) & 0xff;
            int g = (_deb_pal[pix] >> 8) & 0xff;
            int b = _deb_pal[pix] & 0xff;

            _deb_pic[3 * (sx + spy * dst_w)] = r;
            _deb_pic[(3 * (sx + spy * dst_w)) + 1] = g;
            _deb_pic[(3 * (sx + spy * dst_w)) + 2] = b;
          }
        }
      }
      byte = 0;
      for (i = 0; i < 8 * 2; i++) // 64 pixels wide, 2 bitplanes, end line
        accu_write32(stdout, byte, 2, out_mode);
      col++;
    }

    if (out_mode == OUT_TEXT)
      fprintf(stdout, "\n}\n};\n\n");

    fprintf(stderr, "raw size = %d\n",
            (16 * 3) + ((info->width * info->height) / 4) +
                (columns * 8 * 2 * 2));
  } else if (convert_mode == MODE_SPR16) {
    // SPRITES 16
    int deb_w = info->width * 2;
    int deb_h = info->height;

    assert((info->width & 63) == 0);
    assert(effective_colors <= 16);
    fprintf(stderr, "Writting %d sprites of 4 bitplanes, using pairs\n",
            info->width / 64);
    int columns = info->width / 64;
    int dst_w = 64, dst_h = (info->height + 2) * columns;
    int bpl;
    unsigned char byte = 0;
    int col = 0;
    for (x = 0; x < info->width; x += 64) {
      int spr = 0;
      for (spr = 0; spr < 2; spr++) {
        for (i = 0; i < 8 * 2; i++) // 64 pixels wide, 2 bitplanes, control line
          write_byte(stdout, byte, 8, out_mode);

        for (y = 0; y < info->height; y++) {
          int spy = y + col * (info->height + 2);
          for (bpl = 0; bpl < 2; bpl++) {
            int sx;
            byte = 0;
            for (sx = 0; sx < 64; sx++) {
              int pix = _normal_order[(x + sx) + y * info->width];
              if (spr > 0)
                pix >>= 2; // 16 colors sprite are splitted in 2 sprites
              pix &= 0x3;

              int bit = (pix >> bpl) & 1;
              byte <<= 1;
              byte |= bit;
              if ((sx & 7) == 7) {
                accu_write32(stdout, byte, 2, out_mode);
                byte = 0;
              }

              int doffs = (((x / 64) * 128) + (spr * 64) + sx) + (y * deb_w);
              _deb_pic[3 * doffs] = pix << 6;
              _deb_pic[3 * doffs + 1] = pix << 6;
              _deb_pic[3 * doffs + 2] = rand(); // pix << 6;
            }
          } // for (bpl)
        }   // for (y
        byte = 0;
        for (i = 0; i < 8 * 2; i++) // 64 pixels wide, 2 bitplanes, end line
          accu_write32(stdout, byte, 2, out_mode);
      } // for (spr
      col++;
    }
  }

  fflush(stdout);
  return 0;
}

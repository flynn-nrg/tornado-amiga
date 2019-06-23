#ifndef INCLUDE_COPPER_H
#define INCLUDE_COPPER_H

// Insert copper command
#define CI(LIST, COM, ARG)                                                     \
  if ((LIST)->curr < (LIST)->len)                                              \
  (LIST)->commands[(LIST)->curr] = ((COM << 16) | ARG), (LIST)->curr++

#define CW(LIST, ARGX, ARGY)                                                   \
  if ((LIST)->curr < (LIST)->len)                                              \
  (LIST)->commands[(LIST)->curr] =                                             \
      ((0x1 | ((ARGY & 0xff) << 8) | (ARGX & 0xfe)) << 16) | 0xfffe,           \
  (LIST)->curr++

typedef struct {
  unsigned int *commands;
  int curr, len;
} copper_list_t;

typedef struct {
  copper_list_t *cop;
  unsigned short switch_idx;
  unsigned short exit_idx;
  unsigned short blocks_indices[16];
  unsigned short exits_indices[16];
  int curr_block;
} copper_switch_t;

typedef struct {
  int w;
  int h;
  int depth;
  int screenMode;
  int options;
  int numSprites;
  int spriteSize;
  int copperSize;
  int bplcon3;
  unsigned char *chunky;
  unsigned char *planar1;
  unsigned char *planar2;
  unsigned char *fastPlanar1;
  unsigned char *fastPlanar2;
  unsigned char *gbuffer1;
  unsigned char *gbuffer2;
  unsigned char *sprites[8];
  copper_list_t *copper;
  copper_list_t *pal_ref;
  copper_list_t *spr_ref;
  copper_list_t *bpl1_ref;
  copper_list_t *bpl2_ref;
  copper_switch_t *switch_bpls;
  unsigned int *pal256;
} graphics_t;

void copperAlloc(copper_list_t *, int);
void copperInsertPalette(copper_list_t *, unsigned int *, int, int);
void doCopper(graphics_t *, unsigned int);
void setCopper(copper_list_t *);
void insertZeroedSprites(copper_list_t *cop);
void insertBPLPTR(copper_list_t *cop, void *screen_bpls, int depth, int w,
                  int h, int first_plane);
void copper_log(copper_list_t *cop);

// Copper reserve align to 16KB block, to allow atomic copper jumps in
// "copper_switch" funcs
void copper_reserve(copper_list_t *list, int maxlen);
void copper_switch_start(copper_switch_t *t, copper_list_t *cop);
void copper_switch_open_block(copper_switch_t *t);
void copper_switch_end_block(copper_switch_t *t);
void copper_switch_end(copper_switch_t *t);
void copper_switch_choose(copper_switch_t *t, int option);
void copper_modify_jump(copper_list_t *cop, copper_list_t *jmp);

#endif

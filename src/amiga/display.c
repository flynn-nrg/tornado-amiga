/*
Copyright (c) 2019 Miguel Mendez
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

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dos/dos.h>
#include <exec/exec.h>

#include <graphics/modeid.h>
#include <graphics/videocontrol.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>

#include <clib/alib_protos.h>
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>

#include <aga.h>
#include <assets.h>
#include <c2p.h>
#include <canvas.h>
#include <copper.h>
#include <cpu.h>
#include <custom_regs.h>
#include <debug.h>
#include <display.h>
#include <graphics.h>
#include <hardware_check.h>
#include <memory.h>
#include <ptr_bridges.h>
#include <system.h>
#include <tndo_assert.h>
#include <tornado_settings.h>

static int c2pInitDone[64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
static display_instance *instances;
static int maxDisplayInstances;
static sprite_options empty_so = {.num_sprites = 0};
static int lastInstance = 0;

static void clean_bpls(int instance) {
  memset(instances[instance].graph->planar1, 0,
         instances[instance].graph->w * instances[instance].graph->h);
  memset(instances[instance].graph->planar2, 0,
         instances[instance].graph->w * instances[instance].graph->h);
}

static void merge_pal(unsigned int *dst, uint8_t *src, int pos, int count) {
  for (int i = 0; i < count; i++) {
    int r = src[i * 3 + 0];
    int g = src[i * 3 + 1];
    int b = src[i * 3 + 2];
    dst[pos + i] = b | (g << 8) | (r << 16);
  }
}

static void fade_pal_black(unsigned int *dst, unsigned int *src_, float k,
                           int count) {
  uint8_t *src = (uint8_t *)src_;
  for (int i = 0; i < count; i++) {
    int r = src[1] * k;
    int g = src[2] * k;
    int b = src[3] * k;
    src += 4;
    *dst++ = b | (g << 8) | (r << 16);
  }
}

static void fade_pal_white(unsigned int *dst, unsigned int *src_, float k,
                           int count) {
  uint8_t *src = (uint8_t *)src_;
  k = 1.0f - k;
  for (int i = 0; i < count; i++) {
    int r = 255 - ((255 - src[1]) * k);
    int g = 255 - ((255 - src[2]) * k);
    int b = 255 - ((255 - src[3]) * k);
    src += 4;
    *dst++ = b | (g << 8) | (r << 16);
  }
}

// Set fade level: black = -8, normal = 0, white = 8
static void display_set_fade(int instance, unsigned int *faded_pal) {
  static copper_list_t pal_cop = {0, 0, 0};
  graphics_t *graph = instances[instance].graph;
  pal_cop.commands = graph->pal_ref->commands;
  pal_cop.curr = graph->pal_ref->curr;
  pal_cop.len = graph->pal_ref->len;
  copperInsertPalette(&pal_cop, faded_pal, 1 << graph->depth, graph->bplcon3);
}

// Set black fade level: normal = 0, black >= 16
void display_set_fade_black(int instance, int i) {
  if (i <= 0) {
    display_set_fade(instance, instances[instance].pal256);
    return;
  }
  i -= 1;
  if (i > 15) {
    i = 15;
  }
  display_set_fade(instance, &instances[instance].pal256black[i][0]);
}

// Set white fade level: normal = 0, white >= 16
void display_set_fade_white(int instance, int i) {
  if (i <= 0) {
    display_set_fade(instance, instances[instance].pal256);
    return;
  }
  i -= 1;
  if (i >= 15) {
    i = 15;
  }
  display_set_fade(instance, &instances[instance].pal256white[i][0]);
}

// Set sprites position. You most likely only want to do this during
// vblank as you're modifying the control words in chipmem.
void display_set_sprites_pos(int instance, sprite_options *so) {
  graphicsOptions go = {.numSprites = so->num_sprites};
  if (so->num_sprites > 0) {
    t_bpl_head *sprites = so->spr_data;
    uint8_t *spr_data = sprites->data;
    spr_data += (16 * 3); // Skip palette.
    go.spriteSize = instances[instance].sprSize;
    for (uint32_t i = 0; i < so->num_sprites; i++) {
      go.sprites[i] = spr_data;
      go.spritesX[i] = so->spritesX[i];
      go.spritesY[i] = so->spritesY[i];
      go.spritesAttach[i] = so->spritesAttach[i];
      spr_data += instances[instance].sprSize;
    }
  }

  setSpriteControlWords(instances[instance].graph, &go, SPRITE_SETUP_CHIP);
}

void display_subsystem_init(int numInstances) {
  instances = tndo_malloc(numInstances * sizeof(display_instance), 0);
  maxDisplayInstances = numInstances;
}

void display_subsystem_end() {
#ifdef __DEBUG_CODE
  printf("DEBUG - display_subsystem_end(): %u/%u display instances used.\n",
         lastInstance, maxDisplayInstances);
#endif
}

static int display_init_aga(unsigned int *pal, unsigned int options, int mode,
                            unsigned int padding_top,
                            unsigned int padding_bottom, sprite_options *so) {
  if (!so) {
    so = &empty_so;
  }

  tndo_assert(lastInstance < maxDisplayInstances);

  graphicsOptions go = {.screenMode = mode,
                        .paddingTop = padding_top,
                        .paddingBottom = padding_bottom,
                        .flags = CHUNKY_BUFFERS | REUSE_PLANAR_BUFFERS |
                                 CUSTOM_C2P | PALETTE_IN_COPPER,
                        .tornadoOptions = options,
                        .copperSize = 680, // Obtained from the logs.
                        .customPal = pal,
                        .numSprites = so->num_sprites};

  if (so->num_sprites > 0) {
    t_bpl_head *sprites = so->spr_data;
    uint8_t *spr_data = sprites->data;
    merge_pal(pal, spr_data, 240, 16);
    spr_data += (16 * 3); // Skip palette.
    instances[lastInstance].sprSize =
        _get_spr_size(sprites->w, sprites->h, 4) / so->num_sprites;
    go.spriteSize = instances[lastInstance].sprSize;
    go.tornadoOptions |= USE_SPRITES;
    instances[lastInstance].numSprites = so->num_sprites;
    for (uint32_t i = 0; i < so->num_sprites; i++) {
      go.sprites[i] = spr_data;

      go.spritesX[i] = so->spritesX[i];
      go.spritesY[i] = so->spritesY[i];
      go.spritesAttach[i] = so->spritesAttach[i];

      instances[lastInstance].display_sprites[i] = spr_data;
      spr_data += instances[lastInstance].sprSize;
    }
  }

  for (int i = 1; i <= 16; i++) {
    fade_pal_black(&instances[lastInstance].pal256black[i - 1][0], pal,
                   1.0f - (i / 16.0f), 256);
    fade_pal_white(&instances[lastInstance].pal256white[i - 1][0], pal,
                   i / 16.0f, 256);
  }

  instances[lastInstance].graph = initGraphics(&go);
  setSpriteControlWords(instances[lastInstance].graph, &go, SPRITE_SETUP_FAST);

  instances[lastInstance].paddingTop = padding_top;
  instances[lastInstance].paddingBottom = padding_bottom;
  instances[lastInstance].c2pSkip =
      padding_top * instances[lastInstance].graph->w;

  switch (mode) {
  case SCR_16_9_8BPL_PLANAR:
  case SCR_16_9_H_8BPL_PLANAR:
    instances[lastInstance].isPlanar = 1;
    break;
  }

  instances[lastInstance].mode = mode;

  if (c2pInitDone[mode] == 0) {
    switch (mode) {
    case SCR_NORMAL:
      c2p1x1_8_c5_040_init(instances[lastInstance].graph->w,
                           instances[lastInstance].graph->h, 0, 0, 0,
                           (instances[lastInstance].graph->w *
                            instances[lastInstance].graph->h) /
                               8,
                           0);
      break;

    case SCR_16_9:
      c2p1x1_8_c5_040_16_9_init(instances[lastInstance].graph->w,
                                instances[lastInstance].graph->h, 0, 0, 0,
                                (instances[lastInstance].graph->w *
                                 instances[lastInstance].graph->h) /
                                    8,
                                0);
      break;
    case SCR_16_9_4BPL:
      c2p1x1_4_c5_16_9_init(instances[lastInstance].graph->w,
                            instances[lastInstance].graph->h, 0, 0, 0,
                            (instances[lastInstance].graph->w *
                             instances[lastInstance].graph->h) /
                                8);
      break;
    case SCR_16_9_H_8BPL:
      c2p1x1_8_c5_040_16_9_init(instances[lastInstance].graph->w,
                                instances[lastInstance].graph->h, 0, 0, 0,
                                (instances[lastInstance].graph->w *
                                 instances[lastInstance].graph->h) /
                                    8,
                                0);
      break;
    }
    c2pInitDone[mode] = 1;
  }

  instances[lastInstance].chunky = instances[lastInstance].graph->chunky;
  instances[lastInstance].planar[0] = instances[lastInstance].graph->planar1;
  instances[lastInstance].planar[1] = instances[lastInstance].graph->planar2;
  instances[lastInstance].p = 0;
  instances[lastInstance].pal256 = pal;

  int currentInstance = lastInstance;
  lastInstance++;
  return currentInstance;
}

// RTG screen support
static uint32_t rtgInitDone = 0;
static uint32_t rtgInstance;
static uint32_t *rtgPal;
static uint32_t rtgCurScreen = 0;
static uint32_t rtgIsChunkyBuffer = 0;
static uint32_t rtgScreenSizeX, rtgScreenSizeY, rtgScreenDepth, rtgScreenOffset;
static uint32_t rtgModeSizeX, rtgModeSizeY, rtgModeDepth;
static uint8_t *rtgChunkyBuffer;

static struct Screen *rtgScreen = 0;
static struct Window *rtgWindow = 0;
static struct RastPort rtg_wpa8rastport;
static struct BitMap *rtg_wpa8bitmap;
static struct Rectangle rtgScreenrect;
static struct ScreenBuffer *rtgScreenBuffer[2];
static struct MsgPort *rtgScreenBufferPort[2];

static struct TagItem rtg_vctags[] = {{VTAG_BORDERBLANK_CLR, 1}, {TAG_END, 0}};

static struct TagItem rtgScreenTagList[] = {
    SA_DisplayID,    0x544e444f,
    SA_Width,        0,
    SA_Height,       0,
    SA_Depth,        0,
    SA_Top,          0,
    SA_Left,         0,
    SA_Quiet,        TRUE,
    SA_Type,         CUSTOMSCREEN,
    SA_Title,        (ULONG) "Tornado Screen",
    SA_DClip,        (int)&rtgScreenrect,
    SA_VideoControl, (ULONG)rtg_vctags,
    SA_Behind,       TRUE,
    SA_AutoScroll,   FALSE,
    TAG_END};

static struct TagItem rtgWindowTagList[] = {
    WA_CustomScreen,
    0x544e444f,
    WA_Title,
    (ULONG) "Tornado Window",
    WA_Flags,
    WFLG_ACTIVATE | WFLG_BACKDROP | WFLG_BORDERLESS | WFLG_RMBTRAP,
    WA_IDCMP,
    IDCMP_MOUSEBUTTONS,
    TAG_END,
    0};

static struct TagItem rtgScreenModeTagList[] = {
    BIDTAG_NominalWidth, 0, BIDTAG_NominalHeight, 0, BIDTAG_Depth, 0, TAG_END};

static int display_init_rtg(unsigned int *pal, unsigned int options, int mode,
                            unsigned int padding_top,
                            unsigned int padding_bottom, sprite_options *so) {

  // We only allow one instance for now.
  if (rtgInitDone) {
    return rtgInstance;
  }

  tndo_assert(lastInstance < maxDisplayInstances);

  instances[lastInstance].paddingTop = padding_top;
  instances[lastInstance].paddingBottom = padding_bottom;
  instances[lastInstance].c2pSkip =
      padding_top * instances[lastInstance].graph->w;

  instances[lastInstance].mode = mode;

  switch (mode) {
  case RTG_NORMAL:
    rtgScreenSizeX = 320;
    rtgScreenSizeY = 256;
    rtgScreenDepth = 8;
    rtgScreenOffset = 0;
    rtgModeSizeX = 320;
    rtgModeSizeY = 256;
    rtgModeDepth = 8;
    break;
  }

  rtgScreenModeTagList[0].ti_Data = rtgModeSizeX;
  rtgScreenModeTagList[1].ti_Data = rtgModeSizeY;
  rtgScreenModeTagList[2].ti_Data = rtgModeDepth;

  rtgScreenTagList[1].ti_Data = rtgScreenSizeX;
  rtgScreenTagList[2].ti_Data = rtgScreenSizeY;
  rtgScreenTagList[3].ti_Data = rtgScreenDepth;
  rtgScreenTagList[4].ti_Data = rtgScreenOffset;

  rtgScreenTagList[0].ti_Data = BestModeIDA(rtgScreenModeTagList);
  if (rtgScreenTagList[0].ti_Data == INVALID_ID) {
    fprintf(stderr, "FATAL - Could not obtain valid BestModeID. Aborting.\n");
    abort();
  }

  QueryOverscan(rtgScreenTagList[0].ti_Data, &rtgScreenrect, OSCAN_TEXT);

  if (rtgScreenrect.MaxX - rtgScreenrect.MinX != rtgScreenSizeX - 1) {
    rtgScreenrect.MaxX = rtgScreenrect.MinX + rtgScreenSizeX - 1;
  }

  if (rtgScreenrect.MaxY - rtgScreenrect.MinY != rtgScreenSizeY - 1) {
    rtgScreenrect.MaxY = rtgScreenrect.MinY + rtgScreenSizeY - 1;
  }

  rtgScreen = OpenScreenTagList(0, rtgScreenTagList);
  if (!rtgScreen) {
    fprintf(stderr, "FATAL - Could not open screen. Aborting.\n");
    abort();
  }

  if (GetBitMapAttr(rtgScreen->RastPort.BitMap, BMA_FLAGS) & BMF_STANDARD) {

    rtgScreenBuffer[0] = AllocScreenBuffer(rtgScreen, 0, SB_SCREEN_BITMAP);
    if (!rtgScreenBuffer[0]) {
      fprintf(stderr, "FATAL - Could not allocate screen buffer. Aborting.\n");
      abort();
    }

    rtgScreenBufferPort[0] = CreateMsgPort();
    if (!rtgScreenBufferPort[0]) {
      fprintf(stderr, "FATAL - Could allocate message port. Aborthing.\n");
      abort();
    }

    rtgScreenBuffer[0]->sb_DBufInfo->dbi_SafeMessage.mn_ReplyPort =
        rtgScreenBufferPort[0];

    rtgScreenBuffer[1] = AllocScreenBuffer(rtgScreen, 0, 0);
    if (!rtgScreenBuffer[1]) {
      fprintf(stderr, "FATAL - Could not allocate screen buffer. Aborting.\n");
      abort();
    }

    rtgScreenBufferPort[1] = CreateMsgPort();
    if (!rtgScreenBufferPort[1]) {
      fprintf(stderr, "FATAL - Could allocate message port. Aborthing.\n");
      abort();
    }

    rtgScreenBuffer[1]->sb_DBufInfo->dbi_SafeMessage.mn_ReplyPort =
        rtgScreenBufferPort[0];
    rtgScreenBuffer[0]->sb_DBufInfo->dbi_DispMessage.mn_ReplyPort =
        rtgScreenBufferPort[1];
    rtgScreenBuffer[1]->sb_DBufInfo->dbi_DispMessage.mn_ReplyPort =
        rtgScreenBufferPort[1];

  } else {

    rtgIsChunkyBuffer = 1;

    InitRastPort(&rtg_wpa8rastport);

    rtg_wpa8bitmap =
        AllocBitMap(GetBitMapAttr(rtgScreen->RastPort.BitMap, BMA_WIDTH), 1,
                    GetBitMapAttr(rtgScreen->RastPort.BitMap, BMA_DEPTH), 0,
                    rtgScreen->RastPort.BitMap);
    if (!rtg_wpa8bitmap) {
      fprintf(stderr, "FATAL - RTG display initialisation failed. Aborting.");
      abort();
    }

    rtg_wpa8rastport.BitMap = rtg_wpa8bitmap;
  }

  rtgWindowTagList[0].ti_Data = (int)rtgScreen;

  rtgWindow = OpenWindowTagList(0, rtgWindowTagList);
  if (!rtgWindow) {
    fprintf(stderr, "FATAL - Failed to open window. Aborting.\n");
    abort();
  }

  rtgChunkyBuffer = tndo_malloc(rtgScreenSizeX * rtgScreenSizeY, 0);
  rtgPal = pal;
  rtgInitDone = 1;

  instances[lastInstance].isRTG = 1;
  instances[lastInstance].chunky = rtgChunkyBuffer;
  instances[lastInstance].p = 0;
  instances[lastInstance].pal256 = pal;
  instances[lastInstance].graph =
      (graphics_t *)tndo_malloc(sizeof(graphics_t), 0);
  instances[lastInstance].graph->w = rtgScreenSizeX;
  instances[lastInstance].graph->h = rtgScreenSizeY;

  rtgInstance = lastInstance;
  lastInstance++;
  return rtgInstance;
}

int display_init(unsigned int *pal, unsigned int options, int mode,
                 unsigned int padding_top, unsigned int padding_bottom,
                 sprite_options *so) {
  switch (mode) {
  case SCR_NORMAL:
  case SCR_NORMAL_6BPL:
  case SCR_16_9:
  case SCR_16_9_6BPL:
  case SCR_16_9_4BPL:
  case SCR_16_9_6_4_BPL:
  case SCR_DEBUG4:
  case SCR_16_9_H_4BPL:
  case SCR_FLOAT:
  case SCR_16_9_5BPL:
  case SCR_16_9_H_8BPL:
  case SCR_16_9_8BPL_PLANAR:
  case SCR_16_9_H_8BPL_PLANAR:
    return display_init_aga(pal, options, mode, padding_top, padding_bottom,
                            so);

  case RTG_NORMAL:
  case RTG_NORMAL_6BPL:
  case RTG_16_9:
  case RTG_16_9_6BPL:
  case RTG_16_9_4BPL:
  case RTG_DEBUG4:
  case RTG_16_9_H_4BPL:
  case RTG_FLOAT:
  case RTG_16_9_5BPL:
  case RTG_16_9_H_8BPL:
  case RTG_16_9_8BPL_PLANAR:
  case RTG_16_9_H_8BPL_PLANAR:
    return display_init_rtg(pal, options, mode, padding_top, padding_bottom,
                            so);
    break;
  default:
    fprintf(stderr, "FATAL - Unknown graphics mode %d ( check tornado_settings.h for enum). Aborthing.\n", mode);
    abort();
  }
}

static t_canvas _fb = {.w = 0, .h = 0, .bypp = 1};

t_canvas *display_get(int instance) {

  _fb.w = instances[instance].graph->w;
  _fb.h = instances[instance].graph->h;
  _fb.p.pix8 = instances[instance].chunky;
  if (instances[instance].isPlanar) {
    _fb.bypp = 0;
    _fb.p.pix8 = instances[instance].planar[instances[instance].p^1];
  }
  return &_fb;
}

void display_forefront(int instance) {
  if (instances[instance].isRTG) {
    ScreenToFront(rtgScreen);
    LoadRGB32(&rtgScreen->ViewPort, (ULONG *)rtgPal);
    if (!rtgIsChunkyBuffer) {
      ChangeScreenBuffer(rtgScreen, rtgScreenBuffer[rtgCurScreen]);
    }
  } else {
    clean_bpls(instance);
    if (instances[instance].numSprites > 0) {
      for (int i = 0; i < instances[instance].numSprites; i++) {
        memcpy(instances[instance].graph->sprites[i],
               instances[instance].display_sprites[i],
               instances[instance].sprSize);
      }
    }
    WRL(COP1LCH_ADDR, PBRG(instances[instance].graph->copper->commands));
  }
}

void display_set_copper(int instance) {}

static int prev_instance = 0;

static void display_flip_aga(int instance) {

  instances[instance].p ^= 1;

  switch (instances[instance].mode) {
  case SCR_NORMAL:
    c2p1x1_8_c5_040(instances[instance].chunky + instances[instance].c2pSkip,
                    instances[instance].planar[instances[instance].p]);
    break;
  case SCR_16_9:
    c2p1x1_8_c5_040_16_9(instances[instance].chunky +
                             instances[instance].c2pSkip,
                         instances[instance].planar[instances[instance].p]);
    break;
  case SCR_16_9_4BPL:
    c2p1x1_4_c5_16_9(instances[instance].chunky + instances[instance].c2pSkip,
                     instances[instance].planar[instances[instance].p]);
    break;
  case SCR_16_9_6BPL:
    c2p1x1_6_c5_gen(instances[instance].chunky + instances[instance].c2pSkip,
                    instances[instance].planar[instances[instance].p],
                    instances[instance].graph->w *
                        instances[instance].graph->h);
    break;
  case SCR_16_9_5BPL:
    c2p1x1_5_c5_060(instances[instance].chunky + instances[instance].c2pSkip,
                    instances[instance].planar[instances[instance].p],
                    instances[instance].graph->w *
                        instances[instance].graph->h);
    break;
  case SCR_16_9_H_8BPL:
    c2p1x1_8_c5_040_16_9(instances[instance].chunky +
                             instances[instance].c2pSkip,
                         instances[instance].planar[instances[instance].p]);
    break;
  }

  copper_switch_choose(instances[instance].graph->switch_bpls,
                       instances[instance].p);
  if (prev_instance != instance) {
    if (instances[instance].numSprites > 0) {
      for (int i = 0; i < instances[instance].numSprites; i++) {
        memcpy(instances[instance].graph->sprites[i],
               instances[instance].display_sprites[i],
               instances[instance].sprSize);
      }
    }
    WRL(COP1LCH_ADDR, PBRG(instances[instance].graph->copper->commands));
    prev_instance = instance;
  }
}

static void display_flip_rtg(int instance) {
  if (!rtgIsChunkyBuffer) {
    while (!GetMsg(rtgScreenBufferPort[0]))
      WaitPort(rtgScreenBufferPort[0]);

    c2p1x1_8_c5_bm(rtgChunkyBuffer, rtgScreenBuffer[rtgCurScreen]->sb_BitMap,
                   rtgScreenSizeX, rtgScreenSizeY, 0, 0);

    while (!GetMsg(rtgScreenBufferPort[1]))
      WaitPort(rtgScreenBufferPort[1]);

    ChangeScreenBuffer(rtgScreen, rtgScreenBuffer[rtgCurScreen]);

  } else {

    WritePixelArray8(&rtgScreen->RastPort, 0, rtgCurScreen * rtgScreenSizeY,
                     rtgScreenSizeX - 1,
                     (rtgCurScreen * rtgScreenSizeY) + (rtgScreenSizeY - 1),
                     rtgChunkyBuffer, &rtg_wpa8rastport);

    rtgScreen->ViewPort.RasInfo->RyOffset = rtgCurScreen * rtgScreenSizeY;
    ScrollVPort(&rtgScreen->ViewPort);
    WaitBOVP(&rtgScreen->ViewPort);
  }

  rtgCurScreen = 1 - rtgCurScreen;
}

void display_flip(int instance) {
  if (instances[instance].isRTG) {
    display_flip_rtg(instance);
  } else {
    display_flip_aga(instance);
  }
}

void display_end(int instance) {
  if (instances[instance].isRTG) {
#if 0
		if (!rtgIsChunkyBuffer) {
			ChangeScreenBuffer(rtgScreen, rtgScreenBuffer[0]);
			while (!GetMsg(rtgScreenBufferPort[0]))
				WaitPort(rtgScreenBufferPort[0]);

			printf("Esperata uno.\n");
			ChangeScreenBuffer(rtgScreen, rtgScreenBuffer[1]);
			while (!GetMsg(rtgScreenBufferPort[1]))
				WaitPort(rtgScreenBufferPort[1]);
		}
#endif
    if (rtgWindow) {
      CloseWindow(rtgWindow);
      rtgWindow = 0;
    }

    if (rtgScreen) {
      CloseScreen(rtgScreen);
    }

    if (rtgScreenBufferPort[0]) {
      DeleteMsgPort(rtgScreenBufferPort[0]);
      rtgScreenBufferPort[0] = 0;
    }

    if (rtgScreenBufferPort[1]) {
      DeleteMsgPort(rtgScreenBufferPort[1]);
      rtgScreenBufferPort[1] = 0;
    }

    if (rtgScreenBuffer[0]) {
      FreeScreenBuffer(rtgScreen, rtgScreenBuffer[0]);
      rtgScreenBuffer[0] = 0;
    }

    if (rtgScreenBuffer[1]) {
      FreeScreenBuffer(rtgScreen, rtgScreenBuffer[1]);
      rtgScreenBuffer[1] = 0;
    }

    // We can reset the screen value now.
    rtgScreen = 0;

    if (rtg_wpa8bitmap) {
      FreeBitMap(rtg_wpa8bitmap);
      rtg_wpa8bitmap = 0;
    }
  }
}

int display_checkinput(void) {
  struct IntuiMessage *imsg;
  int mustExit = 0;

  while (imsg = (struct IntuiMessage *)GetMsg(rtgWindow->UserPort)) {
    if (imsg->Class == IDCMP_MOUSEBUTTONS)
      if (imsg->Code == IECODE_LBUTTON)
        mustExit = 1;
    ReplyMsg((struct Message *)imsg);
  }

  return mustExit;
}

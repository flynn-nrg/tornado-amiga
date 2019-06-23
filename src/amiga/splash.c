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

#include <devices/audio.h>
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "assets.h"
#include "c2p.h"
#include "memory.h"
#include "mod_replay.h"
#include "ptr_bridges.h"
#include "splash.h"

static void **splashAssets;
static void *splashSample;
static void *splashSampleSize;
static void *splashMod;
static int *splashModSize;
static int *splashSizes;
static void *loadingMusic = 0;

static int palette[(256 * 3) + 2];

static int cur_screen = 0;
static int isChunkyBuffer = 0;
static int screenSizeX, screenSizeY, screenDepth;
static unsigned char *chunkybuffer;

static struct Screen *NullScr = 0;
struct Window *NullWin = 0;
static struct Screen *screen = 0;
static struct Window *window = 0;
static struct RastPort wpa8rastport;
static struct BitMap *wpa8bitmap;
static struct Rectangle screenrect;
static struct ScreenBuffer *screenBuffer[2];
static struct MsgPort *screenBufferPort[2];

static struct TagItem vctags[] = {{VTAG_BORDERBLANK_CLR, 1}, {TAG_END, 0}};

static struct TagItem screenTagList[] = {
    SA_DisplayID,    0x544e444f,
    SA_Width,        0,
    SA_Height,       0,
    SA_Depth,        0,
    SA_Top,          0,
    SA_Left,         0,
    SA_Quiet,        TRUE,
    SA_Type,         CUSTOMSCREEN,
    SA_Title,        (ULONG) "Tornado Screen",
    SA_DClip,        (int)&screenrect,
    SA_VideoControl, (ULONG)vctags,
    SA_Behind,       TRUE,
    SA_AutoScroll,   FALSE,
    TAG_END};

static struct TagItem windowTagList[] = {
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

static struct TagItem screenModeTagList[] = {
    BIDTAG_NominalWidth, 0, BIDTAG_NominalHeight, 0, BIDTAG_Depth, 0, TAG_END};

// Null Widndow and screen.
ULONG NullPal[(256 * 3) + 2];

// Black background to hide the transition between the animation and the splash
// screen.
static struct Screen *bgNullScr = 0;
struct Window *bgNullWin = 0;
static struct Screen *bgScreen = 0;
static struct Window *bgWindow = 0;
static struct RastPort bgWpa8rastport;
static struct BitMap *bgWpa8bitmap;
static struct Rectangle bgScreenrect;
static struct ScreenBuffer *bgScreenBuffer[2];
static struct MsgPort *bgScreenBufferPort[2];

static struct TagItem bgVctags[] = {{VTAG_BORDERBLANK_CLR, 1}, {TAG_END, 0}};

static struct TagItem bgScreenTagList[] = {
    SA_DisplayID,    0x544e444f,
    SA_Width,        0,
    SA_Height,       0,
    SA_Depth,        0,
    SA_Top,          0,
    SA_Left,         0,
    SA_Quiet,        TRUE,
    SA_Type,         CUSTOMSCREEN,
    SA_Title,        (ULONG) "Tornado Screen",
    SA_DClip,        (int)&screenrect,
    SA_VideoControl, (ULONG)vctags,
    SA_Behind,       TRUE,
    SA_AutoScroll,   FALSE,
    TAG_END};

static struct TagItem bgWindowTagList[] = {
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

static struct TagItem bgScreenModeTagList[] = {
    BIDTAG_NominalWidth, 0, BIDTAG_NominalHeight, 0, BIDTAG_Depth, 0, TAG_END};

void splash_bg_init(void) {

  bgScreenModeTagList[0].ti_Data = 320;
  bgScreenModeTagList[1].ti_Data = 256;
  bgScreenModeTagList[2].ti_Data = 2;

  bgScreenTagList[1].ti_Data = 320;
  bgScreenTagList[2].ti_Data = 256;
  bgScreenTagList[3].ti_Data = 2;
  bgScreenTagList[4].ti_Data = 0;

  bgScreenTagList[0].ti_Data = BestModeIDA(bgScreenModeTagList);
  if (bgScreenTagList[0].ti_Data == INVALID_ID) {
    return;
  }

  QueryOverscan(bgScreenTagList[0].ti_Data, &bgScreenrect, OSCAN_TEXT);

  if (bgScreenrect.MaxX - bgScreenrect.MinX != 320 - 1) {
    bgScreenrect.MaxX = bgScreenrect.MinX + 320 - 1;
  }

  if (bgScreenrect.MaxY - bgScreenrect.MinY != 256 - 1) {
    bgScreenrect.MaxY = bgScreenrect.MinY + 256 - 1;
  }

  bgScreen = OpenScreenTagList(0, bgScreenTagList);
  if (!bgScreen) {
    return;
  }

  if (GetBitMapAttr(bgScreen->RastPort.BitMap, BMA_FLAGS) & BMF_STANDARD) {

    bgScreenBuffer[0] = AllocScreenBuffer(bgScreen, 0, SB_SCREEN_BITMAP);
    if (!bgScreenBuffer[0]) {
      return;
    }

    bgScreenBufferPort[0] = CreateMsgPort();
    if (!bgScreenBufferPort[0]) {
      return;
    }

  } else {

    InitRastPort(&bgWpa8rastport);

    bgWpa8bitmap =
        AllocBitMap(GetBitMapAttr(bgScreen->RastPort.BitMap, BMA_WIDTH), 1,
                    GetBitMapAttr(bgScreen->RastPort.BitMap, BMA_DEPTH), 0,
                    bgScreen->RastPort.BitMap);
    if (!bgWpa8bitmap) {
      return;
    }

    wpa8rastport.BitMap = wpa8bitmap;
  }

  bgWindowTagList[0].ti_Data = (int)bgScreen;

  bgWindow = OpenWindowTagList(0, bgWindowTagList);
  if (!bgWindow) {
    return;
  }

  NullPal[0] = 2 << 16;
  NullPal[1] = NullPal[2] = NullPal[3] = NullPal[4] = NullPal[5] = NullPal[6] =
      0;

  bgNullScr = OpenScreenTags(NULL, SA_DisplayID, bgScreenTagList[0].ti_Data,
                             SA_Colors32, (ULONG)NullPal, SA_VideoControl,
                             (ULONG)bgVctags, TAG_DONE);
  if (!bgNullScr) {
    fprintf(stderr, "FATAL - Failed to open null screen!\n");
    exit(EXIT_FAILURE);
  }

  bgNullWin = OpenWindowTags(NULL, WA_CustomScreen, (ULONG)bgNullScr,
                             WA_Backdrop, TRUE, WA_Borderless, TRUE, WA_RMBTrap,
                             TRUE, WA_Activate, TRUE, TAG_DONE);
  if (!bgNullWin) {
    fprintf(stderr, "FATAL - Failed to open null window!\n");
    exit(EXIT_FAILURE);
  }

  LoadRGB32(&bgScreen->ViewPort, (ULONG *)NullPal);

  SetPointer(bgNullWin,
             (UWORD *)bgNullScr->RastPort.BitMap->Planes[0] + (40 * 40), 1, 16,
             0, 0);
  ActivateWindow(bgNullWin);
  ScreenToFront(bgScreen);
}

static int get_palette_skip_splash(void *pal) {
  int *source = pal;
  return (((ENDI4(source[0]) >> 16) * 3 * sizeof(int)) + (2 * sizeof(int)));
}

static void hide_mouse(struct Window *w) {
  SetPointer(w, (UWORD *)NullScr->RastPort.BitMap->Planes[0] + (40 * 40), 1, 16,
             0, 0);
  ActivateWindow(w);
}

static void open_null_screen(void) {
  NullPal[0] = 2 << 16;
  NullPal[1] = NullPal[2] = NullPal[3] = NullPal[4] = NullPal[5] = NullPal[6] =
      0;

  NullScr =
      OpenScreenTags(NULL, SA_DisplayID, screenTagList[0].ti_Data, SA_Colors32,
                     (ULONG)NullPal, SA_VideoControl, (ULONG)vctags, TAG_DONE);
  if (!NullScr) {
    fprintf(stderr, "FATAL - Failed to open null screen!\n");
    exit(EXIT_FAILURE);
  }

  NullWin = OpenWindowTags(NULL, WA_CustomScreen, (ULONG)NullScr, WA_Backdrop,
                           TRUE, WA_Borderless, TRUE, WA_RMBTrap, TRUE,
                           WA_Activate, TRUE, TAG_DONE);
  if (!NullWin) {
    fprintf(stderr, "FATAL - Failed to open null window!\n");
    exit(EXIT_FAILURE);
  }

  hide_mouse(NullWin);
}

void splash_bg_end() {

  // This causes an infinite loop on some Amiga configs.
#if 0	
  if (!isChunkyBuffer) {
    ChangeScreenBuffer(bgScreen, bgScreenBuffer[0]);
    while (!GetMsg(bgScreenBufferPort[0]))
      WaitPort(bgScreenBufferPort[0]);
  }
#endif

  if (bgWindow) {
    CloseWindow(bgWindow);
    bgWindow = 0;
  }

  if (bgScreen) {
    CloseScreen(bgScreen);
  }

  if (bgNullWin) {
    CloseWindow(bgNullWin);
    bgNullWin = 0;
  }

  if (bgNullScr) {
    CloseScreen(bgNullScr);
    bgNullScr = 0;
  }

  if (bgScreenBufferPort[0]) {
    DeleteMsgPort(bgScreenBufferPort[0]);
    bgScreenBufferPort[0] = 0;
  }

  if (bgScreenBufferPort[1]) {
    DeleteMsgPort(bgScreenBufferPort[1]);
    bgScreenBufferPort[1] = 0;
  }

  if (bgScreenBuffer[0]) {
    FreeScreenBuffer(bgScreen, bgScreenBuffer[0]);
    bgScreenBuffer[0] = 0;
  }

  // We can reset the screen value now.
  bgScreen = 0;

  if (bgWpa8bitmap) {
    FreeBitMap(bgWpa8bitmap);
    bgWpa8bitmap = 0;
  }
}

void splash_end() {

  ScreenToFront(bgScreen);

  if (!isChunkyBuffer) {
    ChangeScreenBuffer(screen, screenBuffer[0]);
    while (!GetMsg(screenBufferPort[0]))
      WaitPort(screenBufferPort[0]);

    if (screenBufferPort[1]) {
      ChangeScreenBuffer(screen, screenBuffer[1]);
      while (!GetMsg(screenBufferPort[1]))
        WaitPort(screenBufferPort[1]);
    }
  }

  if (window) {
    CloseWindow(window);
    window = 0;
  }

  if (screen) {
    CloseScreen(screen);
  }

  if (NullWin) {
    CloseWindow(NullWin);
    NullWin = 0;
  }

  if (NullScr) {
    CloseScreen(NullScr);
    NullScr = 0;
  }

  if (screenBufferPort[0]) {
    DeleteMsgPort(screenBufferPort[0]);
    screenBufferPort[0] = 0;
  }

  if (screenBufferPort[1]) {
    DeleteMsgPort(screenBufferPort[1]);
    screenBufferPort[1] = 0;
  }

  if (screenBuffer[0]) {
    FreeScreenBuffer(screen, screenBuffer[0]);
    screenBuffer[0] = 0;
  }

  if (screenBuffer[1]) {
    FreeScreenBuffer(screen, screenBuffer[1]);
    screenBuffer[1] = 0;
  }

  // We can reset the screen value now.
  screen = 0;

  if (wpa8bitmap) {
    FreeBitMap(wpa8bitmap);
    wpa8bitmap = 0;
  }
}

// Assets:
// 0 -> loading palette in LoadRGB32 and screen data.
// 2 -> loading module (< 128Kb!).
unsigned char *splash_init(const char *const *splashFiles, int numFiles,
                           int sizeX, int sizeY, int offset, int depth,
                           int tornadoOptions) {
  screenSizeX = sizeX;
  screenSizeY = sizeY;
  screenDepth = depth;

  if (numFiles > 0) {
    splashAssets = tndo_malloc(sizeof(void *) * numFiles, 0);
    splashSizes = tndo_malloc(sizeof(int) * numFiles, 0);
    if (!loadAssets(&splashAssets[0], &splashFiles[0], &splashSizes[0],
                    numFiles, tornadoOptions, 0)) {
      return 0;
    }
  }

  // Load optional loading music.
  if (numFiles == 2) {
    loadingMusic = get_chipmem_scratchpad_addr(splashSizes[1]);
    memcpy(loadingMusic, splashAssets[1], splashSizes[1]);
  }

  screenModeTagList[0].ti_Data = sizeX;
  screenModeTagList[1].ti_Data = sizeY;
  screenModeTagList[2].ti_Data = depth;

  screenTagList[1].ti_Data = sizeX;
  screenTagList[2].ti_Data = sizeY;
  screenTagList[3].ti_Data = depth;
  screenTagList[4].ti_Data = offset;

  screenTagList[0].ti_Data = BestModeIDA(screenModeTagList);
  if (screenTagList[0].ti_Data == INVALID_ID) {
    return 0;
  }

  QueryOverscan(screenTagList[0].ti_Data, &screenrect, OSCAN_TEXT);

  if (screenrect.MaxX - screenrect.MinX != sizeX - 1) {
    screenrect.MaxX = screenrect.MinX + sizeX - 1;
  }

  if (screenrect.MaxY - screenrect.MinY != sizeY - 1) {
    screenrect.MaxY = screenrect.MinY + sizeY - 1;
  }

  screen = OpenScreenTagList(0, screenTagList);
  if (!screen) {
    return 0;
  }

  if (GetBitMapAttr(screen->RastPort.BitMap, BMA_FLAGS) & BMF_STANDARD) {

    screenBuffer[0] = AllocScreenBuffer(screen, 0, SB_SCREEN_BITMAP);
    if (!screenBuffer[0]) {
      return 0;
    }

    screenBufferPort[0] = CreateMsgPort();
    if (!screenBufferPort[0]) {
      return 0;
    }

    screenBuffer[0]->sb_DBufInfo->dbi_SafeMessage.mn_ReplyPort =
        screenBufferPort[0];

    if (tornadoOptions & DOUBLE_BUFFERED_SPLASH) {
      screenBuffer[1] = AllocScreenBuffer(screen, 0, 0);
      if (!screenBuffer[1]) {
        return 0;
      }

      screenBufferPort[1] = CreateMsgPort();
      if (!screenBufferPort[1]) {
        return 0;
      }

      screenBuffer[1]->sb_DBufInfo->dbi_SafeMessage.mn_ReplyPort =
          screenBufferPort[0];
      screenBuffer[0]->sb_DBufInfo->dbi_DispMessage.mn_ReplyPort =
          screenBufferPort[1];
      screenBuffer[1]->sb_DBufInfo->dbi_DispMessage.mn_ReplyPort =
          screenBufferPort[1];
    }

  } else {

    isChunkyBuffer = 1;

    InitRastPort(&wpa8rastport);

    wpa8bitmap =
        AllocBitMap(GetBitMapAttr(screen->RastPort.BitMap, BMA_WIDTH), 1,
                    GetBitMapAttr(screen->RastPort.BitMap, BMA_DEPTH), 0,
                    screen->RastPort.BitMap);
    if (!wpa8bitmap) {
      return 0;
    }

    wpa8rastport.BitMap = wpa8bitmap;
  }

  windowTagList[0].ti_Data = (int)screen;

  window = OpenWindowTagList(0, windowTagList);
  if (!window) {
    return 0;
  }

  open_null_screen();

  chunkybuffer = tndo_malloc(sizeX * sizeY, 0);

  return chunkybuffer;
}

// Fade out the provided palette or the asset one if no palette is provided.
// Palettes must be in LoadRGB32 format as follows:
// 1 word with the number of colours to load
// 1 word with the first colour to be loaded.
// 3 longwords representing the left justified 32 bit rgb triplet.
// 1 longword 0 to terminate the list.

// Room for 256 colours in LoadRGB32 format.
static uint32_t fadePal[770];
static float current_r[256];
static float delta_r[256];
static float current_g[256];
static float delta_g[256];
static float current_b[256];
static float delta_b[256];

// Create an all-black palette and load it.
void splash_black_pal(uint32_t *pal) {
  uint32_t *source;
  int numColours;

  if (pal) {
    source = pal;
  } else {
    source = splashAssets[0];
  }

  numColours = source[0] >> 16;
  fadePal[0] = source[0];

  int k = 1;

  for (int j = 0; j < numColours; j++) {
    fadePal[k++] = 0x00ffffff;
    fadePal[k++] = 0x00ffffff;
    fadePal[k++] = 0x00ffffff;
  }

  WaitTOF();
  LoadRGB32(&screen->ViewPort, (ULONG *)fadePal);
}

// Fade the splash screen in.
void splash_fadein(uint32_t *pal, int numSteps) {
  uint32_t *source;
  int numColours;

  if (numSteps > 64)
    numSteps = 64;

  if (pal) {
    source = pal;
  } else {
    source = splashAssets[0];
  }

  numColours = source[0] >> 16;
  fadePal[0] = source[0];

  source++;
  int k = 0;
  for (int i = 0; i < numColours; i++) {
    current_r[i] = 0.0f;
    current_g[i] = 0.0f;
    current_b[i] = 0.0f;

    delta_r[i] = ((float)(source[k++] >> 24)) / (float)numSteps;
    delta_g[i] = ((float)(source[k++] >> 24)) / (float)numSteps;
    delta_b[i] = ((float)(source[k++] >> 24)) / (float)numSteps;
  }

  for (int i = 0; i < numSteps - 1; i++) {
    k = 1;
    for (int j = 0; j < numColours; j++) {
      fadePal[k++] = (((int)current_r[j]) << 24) | 0x00ffffff;
      fadePal[k++] = (((int)current_g[j]) << 24) | 0x00ffffff;
      fadePal[k++] = (((int)current_b[j]) << 24) | 0x00ffffff;

      current_r[j] += delta_r[j];
      current_g[j] += delta_g[j];
      current_b[j] += delta_b[j];
    }
    WaitTOF();
    LoadRGB32(&screen->ViewPort, (ULONG *)fadePal);
  }
}

// Fades the splash screen out.
// Fades the optional loading music out.
// Stops the music.
void splash_fadeout(uint32_t *pal, int numSteps) {
  uint32_t *source;
  int numColours;

  if (numSteps > 64)
    numSteps = 64;

  if (pal) {
    source = pal;
  } else {
    source = splashAssets[0];
  }

  numColours = source[0] >> 16;
  fadePal[0] = source[0];

  source++;
  int k = 0;
  for (int i = 0; i < numColours; i++) {
    current_r[i] = (float)(source[k++] >> 24);
    current_g[i] = (float)(source[k++] >> 24);
    current_b[i] = (float)(source[k++] >> 24);

    delta_r[i] = current_r[i] / (float)numSteps;
    delta_g[i] = current_g[i] / (float)numSteps;
    delta_b[i] = current_b[i] / (float)numSteps;
  }

  for (int i = 0; i < numSteps - 1; i++) {
    k = 1;
    for (int j = 0; j < numColours; j++) {
      current_r[j] -= delta_r[j];
      current_g[j] -= delta_g[j];
      current_b[j] -= delta_b[j];

      fadePal[k++] = (((int)current_r[j]) << 24) | 0x00ffffff;
      fadePal[k++] = (((int)current_g[j]) << 24) | 0x00ffffff;
      fadePal[k++] = (((int)current_b[j]) << 24) | 0x00ffffff;
    }
    WaitTOF();
    if (loadingMusic) {
      p61SetVolOSLegal(numSteps - i);
    }
    LoadRGB32(&screen->ViewPort, (ULONG *)fadePal);
  }

  if (loadingMusic) {
    p61EndOSLegal();
    loadingMusic = 0;
  }
}

void splash_show() {

  splash_black_pal(0);

  unsigned char *chunkyBuffer = (unsigned char *)splashAssets[0];
  // Skip palette.
  chunkyBuffer += get_palette_skip_splash(splashAssets[0]);

  ScreenToFront(screen);

  if (!isChunkyBuffer) {
    ChangeScreenBuffer(screen, screenBuffer[0]);

    while (!GetMsg(screenBufferPort[0])) {
      WaitPort(screenBufferPort[0]);
    }

    c2p1x1_8_c5_bm(chunkyBuffer, screenBuffer[0]->sb_BitMap, screenSizeX,
                   screenSizeY, 0, 0);
    if (screenBuffer[1]) {
      c2p1x1_8_c5_bm(chunkyBuffer, screenBuffer[1]->sb_BitMap, screenSizeX,
                     screenSizeY, 0, 0);
    }

  } else {

    WritePixelArray8(&screen->RastPort, 0, 0, screenSizeX - 1, screenSizeY - 1,
                     chunkyBuffer, &wpa8rastport);
    WaitBOVP(&screen->ViewPort);
  }

  splash_fadein(0, 50);

  if (loadingMusic) {
    p61InitOSLegal(loadingMusic, 0, 0, 0);
    p61SetVolOSLegal(64);
  }
}

void splash_set_palette(void *pal) {
  LoadRGB32(&screen->ViewPort, (ULONG *)pal);
}

void splash_wait_vbl() { WaitTOF(); }

void splash_forefront() {
  ScreenToFront(screen);

  if (!isChunkyBuffer) {
    ChangeScreenBuffer(screen, screenBuffer[cur_screen]);
  }
}

void splash_swap_screens(void) {
  if (!isChunkyBuffer) {
    while (!GetMsg(screenBufferPort[0]))
      WaitPort(screenBufferPort[0]);

    c2p1x1_8_c5_bm(chunkybuffer, screenBuffer[cur_screen]->sb_BitMap,
                   screenSizeX, screenSizeY, 0, 0);

    while (!GetMsg(screenBufferPort[1]))
      WaitPort(screenBufferPort[1]);

    ChangeScreenBuffer(screen, screenBuffer[cur_screen]);

  } else {

    WritePixelArray8(&screen->RastPort, 0, cur_screen * screenSizeY,
                     screenSizeX - 1,
                     (cur_screen * screenSizeY) + (screenSizeY - 1),
                     chunkybuffer, &wpa8rastport);

    screen->ViewPort.RasInfo->RyOffset = cur_screen * screenSizeY;
    ScrollVPort(&screen->ViewPort);
    WaitBOVP(&screen->ViewPort);
  }

  cur_screen = 1 - cur_screen;
}

int splash_checkinput(void) {
  struct IntuiMessage *imsg;
  int mustExit = 0;

  while (imsg = (struct IntuiMessage *)GetMsg(window->UserPort)) {
    if (imsg->Class == IDCMP_MOUSEBUTTONS)
      if (imsg->Code == IECODE_LBUTTON)
        mustExit = 1;
    ReplyMsg((struct Message *)imsg);
  }

  return mustExit;
}

// OS-legal sample replay routines. PAL only!
#define PAL_clock 3546895

UBYTE *chip_sample_buffer;
UBYTE channels[] = {1 | 2};
ULONG device = 0xdeadbeef;
ULONG wakebit;

struct IOAudio *AIOptr;
struct MsgPort *AudioPort;
struct Message *AudioMsg;

// Loads and prepares the sample for replay.
// Samples for splash anims are only meant to be a few seconds long.
// The maximum allowed size is 128Kib.
// Returns 0 if successful.
int splash_load_sample(const char *sampleFile, unsigned int tornadoOptions) {
  splashSampleSize = tndo_malloc(sizeof(int), TNDO_REUSABLE_MEM);
  if (!loadAssets(&splashSample, &sampleFile, splashSampleSize, 1,
                  tornadoOptions | ASSETS_IN_REUSABLE_MEM, 0)) {
    return -1;
  }

  uint32_t *header = splashSample;
  uint32_t speed = PAL_clock / header[0];
  uint32_t size = header[1];
  unsigned char *payload =
      (unsigned char *)splashSample + (2 * sizeof(uint32_t));
  chip_sample_buffer = get_chipmem_scratchpad_addr(size);
  if (!chip_sample_buffer) {
    return -1;
  }

  CopyMem(payload, chip_sample_buffer, size);

  AIOptr = (struct IOAudio *)AllocVec(sizeof(struct IOAudio),
                                      MEMF_PUBLIC | MEMF_CLEAR);
  if (!AIOptr) {
    return -1;
  }

  AudioPort = CreatePort(0, 0);
  if (!AudioPort) {
    return -1;
  }

  AIOptr->ioa_Request.io_Message.mn_ReplyPort = AudioPort;
  AIOptr->ioa_Request.io_Message.mn_Node.ln_Pri = 100;
  AIOptr->ioa_AllocKey = 0;
  AIOptr->ioa_Data = channels;
  AIOptr->ioa_Length = 1;

  device = OpenDevice(AUDIONAME, 0L, (struct IORequest *)AIOptr, 0L);

  if (device != 0) {
    return -1;
  }

  AIOptr->ioa_Request.io_Command = CMD_WRITE;
  AIOptr->ioa_Request.io_Flags = ADIOF_PERVOL;

  AIOptr->ioa_Volume = 64;

  AIOptr->ioa_Period = (UWORD)speed;
  AIOptr->ioa_Cycles = 1;
  AIOptr->ioa_Request.io_Message.mn_ReplyPort = AudioPort;
  AIOptr->ioa_Data = (UBYTE *)chip_sample_buffer;

  AIOptr->ioa_Length = size;

  return 0;
}

// http://wiki.amigaos.net/wiki/Audio_Device
void splash_play_sample() { BeginIO((struct IORequest *)AIOptr); }

// Wait for the sample to finish playing and free all the resources.
void splash_end_sample() {
  wakebit = 0L;
  wakebit = Wait(1 << AudioPort->mp_SigBit);
  while ((AudioMsg = GetMsg(AudioPort)) == 0) {
  };

  if (device == 0) {
    CloseDevice((struct IORequest *)AIOptr);
  }

  if (AudioPort) {
    DeletePort(AudioPort);
  }

  if (AIOptr) {
    FreeVec(AIOptr);
  }
}

// shasha

int splash_load_mod(const char *modFile, unsigned int tornadoOptions) {
  splashModSize = tndo_malloc(sizeof(int), TNDO_REUSABLE_MEM);
  if (!loadAssets(&splashMod, &modFile, splashModSize, 1,
                  tornadoOptions | ASSETS_IN_REUSABLE_MEM, 0)) {
    return -1;
  }

  loadingMusic = get_chipmem_scratchpad_addr(splashModSize[0]);
  memcpy(loadingMusic, splashMod, splashModSize[0]);

  return 0;
}

void splash_play_mod() {
  if (loadingMusic) {
    p61InitOSLegal(loadingMusic, 0, 0, 0);
    p61SetVolOSLegal(64);
  }
}

void splash_end_mod() {
  if (loadingMusic) {
    p61EndOSLegal();
    loadingMusic = 0;
  }
}

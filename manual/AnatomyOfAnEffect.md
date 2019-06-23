Anatomy of an effect
================

All the effects in your demo are driven by `demo.c` and use the `effect` structure defined in `demo.h`.

```c
typedef struct tornadoEffect {
  int minTime;
  int numAssets;
  void **Assets;
  int *assetSizes;
  int debug_color;
  int debug_pos_y;
  // Set to the # of telemetry tracks you need.
  int wantTelemetry;
  TelemetryData **telemetry;

  // Called during demoInit & demoFree.
  void (*init)(unsigned int options, struct tornadoEffect *effect);
  void (*flip)(int requires_forefront);
  void (*free)(struct tornadoEffect *effect);

  // Called once per vertical blank. The parameter has the time relative
  // to the start of the effect.
  void (*vbl)(int frame);
  // Called once on each frame to draw. The frame parameter has the time
  // relative to the start of the effect.
  t_canvas *(*render)(int frame);
  t_canvas *(*rocketRender)(int frame);
  // Rocket track initialisation.
  void (*tracks)(struct sync_device *rocket);
  // Rocket row data getter. Once per frame.
  void (*trackData)(int frame, imguiOverlayData *overlayData);

} tornadoEffect;
```

During the initialisation phase of your demo you will call the `init` function for each of the effects. You will typically load your assets and initialise data that your effect requires. The `minTime` variable defines when your effect will start. The `minTime` of the next effect after this one will dictate when the current effect will end.

Let's look at the simple screen example (`simple_screen.c`) in detail:

This is the effect definition in `demo.c`:

```c
#if SIMPLE_SCREEN_ENABLE & EFFECT_MASK
    {
        .minTime = SMPTE(0, 0, 0),
        .debug_color = 0xff,
        .debug_pos_y = 0,
        .wantTelemetry = 0,
        .init = initSimpleScreen,
        .flip = flipSimpleScreen,
        .free = freeSimpleScreen,
        .vbl = vblNull,
        .render = renderSimpleScreen,
        .rocketRender = renderNull,
        .tracks = tracksNull,
        .trackData = trackDataNull,
    },
#endif
```

The `if` statement in the beginning allows us to quickly switch effects on and off for testing.

We've set the `minTime` to 0 minutes, 0 seconds and 0 frames. This effect will be the first one to run.

The `debug_color` and `debug_pos_y` configure the colour and y position of the on-screen profiler information. See the [Performance Monitoring](PerformanceMonitoring.md) section for more information.

`wantTelemetry` is used to configure performance data. Again, refer to the  [Performance Monitoring](PerformanceMonitoring.md) section for more information.

If you don't need a specific function for your effect you can just use the supplied `Null` implementations. In this example we do not need a [vertical blank](https://en.wikipedia.org/wiki/Vertical_blanking_interval) callback, so we just set it to the null version. Similarly, this effect does not have [Rocket](Rocket.md) support, so we just used the `Null` functions.

Effect implementation
-----------------------------

First we need an include file so that `demo.c` gets the function declarations. In this example, said file is `simple_screen.h`:

```c
#ifndef SIMPLE_SCREEN_H
#define SIMPLE_SCREEN_H

#include <demo.h>
#include <imgui_overlay.h>

void initSimpleScreen(unsigned int, tornadoEffect *);
void freeSimpleScreen(tornadoEffect *);
t_canvas *renderSimpleScreen(int);
void flipSimpleScreen(int);

#endif
```

You can see these functions referenced above when we defined the effect structure.

Let's look at what each of these do in the implementation file (`simple_screen.c`):

Initialisation
----------------

```c
void initSimpleScreen(unsigned int tornadoOptions, tornadoEffect *effect) {

  static int init = 0;
  if (init)
    return;
  init = 1;

  effect->numAssets = sizeof(assetList) / sizeof(char *);
  effect->Assets = (void **)tndo_malloc(sizeof(void *) * effect->numAssets, 0);
  effect->assetSizes = (int *)tndo_malloc(sizeof(int) * effect->numAssets, 0);
  if (!loadAssets(effect->Assets, &assetList[0], effect->assetSizes,
                  effect->numAssets, tornadoOptions, 0)) {
    tndo_memory_shutdown(tornadoOptions);
    if (tornadoOptions & LOGGING) {
      printf("FATAL - Asset loading failed!\n");
    }
    exit(1);
  }

  // Palette is stored in LoadRGB32 format so we need to convert it first.
  loadRGB32toRGB((uint32_t *)effect->Assets[0], pal);

  // 320x256 8 bitplanes. No sprites and no padding.
  displayInstance = display_init(pal, tornadoOptions, SCR_NORMAL, 0, 0, 0);

  // The first 3080 bytes are the palette in LoadRGB32 format.
  background = (char *)effect->Assets[0] + 3080;
}
```

This is our init function. It will be called during the initialisation phase when the operating system is still running and receives two parameters:

* `tornadoOptions`: This variable contains all the demo flags. Refer to the [Anatomy of a Demo](AnatomyOfADemo.md) and [tornadoOptions](tornadoOptions.md) sections for more information about this.
* A pointer to this effect's structure.

The first statement makes sure that init is only attempted once.

The we load the assets for this effect. In this case we only have a background declared earlier in the file like this:

```c
static const char *assetList[] = {
    "data/Capsule_logo.tndo", // Compressed raw pixel data and palette.
};
```

The [Asset Manager](AssetManager.md) takes care of loading and unpacking data for you.

Once we have loaded the file we prepare the palette and initialise the [Display](Displays.md).

We also keep a pointer to the background image for later use.

The Render function
---------------------------

The render function is called over and over until the time to switch to the next effect is reached or the demo ends.

The demo timer is driven by a 50Hz counter which is created from a level 3 interrupt on the Amiga and by a timer on the SDL implementation. 

The render function receives a `frame` number. This frame is always relative to the effect, so regardless of when the effect runs, the first frame is always 0.

Depending on how long your render function takes the next frame might not be 1. This allows your demo to run more smoothly on faster systems while keeping timing intact.

Let's look at the render function:

```c
t_canvas *renderSimpleScreen(int frame) {
  t_canvas *c = display_get(displayInstance);
  unsigned char *chunky = c->p.pix8;

  // Trivial copy...
  memcpy(chunky, background, 320 * 256);

  return c;
}
```

The first thing we need is a chunky buffer to write to. We obtain it by calling `display_get`. This returns a pointer to a `canvas` struct that has a pointer to the chunky buffer. 

In this trivial example our render function simply copies the background image to the buffer. We are not making use of the `frame` variable but we could use it to e.g. copy different background images based on which frame we are rendering.

The last thing we need to do is return the pointer to the canvas. This is used by the [Performance Monitoring](PerformanceMonitoring.md) system to display data on top of your image.

The Flip Function
-----------------------

Once the render function has completed and the profiling data has been layered on top (if enabled), `demo.c` will call the `flip` function. Its main purpose is to, as its name suggests, perform a flip of the double buffer and show the newly rendered image. This functionality is implemented by the [Display](Displays.md) subsystem and most of the time you will just call the `display_flip` function.

```c
void flipSimpleScreen(int requires_forefront) {
  if (requires_forefront) {
    display_forefront(displayInstance);
  }

  display_flip(displayInstance);
}
```

The `requires_forefront` variable will be set if your effect just gained focus. It is done this way so that you can seamlessly switch between effects. This makes sure the bitplane memory regions are cleared and that the correct Copper list is used.

And finally we have the `free` function:

```c
void freeSimpleScreen(tornadoEffect *effect) {}
```

In this case this function is not doing anything. Because the Tornado [Memory Manager](MemoryManager.md) automagically frees memory for you, there's no need to explicitly call `free`.

What's the use of this you may ask? Anything you want to do when the demo has ended and the operating system is running again right before exiting. An example of this is saving telemetry data as you can see in the [Performance Monitoring](PerformanceMonitoring.md) section.

That's it. You've successfully implemented an effect! You can go read about the [Anatomy of a Demo](AnatomyOfADemo.md) to see how you can put multiple effects together.



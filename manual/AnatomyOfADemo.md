Anatomy of a demo
===============

A demo is comprised of at least one effect. You can learn about effects in the [Anatomy of an effect](AnatomyOfAnEffect.md) section.

The main component of a demo is the file ```demo.c```

A demo implements the following functions:

```c
void demoSettings(demoParams *);
void demoInit(unsigned int, int);
void demoSplash(unsigned int);
void demoMain(unsigned int, memoryLog *);
void demoVBL(void);
void demoFree(void);
```

We are going to use the ```simple_screen``` example to see how this is done.

The first thing we are going to do is declare the effects that we have:

```c
// Your demo effects come here...
#include "simple_screen/simple_screen.h"

static tornadoEffect effects[] = {

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
    {
        .minTime = SMPTE(0, 30, 0),
        .init = initNull,
        .flip = 0,
        .free = freeNull,
        .vbl = vblNull,
        .render = renderNull,
        .rocketRender = renderNull,
        .tracks = tracksNull,
        .trackData = trackDataNull,
    },
    {
        .minTime = 50 * 9999,
        .init = initNull,
        .flip = 0,
        .free = freeNull,
        .vbl = vblNull,
        .render = renderNull,
        .rocketRender = renderNull,
        .tracks = tracksNull,
        .trackData = trackDataNull,
    }};
```

The last two effects always exist and are used by Tornado to trigger the exit at the end. To learn more about what each parameter means please refer to the [Anatomy of an effect](AnatomyOfAnEffect.md) section.


Demo Settings
--------------------

The first thing the Tornado framework is going to do is call the ```demoSettings``` function. This will configure all the demo parameters.

```c
void demoSettings(demoParams *dp) {
  dp->minCPU = MIN_CPU_040;
  dp->tornadoOptions = KILL_OS | LOGGING | INSTALL_LEVEL3 | INSTALL_LEVEL2;
#ifdef __DEBUG_CODE
  dp->tornadoOptions |= VERBOSE_DEBUGGING | MEMORY_PROFILING;
#endif
  dp->minFast = 1 * 1024 * 1024;
  dp->minChip = 1000 * 1024;
  dp->fastMemPool = FAST_MEM_POOL_SIZE * 1024 * 1024;
  dp->chipMemPool = 2 * 1024;
  dp->chipScratch = 1024 * 700;
  dp->packedData = 1 * 1024 * 1024; // Enough rooom to hold LZW-compressed assets' dictionaries plus streaming buffers.
  dp->numDisplays = 1;
  my_dp = dp;
}
```

* ```minCPU``` sets the minimum CPU that this demo requires. Unless you have a very good reason not to do so, leave it set to 68040. Tornado is able to detect the [Apollo 68080](http://apollo-core.com/) but will not take advantage of it yet. This will happen in later releases once the [Vampire 1200](https://www.apollo-accelerators.com/) accelerators become available.
* ```tornadoOptions``` sets a series of flags that determine how parts of the framework behave. Please refer to the [tornadoOptions](tornadoOptions.md) section for more details. As a general rule, these are sane defaults and you usually do not need to change them.

The following settings are all related to the [Memory Manager](MemoryManager.md):

* ```minFast``` is the minimum contiguous block of Fast RAM that your demo requires. If your demo needs 64MiB and you set this to 64MiB it is almost guaranteed that it will not work as the memory will be fragmented by the time your demo is executed. Setting it to 32MiB is a good option.
* ```minChip``` is the minimum contiguous block of Chip RAM that your demo needs. If you are not implementing custom displays that use a lot of Chip RAM this can be relatively small.
* ```fastMemPool``` is a pool of Fast RAM that can be allocated at run time, i.e. once the demo is running. Please refer to the [Memory Manager](MemoryManager.md) for more details.
* ```chipMemPool``` is being phased out in favour of the ```chipScratch``` scratch pad and should not be used at this point. At this point it still cannot be zero, so set it to a couple of KiB.
* ```chipScratch``` provides a Chip RAM scratch pad that effects and displays can use during the execution of the demo.  Please refer to the [Memory Manager](MemoryManager.md) section for more details.
* ```packedData``` needs to be at least as big as the largest LZW dictionary plus enough room for the streaming buffers. While both LZSS and LZW assets are streamed and unpacked in chunks, the LZW dictionary needs to be held in memory.

And the last setting:

* ```numDisplays``` is the number of display instances that your demo needs. If you have 5 effects each of which allocates a display instance then you need to set this to 5. The [Displays](Displays.md) section describes how displays work in more detail.

Demo Initialisation
-------------------------

Now that we've told Tornado how we want things it's time to initialise the entire demo:

```c
void demoInit(unsigned int tornadoOptions, int initialEffect) {
  uint32_t before, after, initTime;

  demoInitDone = 0;

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - demoInit()\n");
  }

  if (initialEffect > numEffects) {
    fprintf(stderr, "FATAL - Tried to jump to effect %i but there's only %i.\n",
            initialEffect, numEffects);
    tndo_memory_shutdown(tornadoOptions);
    exit(1);
  }
```

We receive two parameters: the flags we just set before and the initial effect that the demo will start from.


```c
  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - Starting demo from effect <%i>\n", initialEffect);
  }

  currentEffect = initialEffect;
  epoch = effects[initialEffect].minTime;

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - demoAudioInit()\n");
  }

  initTime = 0;

#ifdef __AMIGA__
  timeGet(&initBegin);
  timeGet(&initEffectBegin);
#endif

```

We set up the timing functions that will report how long each effect takes to initialise.

```c
  demoAudioInit(tornadoOptions);
```

This function is used to perform audio related initialisation and can be empty if you don't need to do any, as is the case in this example.

```c
#ifdef __AMIGA__
  timeGet(&initEffectEnd);
  initTime = timeDiffSec(&initEffectBegin, &initEffectEnd);
#endif

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - demoAudioInit() completed in %u seconds\n", initTime);
  }

  for (int i = 0; i < numEffects; i++) {

    if (effects[i].wantTelemetry) {
      effects[i].telemetry =
          allocateTelemetry(effects[i].wantTelemetry,
                            effects[i + 1].minTime - effects[i].minTime);
    }

    if (tornadoOptions & VERBOSE_DEBUGGING) {
      printf("DEBUG - effectInit(%d)\n ", i);
      before = tndo_memory_used();
    }

#ifdef __AMIGA__
    timeGet(&initEffectBegin);
#endif
    effects[i].init(tornadoOptions, &effects[i]);

#ifdef __AMIGA__
    timeGet(&initEffectEnd);
    initTime = timeDiffSec(&initEffectBegin, &initEffectEnd);
#endif

    if (tornadoOptions & VERBOSE_DEBUGGING) {
      after = tndo_memory_used();
      printf("DEBUG - effectInit(%d) allocated %u bytes.\n", i, after - before);
      printf("DEBUG - effectInit(%d) completed in %u seconds.\n ", i, initTime);
    }
  }
```

The for loop will iterate through each effect declared in the ```effects``` array and call its init function. It also initialises the telemetry data structures if your effect requires them. Please refer to the [Performance Monitoring](PerformanceMonitoring.md) section for more details about how this works.

```c
  numAudioAssets = sizeof(audioList) / sizeof(char *);
  audioSizes = (int *)tndo_malloc(sizeof(int) * numAudioAssets, 0);
  audioAssets = (void **)tndo_malloc(sizeof(void *) * numAudioAssets, 0);
  if (!loadAssets(&audioAssets[0], &audioList[0], &audioSizes[0],
                  numAudioAssets, tornadoOptions, my_dp)) {
    tndo_memory_shutdown(tornadoOptions);
    if (tornadoOptions & VERBOSE_DEBUGGING) {
      printf("failed!\n");
    }
    exit(1);
  }

  // Copy module to chip mem
#ifdef __AMIGA__
  chipBuffer = tndo_malloc(audioSizes[0], TNDO_ALLOC_CHIP);
  memcpy(chipBuffer, audioAssets[0], audioSizes[0]);
#endif

#ifdef __AMIGA__
  timeGet(&initEnd);
  initTime = timeDiffSec(&initBegin, &initEnd);
#endif

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - demoInit() completed in %u seconds.\n", initTime);
  }

  demoInitDone = 1;
}
```

This last part loads an Amiga module into Chip RAM so that we have some music playing during our demo. The [Audio](Audio.md) section expands on this topic.

The ```demoSplash``` function is called if the ```SHOW_SPLASH``` flag is set. It is used to open a system friendly screen that is displayed while the demo is loading. If you don't have such a requirement you can leave it empty, like this:

```c
void demoSplash(unsigned int tornadoOptions) {}
```

Demo Main
----------------

At this point the framework has shut down Amiga OS and you have full access to the hardware.

```c
// ---------------------------------------------------------------------------
// Main demo loop begins here...
// ---------------------------------------------------------------------------
void demoMain(unsigned int tornadoOptions, memoryLog *log) {

  int timings = 0;
  prof_enabled(timings);
  int loadEffect = 1;
  int mustExit = 0;
  int oldTime = 0;

  prof_reset();

  int requires_forefront = 0;
  lastFrame = -1;
```

First we set up the initial state and reset the profiler.

```c
  // Kick-off music. Note that this only works on Amiga!
#ifdef __AMIGA__
  p61Init(chipBuffer, 0, 0, 0);
#endif
```
We start the music.

```c
  for (;;) {
    if (loadEffect) {
      loadEffect = 0;
      requires_forefront = 1;
    }
    tornadoEffect *e = &effects[currentEffect];
    oldTime = epoch;
    oldTime += getMasterTimer();
    if (lastFrame == oldTime) {
      continue;
    }

```

This is an infinite loop that will exit when either the demo ends or the user presses the left mouse button.

The call to ```getMasterTimer()``` returns the current time in frames. This is used to tell each effect which frame they need to render and to move between effects.

**IMPORTANT**: Remember that your effect works with relative time, so the first frame is always 0.

```c
    int effectLocalTime = oldTime - e->minTime;
    t_canvas *c = 0;

#ifndef __AMIGA__

    imguiOverlayData overlayData = {
        oldTime, // SMPTE Time.
        0,       // Number of sliders.
        0,       // Slider array.
    };
    imgui_overlay_set(&overlayData);
    if (e->render)
      c = e->render(effectLocalTime);
#else

    if (e->render) {
      c = e->render(effectLocalTime);
      lastFrame = oldTime;
    }
#endif
```

Now we call the ```render``` function of the effect.

```c
    if (e->flip)
      e->flip(requires_forefront);
```

And right after that we call ```flip```.

```c
    requires_forefront = 0;

    if (mousePressL() || (e->render == renderNull)) {
      mustExit = 1;
    } else {
      while (effects[currentEffect + 1].minTime < oldTime) {
        currentEffect++;
        requires_forefront = 1;
        if (currentEffect >= numEffects) {
          mustExit = 1;
          break;
        }
        loadEffect = 1;
      }
    }
    if (mustExit) {
      break;
    }
  }

  // Stop music.
#ifdef __AMIGA__
  p61End();
#endif
}
```
We check whether we should exit and do so if that's the case.

The Master VBL callback
----------------------------------

This function is called by the Tornado framework once every vertical blank and is used to call the VBL
functions of each effect.

**IMPORTANT**: Remember that your effect works with relative time, so the first frame is always 0.

```c
// --------------------------------------------------------------------------
// Master VBL callback
// --------------------------------------------------------------------------
void demoVBL() {
  int oldTime = 0;

  oldTime = epoch;
  oldTime += getMasterTimer();
  tornadoEffect *e = &effects[currentEffect];
  if (e->vbl)
    e->vbl(oldTime - e->minTime);
}
```

The Free function
------------------------

The Tornado [Memory Manager](MemoryManager.md) frees all the memory for you. This function is now used for any clean up operation you might want to perform at the end of your demo, such as saving telemetry data.


```c
// ---------------------------------------------------------------------------
// Do NOT forget to free all your memory resources!!!
// ---------------------------------------------------------------------------
void demoFree() {
  for (int i = 0; i < numEffects; i++) {
    effects[i].free(&effects[i]);
  };
}
```

To see a more complex version of this you can check out the ```zoom``` example, which uses streaming music and has a VBL callback that handles the fade to white at the end.
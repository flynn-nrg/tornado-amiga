Creating custom displays
============

Tornado 2.0 ships with a unified [Display Manager](Displays.md). At the time we made [Brutalism](http://www.pouet.net/prod.php?which=81062) this was not the case, so every effect had to implement its own.

In this section we will take a look at the custom display I wrote for the splash screen shown while the demo is loading.

If you remember from the [Displays](Displays.md) chapter, every display needs to comply with the API and expose the following functions:

* ```Init``` : Allocates resources for the display and performs initialisation.
* ```Flip``` : Kicks off the copy to Chip RAM and swaps the double buffer active instance.
* ```Forefront``` : Prepares the Chip RAM areas that contain the planar data, copies the sprites to Chip RAM and sets up the Copper list pointers.
* ```Get``` : Returns a chunky buffer that your effect can write to.

The splash display extends the API and adds a few more functions. The AmigaOS dependent parts of the display are implemented in the ```amiga/splash.c``` file in the Tornado source directory, and deal with opening screens and allocating buffers as well as sound replay.

Let's take a look now at the display implementation in the ```splash``` example:

Amiga implementation
------------

This is just a very thin layer on top of the ```amiga/splash.c``` code:

We have a non-standard ```Init``` function here because this display also does asset management.

```c
void display_init_splash(const char *const *splashFiles, int numFiles,
                         int sizeX, int sizeY, int offset, int depth,
                         int tornadoOptions) {
  _chunky = splash_init(splashFiles, numFiles, sizeX, sizeY, offset, depth,
                        tornadoOptions);
  if (!_chunky) {
    fprintf(stderr, "FATAL - Cannot initialise splash screen. Aborting.\n");
    exit(EXIT_FAILURE);
  }

  _fb.w = sizeX;
  _fb.h = sizeY;
  _fb.p.pix8 = _chunky;
}
```

The ```Get``` function returns a pointer to the chanvas that contains the chuky buffer information:

```c
t_canvas *display_get_splash() { return &_fb; }

void display_waitvbl_splash(int num_frames) {
  for (int i = 0; i < num_frames; i++) {
    splash_wait_vbl();
  }
}
```

```Forefront``` calls the underlying hardware-dependent implementation. We also have a helper
function to set the palette and calculate where the palette ends in a file converted with  ```png2raw```.
```c
void display_forefront_splash() { splash_forefront(); }

void display_set_palette_splash(void *palette) { splash_set_palette(palette); }

int display_get_palette_skip_splash(void *pal) {
  int *source = pal;
  return (((ENDI4(source[0]) >> 16) * 3 * sizeof(int)) + (2 * sizeof(int)));
}

```

Last but not least we have two wrappers around the ```Show``` and ```Swap Screens``` functions and the ```End``` call, which fades out the image and the music and frees up the resources.
```c
void display_show_splash() { splash_show(); }

void display_flip_splash() { splash_swap_screens(); }

void display_end_splash(uint32_t *pal) {
  splash_fadeout(pal, 50);
  splash_end();
}
```

SDL/Posix implementation
------------------------

The SDL/Posix implementatin is self contained in the ```sdl_posix/display_splash.c``` file in the ```splash``` example.

The ```set_palette``` function converts a palette array in [LoadRGB32](http://amigadev.elowar.com/read/ADCD_2.1/Includes_and_Autodocs_3._guide/node02FB.html) format to something we can use in our SDL target, also taking care of the endianness. 
```c
void display_set_palette_splash(void *pal) {
  uint32_t *source = (uint32_t *)pal;
  int numCols;
  int startCols;

  numCols = (ENDI4(source[0])) >> 16;
  int k = 0;
  source++;
  for (int i = 0; i < numCols; i++) {
    unsigned int r = source[k++];
    unsigned int g = source[k++];
    unsigned int b = source[k++];
    _pal256_splash[i] = ((ENDI4(r)) & 0xff000000) >> 8;
    _pal256_splash[i] |= ((ENDI4(g)) & 0xff000000) >> 16;
    _pal256_splash[i] |= ((ENDI4(b)) & 0xff000000) >> 24;
  }
}
```

The ```Init``` function loads the associated assets and initialises the SDL window.
```c
void display_init_splash(const char *const *splashFiles, int numFiles,
                         int sizeX, int sizeY, int offset, int depth,
                         int tornadoOptions) {
  _fb_splash.w = sizeX;
  _fb_splash.h = sizeY;
  _fb_splash.bypp = 1;
  _fb_splash.p.pixels =
      tndo_malloc(_fb_splash.w * _fb_splash.h * _fb_splash.bypp, 0);

  if (numFiles > 0) {
    splashAssets = (void **)tndo_malloc(sizeof(void *) * numFiles, 0);
    splashSizes = (int *)tndo_malloc(sizeof(int) * numFiles, 0);
    if (!loadAssets(&splashAssets[0], &splashFiles[0], &splashSizes[0],
                    numFiles, tornadoOptions, 0)) {
      fprintf(stderr, "Failed to load splash asserts. Aborting");
      abort();
    }
  }

  if (sizeX > 320 || sizeY > 180) {
    splashHires = 1;
  }

  if (numFiles > 0) {
    display_set_palette_splash(splashAssets[0]);
  }

  dev_window_output_init(flip_delay, 0);
}
```

```WaitVBL```, ```Forefront``` and ```End``` are stubs and only exist to satisfy the API.
```c
void display_waitvbl_splash(int num_frames) {}

void display_forefront_splash() {}

void display_end_splash(uint32_t *pal) {}
```

Because the raw files pack the palette and the pixel data together, we use this function to know where the palette ends.
```c
int display_get_palette_skip_splash(void *pal) {
  uint32_t *source = (uint32_t *)pal;
  int numCols = (ENDI4(source[0])) >> 16;
  return ((numCols * 3 * sizeof(uint32_t)) + (2 * sizeof(uint32_t)));
}
```

As previously, we just return a pointer to the canvas struct.
```c
t_canvas *display_get_splash() { return &_fb_splash; }
```

The ```Flip``` function on SDL needs to convert a colour index array into RGB data that SDL can consume. We also double the size if the screen resolution is 320 and centre it if it's a 16:9 mode.

You will notice that there's no actual centering in the hires case. The reason for that is that in the SDL/posix target the whole demo loads and initialises in a second so you don't even have the chance to see the loading screen, thus I didn't bother. The animation, which is in low res, is shown on both targets, so I wanted that to look ok on both.

```c
void display_flip_splash() {
  int x, y;
  int y_delta;

  t_canvas *out = dev_window_get_canvas();
  tndo_assert((_fb_splash.w) <= out->w);
  tndo_assert((_fb_splash.h) <= out->h);

  if (splashHires) {
    for (y = 0; y < _fb_splash.h; y++) {
      unsigned int *dst_a = out->p.pix32 + y * out->w;

      for (x = 0; x < _fb_splash.w; x++) {
        unsigned int og = _fb_splash.p.pix8[y * _fb_splash.w + x];
        unsigned int c = _pal256_splash[og];

        dst_a[x] = c;
      }
    }

  } else {

    // This splash screen is 16:9. Centre it.
    y_delta = ((out->h / 2) - SCREEN_H) / 2;

    for (y = 0; y < _fb_splash.h; y++) {
      unsigned int *dst_a = out->p.pix32 + (y + y_delta) * 2 * out->w;
      unsigned int *dst_b = out->p.pix32 + (((y + y_delta) * 2) + 1) * out->w;
      for (x = 0; x < _fb_splash.w; x++) {
        unsigned int og = _fb_splash.p.pix8[y * _fb_splash.w + x];
        unsigned int c = _pal256_splash[og];

        dst_a[x * 2] = c;
        dst_a[x * 2 + 1] = c;
        dst_b[x * 2] = c;
        dst_b[x * 2 + 1] = c;
      }
    }
  }

  dev_window_output_flip();
}
```

```Show Splash``` copies the image data to the chunky buffer, sets the palette and forces a flip.
```c
void display_show_splash() {
  unsigned char *source = (unsigned char *)splashAssets[0];
  // Skip palette
  source += display_get_palette_skip_splash(splashAssets[0]);

  display_set_palette_splash(splashAssets[0]);

  memcpy(_fb_splash.p.pix8, source, _fb_splash.w * _fb_splash.h);
  display_flip_splash();
}

```

Putting it all together
-----------------------

We need to make sure we link these object files into our demo. In order to do that we add them in the Makefile.


```c
#################################################################################
# Tornado root directory
#################################################################################
TORNADO_BASE = ../..

#################################################################################
# Your demo code comes here
#################################################################################

LOCAL_INCDIR  = ./src 

# Demo objects
DEMO_OBJS += demo.o

# Anim replay and custom display
DEMO_OBJS += anim_play.o
DEMO_OBJS += amiga/display_splash.o

# Save all object and source files to tmp.
BUILDDIR = /tmp/build-amiga

# Remove asserts while compiling.
CCFLAGS += -DNDEBUG

TARGET = ${SHARED}/splash.68k

# Remove for final release
CCFLAGS += -D__DEBUG_CODE

# Tndo malloc debug.
#CCFLAGS += -DTNDO_MEMORY_DEBUG

#################################################################################
# Do NOT add anything below this line!!!
#################################################################################
include $(TORNADO_BASE)/tornado.mk
```

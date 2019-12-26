Music
=====

No demo would be complete without a soundtrack. Tornado provides three ways to play music:

* Old school [Amiga modules](https://modarchive.org/index.php?article-modules) using [The Player](http://www.pouet.net/prod.php?which=64049) by Sahara Surfers.
* [Pretracker](http://www.pouet.net/prod.php?which=80999) modules.
* DDPCM compressed stereo 14bit audio up to 22050Hz.

Using modules
---------------------

Modules have the advantage of potentially requiring a very small amount of memory and are best suited for intros. Capsule's 2018 [Renouncetro](http://www.pouet.net/prod.php?which=75732) 64Kib intro is such an example.

Tornado contains two copies of the P61 replay routine: One that's AmigaOS friendly and thus can be used before the demo has started, e.g. while the demo is loading, and one that uses direct hardware access and can only be used while the demo is running.

An example of how to use module music in your demo can be seen in the [Anatomy of a Demo](AnatomyOfADemo.md) section which uses the ```simple_screen``` example as a guide.

A few things to keep in mind:

* You can have as many modules as you want as long as you call ```p61Init``` and ```p61End``` for each of them.
* Modules need to reside in Chip RAM. Make sure the module data resides in Chip RAM.
* Module replay **only** works on the Amiga target. You will not hear anything when running your demo on the posix/SDL target.
* Because profiling needs the CIAB timer which is used by the module replay routine, it is not possible to have profiling and music at the same time on Amiga. Please refer to the [Performance Monitoring](PerformanceMonitoring.md) section for more information on this topic.

Using Pretracker modules
--------------

Tornado provides wrapper routines that allow you to use the Pretracker replay routine. A ready made example is available under the ```examples/simple_screen_prt``` directory.

Pretracker modules are ideal for intros as they need very little amount of storage and the precalc phase is very fast on accelerated Amigas.

Since the replay routine is called from the VBL handler, that frees up the CIAB hardware so you can use the profiler while the music is playing.

All you have to do is load the asset and then call the init routine, like this:

```c
const char *audioList[] = {"data/aceman_glitch.prt"};

  // Allocate memory for the generated samples
  // and initialise the song and replay routine.
#ifdef __AMIGA__
  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - Pretracker init...");
  }  
  chipBuffer = tndo_malloc(128*1024, TNDO_ALLOC_CHIP);
  prtInit(chipBuffer, audioAssets[0]);
  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("done.\n");
  }    
#endif
```

And then you call the tick function every vblank:

```c
  // Call Pretracker tick every vblank.
#ifdef __AMIGA__
  prtVBL();
#endif
```

Last but not least we will shutdown Paula at the end of demoMain:

```c
  // Stop music.
#ifdef __AMIGA__
  prtEnd();
#endif
```

Using DDPCM compressed music
----------------------------------------------

As a composer myself, the idea of our music being butchered by the 4bit [ADPCM](https://en.wikipedia.org/wiki/Adaptive_differential_pulse-code_modulation) compression wasn't very appealing. While this algorithm results in small audio assets, it does not sound good. I always say that ADPCM-encoded music sounds nice if you've never listened to the original.

For that reason I've developed my own [DPCM](https://en.wikipedia.org/wiki/Differential_pulse-code_modulation) encoder that delivers very good quality with low decoding overhead and a reasonable compression ratio of about 2.4:1. 

A highly optimised replay routine is provided with the framework.

The first thing you need is a 16bit stereo WAV file with data saved at one of these supported sampling rates:

* 11025Hz
* 22050Hz

A 11025Hz version of the music is practical during development as it minimises loading times. For the release version of your demo we recommend that you create a 22050Hz file to obtain the highest quality.

You are also going to need an [MP3](https://en.wikipedia.org/wiki/MP3) or [OGG](https://xiph.org/ogg/) version of your track to be used by the posix/SDL target. This one can be the full 16bit 44100Hz version of your music as it came out of your DAW.


Let's convert a WAV file to a TNDO audio file:

```
thunderball:~ mmendez$ tndo_ddcpm -i brut_final_22050_16.wav -o brut_final_22050_16_ddpcm.tndo

Tornado DDPCM encoder. (C) 2019 Miguel Mendez.

Audio data information:
-----------------------
Bits per sample: 16
Sample rate: 22050Hz
Size: 19756800

Per channel data:
-----------------
Number of samples: 4966400
Sample size: 2
Frame size: 50 samples
Number of frames: 99328
Frames per quantisation table: 97

Compressing left channel: 99327/99328
Compressing right channel: 99327/99328

Uncompressed size: 19756800 bytes
Compressed size: 8407040 bytes (262144 bytes in qtables, 198656 bytes in scale tables, 7946240 bytes in compressed frames)
thunderball:~ mmendez$ ls -la brut_final_22050_16*
-rw-r--r--  1 mmendez  staff  19798440  7 Sep 13:41 brut_final_22050_16.wav
-rw-r--r--  1 mmendez  staff   8407128  7 Sep 13:45 brut_final_22050_16_ddpcm.tndo
```
 
Now let's take a look at what we did in the ```blur``` example to play music:


We have the two versions of our music in the data directory:

```
thunderball:blur mmendez$ ls -la data/brut*
-rw-r--r--  1 mmendez  staff   966447 23 Jun 15:12 data/brut_blur.mp3
-rw-r--r--  1 mmendez  staff  1185880 18 Aug 15:56 data/brut_ddpcm.tndo
```

We have also set the ```USE_AUDIO``` flag in our demo settings function:

```c
void demoSettings(demoParams *dp) {
  dp->minCPU = MIN_CPU_040;
  dp->tornadoOptions =
      KILL_OS | LOGGING | INSTALL_LEVEL3 | INSTALL_LEVEL2 | USE_AUDIO;
```

And this is our audio init function:

```c
// --------------------------------------------------------------------------
// Music initialisation.
// --------------------------------------------------------------------------
static int *audioSizes;
static int numAudioAssets;
static void **audioAssets;

// SDL/Posix soundtrack
#define bassTrack "data/brut_blur.mp3"

const char *audioList[] = {"data/brut_ddpcm.tndo"};

void demoAudioInit(unsigned int tornadoOptions) {
  if (tornadoOptions & USE_AUDIO) {
    numAudioAssets = sizeof(audioList) / sizeof(char *);
    audioSizes = (int *)tndo_malloc(sizeof(int) * numAudioAssets, 0);
    audioAssets = (void **)tndo_malloc(sizeof(void *) * numAudioAssets, 0);
    if (tornadoOptions & VERBOSE_DEBUGGING) {
      printf("DEBUG - Loading audio data...");
      fflush(stdout);
    }
    if (!loadAssets(&audioAssets[0], &audioList[0], &audioSizes[0],
                    numAudioAssets, tornadoOptions, my_dp)) {
      tndo_memory_shutdown(tornadoOptions);
      if (tornadoOptions & VERBOSE_DEBUGGING) {
        printf("failed!\n");
      }
      exit(1);
    }

    musicSecond = my_dp->sampleRate * (my_dp->bitsPerSample / 8);
    int offset = (effects[currentEffect].minTime / 50) * musicSecond;
    musicBeginL = *my_dp->mixState;
    musicBeginR = *my_dp->mixState2;
    *my_dp->mixState = musicBeginL + offset;
    *my_dp->mixState2 = musicBeginR + offset;

    if (tornadoOptions & VERBOSE_DEBUGGING) {
      printf("done.\n");
    }
  }

#ifndef __AMIGA__
  // Bass initialization.
  if (tornadoOptions & USE_AUDIO) {
    if (!BASS_Init(-1, 44100, 0, 0, 0)) {
      fprintf(stderr, "FATAL - Failed to init bass. Aborting.\n");
      exit(1);
    }

    stream = BASS_StreamCreateFile(0, bassTrack, 0, 0, BASS_STREAM_PRESCAN);
    if (!stream) {
      fprintf(stderr, "FATAL - Failed to open music file <%s>\n", bassTrack);
      exit(1);
    }
  }
#endif
}
```

This function is loading and uncompressing the audio data using the [Asset Manager](AssetManager.md). This happens on the posix/SDL target as well, even though this track will not be used. The reason for this is to make sure that the audio unpacking code path is working correctly.

On posix/SDL only we are also initialising the BASS library replay routine.

We call the audio init routine from the ```demoInit``` function, like this:

```c
#ifdef __AMIGA__
  timeGet(&initBegin);
  timeGet(&initEffectBegin);
#endif

  demoAudioInit(tornadoOptions);

#ifdef __AMIGA__
  timeGet(&initEffectEnd);
  initTime = timeDiffSec(&initEffectBegin, &initEffectEnd);
#endif

  if (tornadoOptions & VERBOSE_DEBUGGING) {
    printf("DEBUG - demoAudioInit() completed in %u seconds\n", initTime);
  }
```

The last thing we need to do is tell the BASS library to start playing the music on the sdl/POSIX target. This happens automagically on Amiga.

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

#ifndef __AMIGA__
  if (tornadoOptions & USE_AUDIO) {
    BASS_Start();
    QWORD pos = BASS_ChannelSeconds2Bytes(stream, epoch / 50);
    BASS_ChannelSetPosition(stream, pos, BASS_POS_BYTE);
    BASS_ChannelPlay(stream, 0);
    if (rocket_control) {
      BASS_ChannelPause(stream);
    }
  }
#endif
```

Using modules and streaming music
------------------------------------------------

It is also possible to use both a module and streaming music in your demo. Capsule's demo [Brutalism](http://www.pouet.net/prod.php?which=81062) has 2 modules and a compressed audio stream for the main part.

In order to switch between audio modes you have to call the ```setAudioMode``` function, declared in ```audio.h```. 

The two settings are:

* ```PCM_REPLAY_MODE``` : Switch to PCM replay and initialise the Paula output routines.
* ```P61_REPLAY_MODE``` : Shut down the Paula output routines and switch to P61 replay mode.

If you were playing an audio stream and want to play a module you switch modes before calling ```p61Init```. In Brutalism, the last effect of the demo loads a p61 module during initialisation and switches to the module when the effect starts, like this:

```c
t_canvas *renderEndscroll(int frame) 
{
    _frame = frame;

    int alpha = up_down_get_alpha (_fades, frame, 2);
    if (alpha >= 0)
        display_endscroll_fade_to (0, alpha);

    if (!music_started && (frame > TIME_MUSIC_START)) 
    {
#ifdef __AMIGA__
        setAudioMode(P61_REPLAY_MODE);
        p61Init(mod_data, 0, 0, 0);
        music_started = 1;
#endif
    }
```

Similarly, to switch from a module to streaming music we would call ```p61End``` and then switch to ```PCM_REPLAY_MODE```.



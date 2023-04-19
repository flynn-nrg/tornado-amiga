/*
Copyright (c) 2019, Miguel Mendez. All rights reserved.

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

#ifndef INCLUDE_TORNADO_SETTINGS_H
#define INCLUDE_TORNADO_SETTINGS_H

#define TORNADO_VERSION "3.0"

// For the minCPU setting.
#define MIN_CPU_020 2
#define MIN_CPU_030 3
#define MIN_CPU_040 4
#define MIN_CPU_060 6

// Screen modes.
#define SCR_NORMAL 1
#define SCR_NORMAL_6BPL 2
#define SCR_16_9 3
#define SCR_16_9_6BPL 4
#define SCR_16_9_4BPL 5
#define SCR_16_9_6_4_BPL 6
#define SCR_DEBUG4 7
#define SCR_16_9_H_4BPL 8
#define SCR_FLOAT 9
#define SCR_16_9_5BPL 10
#define SCR_16_9_H_8BPL 11
#define SCR_16_9_8BPL_PLANAR 12
#define SCR_16_9_H_8BPL_PLANAR 13

// System-friendly screen modes.
#define RTG_NORMAL 31
#define RTG_NORMAL_6BPL 32
#define RTG_16_9 33
#define RTG_16_9_6BPL 34
#define RTG_16_9_4BPL 35
#define RTG_16_9_6_4_BPL 36
#define RTG_DEBUG4 37
#define RTG_16_9_H_4BPL 38
#define RTG_FLOAT 39
#define RTG_16_9_5BPL 40
#define RTG_16_9_H_8BPL 41
#define RTG_16_9_8BPL_PLANAR 42
#define RTG_16_9_H_8BPL_PLANAR 43

// Graphics init options
#define CHUNKY_BUFFERS 1
// Additional planar buffer in fast ram.
#define FAST_PLANAR 1 << 1
// Effects that need g-buffer/z-buffer
#define G_BUFFER 1 << 2
// Generate default grayscale palette.
#define GEN_DEFAULT_PALETTE 1 << 3
// Insert colour commands into copper list.
#define PALETTE_IN_COPPER 1 << 4
// Your code will handle the c2p init and calls.
#define CUSTOM_C2P 1 << 5
// Reuse planar buffers
#define REUSE_PLANAR_BUFFERS 1 << 6

// Create dummy sprites of 64x4
#define DUMMY_SPRITES 1 << 7

// Size of a Gbuffer pixel in bytes.
#define SCR_GBUFFER_PIXEL_SIZE 4

// Kill the system while the demo is running?
#define CLOSE_OS 1

// Install Level3 handler
#define INSTALL_LEVEL3 1 << 1

// Startup banner and system info will be logged to screen.
// Disable for quiet mode.
#define LOGGING 1 << 2

// Enable audio hardware.
#define USE_AUDIO 1 << 3

// -----------------------------------------------------------------------------
// You probably only want these enabled during development. Disable for release.
// -----------------------------------------------------------------------------
// Enable profiling code.
#define PROFILER_ENABLED 1 << 4

// Additional debugging info.
#define VERBOSE_DEBUGGING 1 << 5

// Load assets specified as command line arguments.
#define ASSETS_IN_ARGV 1 << 6

// Log to serial port. You can use serial_port = tcp://127.0.0.1:9999 on fs-uae
// and telnet to localhost 9999 to see the output.
#define LOG_TO_SERIAL 1 << 7

// Memory logging during runtime with file save.
#define MEMORY_LOGGING 1 << 8
#define DEFAULT_MEM_LOG_SIZE 1 * 1024 * 1024
#define DEFAULT_LOG_FILE "T:tornado.log"

// Profile memory usage.
#define MEMORY_PROFILING 1 << 9

// ---------------------------------------------------------------------------
// End of development options.
// ---------------------------------------------------------------------------

// Install keyboard handler.
#define INSTALL_LEVEL2 1 << 10

// Do not decompress ZLIB compressed files, just load the compressed data.
#define NO_Z_DECOMPRESS 1 << 11

// Load assets on reusable memory
#define ASSETS_IN_REUSABLE_MEM 1 << 12

// Enable sprites.
#define USE_SPRITES 1 << 13

// Similar to double buffer planar screens.
#define DOUBLE_BUFFERED_SPRITES 1 << 14

// Show splash screen while loading assets.
#define SHOW_SPLASH 1 << 15

// Flags for interleaved audio.
#define INTERLEAVED_AUDIO 1 << 16

// Dump frame buffer to disk after rendering a frame. Posix only!!!
#define ENABLE_SCREEN_DUMP 1 << 17

// Reload assets on disk change. Posix only!!!
#define HOTSWAPPABLE_ASSETS 1 << 18

// Enable Rocket control. Posix only!!!
#define ENABLE_ROCKET 1 << 19

// Step mode. Bypass the 50Hz timer and render every frame.
#define STEP_MODE 1 << 20

// Double buffered Splash screen
#define DOUBLE_BUFFERED_SPLASH 1 << 21

// Tornado VFS enable.
#define ENABLE_TNDO_VFS 1 << 22

// Generate container script
#define EMIT_CONTAINER_SCRIPT 1 << 23

// DDPCM streaming
#define DDPCM_STREAMING 1 << 24

// DDPCM unpack
#define DDPCM_UNPACK 1 << 25

// -----------------------------------------------------------------------------
// Demo parameter struct.
// -----------------------------------------------------------------------------
typedef struct demoParams {
  unsigned int tornadoOptions;
  unsigned int minCPU;
  unsigned int minFast;
  unsigned int minChip;
  unsigned int fastMemPool;
  unsigned int chipMemPool;
  unsigned int chipScratch;
  unsigned int packedData;
  unsigned int audioPeriod;
  unsigned int audioMode;
  unsigned int sampleRate;
  unsigned int bitsPerSample;
  unsigned int numDisplays;
  const char *tndoContainerPath;
  void (*vfsLoaderCallback)(int loaded, int total);
  char **mixState;
  char **mixState2;
  unsigned int numSamples;
} demoParams;

#endif

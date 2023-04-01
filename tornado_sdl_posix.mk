#################################################################################
# Do NOT modify this makefile directly!!!
#################################################################################

#################################################################################
TORNADO_OBJ =  assets.o sdl_posix/display.o sdl_posix/startup.o sdl_posix/cpu.o
TORNADO_OBJ += sdl_posix/graphics.o sdl_posix/system.o sdl_posix/audio.o sdl_posix/sdl_window.o
TORNADO_OBJ += sdl_posix/chrono.o sdl_posix/splash.o sdl_posix/imgui_overlay.o sdl_posix/sdl_audio.o
TORNADO_OBJ += c2p.o tndo.o debug.o memory.o ddpcm_loader.o
TORNADO_OBJ += telemetry.o prof.o lzw_loader.o lzss_loader.o lzh_loader.o
TORNADO_OBJ += lzw_unpack_stream.o lzss_unpack_stream.o tndo_file.o dprint.o ddpcm_decode.o
TORNADO_OBJ += placeholder.o
TORNADO_SRCDIR = $(TORNADO_BASE)/src

LZW_BASE = $(TORNADO_BASE)/tools/compress
LZW_OBJS = lzw_unpack.o
LZSS_OBJS = lzss_unpack.o
LZH_OBJS = lzh_unpack.o
LZW_INCDIR = $(LZW_BASE)

DDPCM_BASE = $(TORNADO_BASE)/tools/ddpcm
DDPCM_INCDIR = $(DDPCM_BASE)

ROCKET_BASE = $(TORNADO_BASE)/third_party/rocket/lib
ROCKET_INCDIR = $(ROCKET_BASE)
ROCKET_SRCS = $(ROCKET_BASE)/device.c $(ROCKET_BASE)/track.c
ROCKET_OBJS = device.o track.o

IMGUI_BASE = $(TORNADO_BASE)/third_party/imgui
IMGUI_INCDIR = $(IMGUI_BASE)
IMGUI_SOURCES = $(IMGUI_BASE)/backends/imgui_impl_sdl.cpp
IMGUI_SOURCES += $(IMGUI_BASE)/imgui.cpp $(IMGUI_BASE)/imgui_demo.cpp $(IMGUI_BASE)/imgui_draw.cpp $(IMGUI_BASE)/imgui_widgets.cpp $(IMGUI_BASE)/imgui_tables.cpp
IMGUI_OBJS = $(addsuffix .o, $(basename $(notdir $(IMGUI_SOURCES))))

IMGUI_SDL_BASE = $(TORNADO_BASE)/third_party/imgui_sdl
IMGUI_SDL_INCDIR = $(IMGUI_SDL_BASE)
IMGUI_SDL_SOURCES = $(IMGUI_SDL_BASE)/imgui_sdl.cpp
IMGUI_SDL_OBJS = $(addsuffix .o, $(basename $(notdir $(IMGUI_SDL_SOURCES))))

INCDIR = $(TORNADO_BASE)/include
INCDIR += $(LZW_INCDIR)
INCDIR += $(DDPCM_INCDIR)
INCDIR += $(ROCKET_INCDIR)
INCDIR += $(IMGUI_INCDIR)
INCDIR += $(IMGUI_SDL_INCDIR)
INCDIR += $(LOCAL_INCDIR)

#################################################################################

OBJECTS = $(TORNADO_OBJ)
OBJECTS += $(LZW_OBJS)
OBJECTS += $(LZH_OBJS)
OBJECTS += $(LZSS_OBJS)
OBJECTS += $(ROCKET_OBJS)
OBJECTS += $(IMGUI_OBJS)
OBJECTS += $(IMGUI_SDL_OBJS)
OBJECTS += $(DEMO_OBJS)

#################################################################################

TARGET   ?= bin/out.elf
SRCDIR   = src
BUILDDIR ?= build
INCDIR   += include
INCDIR   += ../../libs
LIBDIR   = lib

################################################################################

OBJECTS  := $(OBJECTS:%=$(BUILDDIR)/%)

################################################################################

ifdef LINUX_HOST
CCFLAGS += -D_XOPEN_SOURCE=500
CCFLAGS += -D_POSIX_C_SOURCE=200112L
endif

# One group or the other please...
#
#CCFLAGS += --std=c99
#
CC := cc
CXX := c++

CCFLAGS += -c  
CCFLAGS += --std=c99
CCFLAGS += -O0 
CCFLAGS += -g
CCFLAGS += -fno-omit-frame-pointer
CCFLAGS += -Wall
CCFLAGS += -Wfatal-errors
CCFLAGS += -Wno-deprecated
CCFLAGS += -Wno-unused-variable 
CCFLAGS += -Wno-unused-function
CCFLAGS += -Wno-missing-braces
CCFLAGS += -DUSE_GETADDRINFO
CCFLAGS += -fsanitize=address -fsanitize=undefined

# SDL2 and SDL2_Mixer
CCFLAGS += $(shell pkg-config --cflags SDL2)
CCFLAGS += $(shell pkg-config --cflags SDL2_Mixer)

CXXFLAGS = $(CCFLAGS)
CXXFLAGS += --std=c++14

################################################################################
LDFLAGS := -lc
LDFLAGS += -lm
LDFLAGS += -fno-omit-frame-pointer

# SDL2 and SDL2_Mixer
LDFLAGS += $(shell pkg-config --libs SDL2)
LDFLAGS += $(shell pkg-config --libs SDL2_Mixer)

LDFLAGS += -fsanitize=address -fsanitize=undefined


################################################################################
MKDIR    = @mkdir -p
RM       = rm -rf
QUIET	= @
ECHO	= echo 
################################################################################

all: $(TARGET)


$(TARGET): $(OBJECTS) Makefile
	$(QUIET)$(ECHO) "(LD) -> $@"
	$(QUIET)$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

$(BUILDDIR)/%.o: $(TORNADO_SRCDIR)/%.c Makefile
	$(MKDIR) $(dir $@)
	$(QUIET)$(ECHO) "(CC) -> $@"
	$(QUIET)$(CC) $(addprefix -I,$(INCDIR)) $(addprefix -I,$(ZINCDIR)) $(CCFLAGS) $< -o $@

$(BUILDDIR)/%.o: $(TORNADO_SRCDIR)/%.cpp Makefile
	$(MKDIR) $(dir $@)
	$(QUIET)$(ECHO) "(CXX) -> $@"
	$(QUIET)$(CXX) $(addprefix -I,$(INCDIR)) $(addprefix -I,$(ZINCDIR)) $(CXXFLAGS) $< -o $@

$(BUILDDIR)/lzw_unpack.o: $(LZW_BASE)/lzw_unpack.c Makefile
	$(MKDIR) $(dir $@)
	$(QUIET)$(ECHO) "(CC) -> $@"
	$(QUIET)$(CC) $(addprefix -I,$(INCDIR)) $(addprefix -I,$(LZW_INCDIR)) $(CCFLAGS) $< -o $@

$(BUILDDIR)/lzh_unpack.o: $(LZW_BASE)/lzh_unpack.c Makefile
	$(MKDIR) $(dir $@)
	$(QUIET)$(ECHO) "(CC) -> $@"
	$(QUIET)$(CC) $(addprefix -I,$(INCDIR)) $(addprefix -I,$(LZW_INCDIR)) $(CCFLAGS) $< -o $@

$(BUILDDIR)/lzss_unpack.o: $(LZW_BASE)/lzss_unpack.c Makefile
	$(MKDIR) $(dir $@)
	$(QUIET)$(ECHO) "(CC) -> $@"
	$(QUIET)$(CC) $(addprefix -I,$(INCDIR)) $(addprefix -I,$(LZW_INCDIR)) $(CCFLAGS) $< -o $@

$(BUILDDIR)/device.o: $(ROCKET_BASE)/device.c Makefile
	$(MKDIR) $(dir $@)
	$(QUIET)$(ECHO) "(CC) -> $@"
	$(QUIET)$(CC) $(addprefix -I,$(INCDIR)) $(addprefix -I,$(ROCKET_INCDIR)) $(CCFLAGS) $< -o $@

$(BUILDDIR)/track.o: $(ROCKET_BASE)/track.c Makefile
	$(MKDIR) $(dir $@)
	$(QUIET)$(ECHO) "(CC) -> $@"
	$(QUIET)$(CC) $(addprefix -I,$(INCDIR)) $(addprefix -I,$(ROCKET_INCDIR)) $(CCFLAGS) $< -o $@

$(BUILDDIR)/imgui_impl_sdl.o: $(IMGUI_BASE)/backends/imgui_impl_sdl.cpp Makefile
	$(MKDIR) $(dir $@)
	$(QUIET)$(ECHO) "(CXX) -> $@"
	$(QUIET)$(CXX) $(addprefix -I,$(INCDIR)) $(addprefix -I,$(IMGUI_INCDIR)) $(CXXFLAGS) $< -o $@

$(BUILDDIR)/imgui.o: $(IMGUI_BASE)/imgui.cpp Makefile
	$(MKDIR) $(dir $@)
	$(QUIET)$(ECHO) "(CXX) -> $@"
	$(QUIET)$(CXX) $(addprefix -I,$(INCDIR)) $(addprefix -I,$(IMGUI_INCDIR)) $(CXXFLAGS) $< -o $@

$(BUILDDIR)/imgui_demo.o: $(IMGUI_BASE)/imgui_demo.cpp Makefile
	$(MKDIR) $(dir $@)
	$(QUIET)$(ECHO) "(CXX) -> $@"
	$(QUIET)$(CXX) $(addprefix -I,$(INCDIR)) $(addprefix -I,$(IMGUI_INCDIR)) $(CXXFLAGS) $< -o $@

$(BUILDDIR)/imgui_draw.o: $(IMGUI_BASE)/imgui_draw.cpp Makefile
	$(MKDIR) $(dir $@)
	$(QUIET)$(ECHO) "(CXX) -> $@"
	$(QUIET)$(CXX) $(addprefix -I,$(INCDIR)) $(addprefix -I,$(IMGUI_INCDIR)) $(CXXFLAGS) $< -o $@

$(BUILDDIR)/imgui_widgets.o: $(IMGUI_BASE)/imgui_widgets.cpp Makefile
	$(MKDIR) $(dir $@)
	$(QUIET)$(ECHO) "(CXX) -> $@"
	$(QUIET)$(CXX) $(addprefix -I,$(INCDIR)) $(addprefix -I,$(IMGUI_INCDIR)) $(CXXFLAGS) $< -o $@

$(BUILDDIR)/imgui_tables.o: $(IMGUI_BASE)/imgui_tables.cpp Makefile
	$(MKDIR) $(dir $@)
	$(QUIET)$(ECHO) "(CXX) -> $@"
	$(QUIET)$(CXX) $(addprefix -I,$(INCDIR)) $(addprefix -I,$(IMGUI_INCDIR)) $(CXXFLAGS) $< -o $@

$(BUILDDIR)/imgui_sdl.o: $(IMGUI_SDL_BASE)/imgui_sdl.cpp Makefile
	$(MKDIR) $(dir $@)
	$(QUIET)$(ECHO) "(CXX) -> $@"
	$(QUIET)$(CXX) $(addprefix -I,$(INCDIR)) $(addprefix -I,$(IMGUI_SDL_INCDIR)) $(CXXFLAGS) $< -o $@

$(BUILDDIR)/%.o: $(SRCDIR)/%.c Makefile
	$(MKDIR) $(dir $@)
	$(QUIET)$(ECHO) "(CC) -> $@"
	$(QUIET)$(CC) $(addprefix -I,$(INCDIR)) $(CCFLAGS) $< -o $@

clean:
	$(RM) $(BUILDDIR) $(TARGET)

################################################################################

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

# Zoom
DEMO_OBJS += zoom/zoom.o
DEMO_OBJS += zoom/zoom_util.o
DEMO_OBJS += zoom/amiga/zoom_inner.o

# Save all object and source files to tmp.
BUILDDIR = /tmp/build-amiga

# Remove asserts while compiling.
CCFLAGS += -DNDEBUG

TARGET = ${SHARED}/zoom.68k

# Remove for final release
CCFLAGS += -D__DEBUG_CODE

# Tndo malloc debug.
#CCFLAGS += -DTNDO_MEMORY_DEBUG

#################################################################################
# Do NOT add anything below this line!!!
#################################################################################
include $(TORNADO_BASE)/tornado.mk

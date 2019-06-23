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

# Sprites
DEMO_OBJS += sprites/sprites.o

# Save all object and source files to tmp.
BUILDDIR = /tmp/build-amiga

# Remove asserts while compiling.
CCFLAGS += -DNDEBUG

TARGET = ${SHARED}/sprites.68k

# Remove for final release
CCFLAGS += -D__DEBUG_CODE

# Tndo malloc debug.
#CCFLAGS += -DTNDO_MEMORY_DEBUG

#################################################################################
# Do NOT add anything below this line!!!
#################################################################################
include $(TORNADO_BASE)/tornado.mk

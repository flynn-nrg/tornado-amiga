OBJECTS = main.o quantiser.o wav.o ddpcm.o
TARGET = tndo_ddcpm
#DEBUG = 1

CFLAGS += -I../../include

ifdef DEBUG 
LDFLAGS += -g
CFLAGS += -g -fsanitize=address -fsanitize=undefined
LDFLAGS += -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
else
CFLAGS += -O2
endif

RM = rm -rf

all: $(TARGET)

$(TARGET): $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

clean:
	$(RM) $(OBJECTS) $(TARGET)

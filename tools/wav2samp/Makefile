OBJECTS = wav2samp.o
TARGET = wav2samp
LDFLAGS += -g
CFLAGS += -g -fsanitize=address -fsanitize=undefined
LDFLAGS += -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
RM = rm -rf

all: $(TARGET)

$(TARGET): $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

clean:
	$(RM) $(OBJECTS) $(TARGET)

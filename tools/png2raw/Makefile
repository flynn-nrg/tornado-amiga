OBJECTS = png2raw.o
TARGET = png2raw
LDFLAGS += -g -lz -lpng
CFLAGS += -g -fsanitize=address -fsanitize=undefined -D_XOPEN_SOURCE -std=gnu11
LDFLAGS += -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
RM = rm -rf

all: $(TARGET)

$(TARGET): $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

clean:
	$(RM) $(OBJECTS) $(TARGET)

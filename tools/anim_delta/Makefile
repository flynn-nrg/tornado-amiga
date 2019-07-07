OBJECTS = anim_delta.o
TARGET = anim_delta
LDFLAGS += -lz -lpng
#CFLAGS += -g -fsanitize=address -fsanitize=undefined -std=c99 -D_XOPEN_SOURCE 
#LDFLAGS += -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
RM = rm -rf

all: $(TARGET)

$(TARGET): $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(CFLAGS) $(LDFLAGS) -o $@

clean:
	$(RM) $(OBJECTS) $(TARGET)

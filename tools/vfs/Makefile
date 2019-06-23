TORNADO_INCDIR = ../../include 
CFLAGS += -I ${TORNADO_INCDIR} -g -fsanitize=address -fsanitize=undefined -D_XOPEN_SOURCE -std=c99
LDFLAGS += -g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
OBJECTS = assemble_assets.o
TARGET = assemble_assets
RM = rm -rf

all: $(TARGET)

$(TARGET): $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

clean:
	$(RM) $(OBJECTS) $(TARGET)

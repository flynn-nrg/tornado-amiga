TORNADO_BASE = ../../include/

OBJECTS = tndo_compress.o lzw.o lzss.o lzh.o

CFLAGS += -D_XOPEN_SOURCE -std=gnu11
LDFLAGS += -lz

TEST_LZH_OBJS = test_lzh.o lzh.o lzw.o lzss.o lzh_unpack.o lzss_unpack.o
TEST_LZW_OBJS = test_lzw.o lzw.o lzw_unpack.o
TEST_LZSS_OBJS = test_lzss.o lzss.o lzss_unpack.o

TARGET = tndo_compress

ifndef DEBUG_LZ
CFLAGS += -I$(TORNADO_BASE)
LZH_FLAGS = -O2 $(CFLAGS)
LZSS_FLAGS = -O2 $(CFLAGS)
LZW_FLAGS = -O2 $(CFLAGS)
else
CFLAGS += -g -I$(TORNADO_BASE) -fsanitize=address -fsanitize=undefined
LDFLAGS += -g -fsanitize=address -fsanitize=undefined -fno-omit-frame-pointer
LZH_FLAGS = $(CFLAGS)
LZSS_FLAGS = $(CFLAGS)
LZW_FLAGS = $(CFLAGS)
endif

RM = rm -rf

all: $(TARGET)

$(TARGET): $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

lzw.o:	lzw.c Makefile
	$(CC) $(LZW_FLAGS) -c lzw.c

lzss.o:	lzss.c Makefile
	$(CC) $(LZSS_FLAGS) -c lzss.c

lzh.o: lzh.c Makefile
	$(CC) $(LZH_FLAGS) -c lzh.c

test_lzh: $(TEST_LZH_OBJS) Makefile
	$(CC) $(TEST_LZH_OBJS) $(LDFLAGS) -o $@

test_lzw: $(TEST_LZW_OBJS) Makefile
	$(CC) $(TEST_LZW_OBJS) $(LDFLAGS) -o $@

test_lzss: $(TEST_LZSS_OBJS) Makefile
	$(CC) $(TEST_LZSS_OBJS) $(LDFLAGS) -o $@

clean:
	$(RM) $(OBJECTS) $(TEST_LZW_OBJS) $(TEST_LZH_OBJS) $(TEST_LZSS_OBJS) $(TARGET) test_lzw test_lzss test_lzh

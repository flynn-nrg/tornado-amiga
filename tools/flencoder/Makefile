TORNADO_BASE = ../../
OBJECTS = fixed.o FastQuantizer.o encapsulator.o de-encapsulator.o
TARGET = flencoder
CFLAGS += -I$(TORNADO_BASE)/include
CFLAGS += -O2
LDFLAGS += -lm
RM = rm -rf

all: $(TARGET)

$(TARGET): $(OBJECTS) Makefile
	$(CC) $(CLFAGS) $(OBJECTS) $(LDFLAGS) -o $@

clean:
	$(RM) $(OBJECTS) $(TARGET)

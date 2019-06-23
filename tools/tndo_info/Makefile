TORNADO_INCDIR = ../../include 
OBJECTS = tndo_info.o
TARGET = tndo_info
CFLAGS += -I${TORNADO_INCDIR}
RM = rm -rf

all: $(TARGET)

$(TARGET): $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

clean:
	$(RM) $(OBJECTS) $(TARGET)

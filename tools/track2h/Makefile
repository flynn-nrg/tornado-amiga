OBJECTS = track2h.o
TARGET = track2h
LDFLAGS += -lm
RM = rm -rf

all: $(TARGET)

$(TARGET): $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

clean:
	$(RM) $(OBJECTS) $(TARGET)

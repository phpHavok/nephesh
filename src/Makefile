HEADERS := editor.h
OBJECTS := editor.o
TARGET := nephesh
LDFLAGS := -lcurses
CCFLAGS := -Wall

$(TARGET): main.o $(OBJECTS)
	gcc $(LDFLAGS) -o $@ $^

%.o: %.c $(HEADERS)
	gcc -c -o $@ $<

.PHONY: clean
clean:
	rm -f $(TARGET) *.o

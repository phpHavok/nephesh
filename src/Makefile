HEADERS := editor.h utf8.h scanner.h
OBJECTS := editor.o utf8.o scanner.o
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

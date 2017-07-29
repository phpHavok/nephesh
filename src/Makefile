HEADERS := editor.h utf8.h scanner.h parser.h command.h
OBJECTS := editor.o utf8.o scanner.o parser.o command.o
TARGET := nephesh
LDFLAGS := -lcurses
CCFLAGS := -Wall -D _GNU_SOURCE

$(TARGET): main.o $(OBJECTS)
	gcc $(LDFLAGS) -o $@ $^

%.o: %.c $(HEADERS)
	gcc -c -o $@ $(CCFLAGS) $<

.PHONY: clean
clean:
	rm -f $(TARGET) *.o

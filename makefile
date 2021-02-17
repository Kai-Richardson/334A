
CFLAGS= -g -O2 -Wall
programs = CreateFixedLengthDB ReadAndFindDB

all: $(programs)

$(programs): %: %.c
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -r $(programs)

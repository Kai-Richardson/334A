
CFLAGS=-g
programs = CreateFixedLengthDB ReadAndFindDB

all: $(programs)

$(programs): %: %.c
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -r $(programs)

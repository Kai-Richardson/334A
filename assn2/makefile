
CFLAGS= -g -Wall

all: cs334.o helpers.o
	$(CC) $(CFLAGS) cs334.o helpers.o -o mycs334shell

cs334.o: cs334.c cs334.h
	$(CC) $(CFLAGS) -c cs334.c

helpers.o: helpers.c helpers.h
	$(CC) $(CFLAGS) -c helpers.c

.PHONY: clean
clean:
	rm -r cs334.o mycs334shell helpers.o

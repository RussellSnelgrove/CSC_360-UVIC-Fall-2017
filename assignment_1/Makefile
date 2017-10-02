.phony all:
all: pman

pman: pman.c
	gcc -Wall pman.c -lreadline -lhistory -o PMan

.PHONY clean:
clean:
	-rm -rf *.o *.exe

CC=gcc

default: runfile

runfile.o: main.c
	$(CC) -c main.c -o runfile.o

mips_instruction.o: mips_instruction.c 
	$(CC) -c mips_instruction.c -o mips_instruction.o

runfile: runfile.o mips_instruction.o
	$(CC) runfile.o mips_instruction.o -o runfile

clean:
	rm -f *.o
	rm -f runfile

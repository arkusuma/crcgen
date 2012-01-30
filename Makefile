crcgen.exe: crcgen.c
	gcc -O2 -Wall -o crcgen.exe crcgen.c
	strip -s crcgen.exe

clean:
	del crcgen.exe

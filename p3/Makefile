all:
	gcc -c -Wall -Werror -fpic mem1.c
	gcc -c -Wall -Werror -fpic mem2.c
	gcc -c -Wall -Werror -fpic mem3.c
	gcc -shared -o libmem1.so mem1.o 
	gcc -shared -o libmem2.so mem2.o 
	gcc -shared -o libmem3.so mem3.o 
	gcc -lmem2 -L. -o test test.c -Wall -Werror

clean:
	rm -rf mem1.o mem2.o mem3.o libmem1.so libmem2.so libmem3.so test

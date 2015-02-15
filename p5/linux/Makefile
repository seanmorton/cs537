all:
		ctags -R .
		gcc -c -Wall -Werror -fpic mfs.c udp.c
		gcc -shared -o libmfs.so mfs.o udp.o
		gcc -lmfs -L. -Wall -Werror -o client client.c
		gcc server.c udp.c -o server
		rm images/mfs.img

clean:
		rm client server libmfs.so mfs.o udp.o 

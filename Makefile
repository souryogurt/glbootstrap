CC?=gcc
CFLAGS?=--std=c89 -O0 -g -funsafe-loop-optimizations $(C_NINJA)
glbootstrap:	main_x11.o
	$(CC) $(CFLAGS) -L/usr/local/lib -o glbootstrap main_x11.o -lX11 -lGL
main_x11.o:	src/main_x11.c
	$(CC) $(CFLAGS) -c -I/usr/local/include -I./inc src/main_x11.c

clean:	
	rm ./glbootstrap ./main_x11.o

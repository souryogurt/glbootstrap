CC?=gcc
DEFS?=-DHAVE_CONFIG_H
CFLAGS?=--std=c89 -O0 -g -funsafe-loop-optimizations $(C_NINJA)
glbootstrap:	main_x11.o ugl_glx.o
	$(CC) $(CFLAGS) -Wl,--as-needed -L/usr/local/lib -o glbootstrap main_x11.o ugl_glx.o \
		-lX11 -lGL
main_x11.o:	src/main_x11.c
	$(CC) $(DEFS) $(CFLAGS) -c -I/usr/local/include -I./inc src/main_x11.c

ugl_glx.o:	inc/ugl.h src/ugl_glx.c
	$(CC) $(DEFS) $(CFLAGS) -c -I/usr/local/include -I./inc src/ugl_glx.c

clean:	
	rm ./glbootstrap ./main_x11.o ./ugl_glx.o

oclint:		inc/ugl.h src/ugl_glx.c src/main_x11.c
	oclint --enable-global-analysis src/main_x11.c src/ugl_glx.c -- -c $(DEFS) -I./inc

tags:
	ctags -R

astyle:
	astyle --options=./astylerc src/*.c inc/*.h

cppcheck:
	cppcheck -q --template=gcc --std=c89 --enable=all $(DEFS) -I./inc src/*.c

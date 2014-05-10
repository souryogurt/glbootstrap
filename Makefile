CC?=gcc
DEFS?=-DHAVE_CONFIG_H
CFLAGS?=--std=c89 -O0 -g -funsafe-loop-optimizations $(C_NINJA)

glbootstrap:	main_x11.o  egl_glx.o
	$(CC) $(CFLAGS) -L/usr/local/lib -o glbootstrap main_x11.o egl_glx.o -lX11 -lGL

main_x11.o:	src/main_x11.c
	$(CC) $(DEFS) $(CFLAGS) -c -I/usr/local/include -I./inc src/main_x11.c

egl_glx.o:	src/egl_glx.c
	$(CC) $(DEFS) $(CFLAGS) -c -I/usr/local/include -I./inc src/egl_glx.c

clean:	
	rm ./glbootstrap ./main_x11.o ./egl_glx.o

oclint:		src/main_x11.c src/egl_glx.c
	oclint --enable-global-analysis src/main_x11.c src/egl_glx.c -- -c $(DEFS) -I./inc

tags:
	ctags -R

astyle:
	astyle --options=./astylerc src/main_x11.c src/egl_glx.c

cppcheck:
	cppcheck -q --template=gcc --std=c89 --enable=all $(DEFS) -I./inc src/main_x11.c src/egl_glx.c

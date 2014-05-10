CC?=gcc
DEFS?=-DHAVE_CONFIG_H
CFLAGS?=--std=c89 -O0 -g -funsafe-loop-optimizations $(C_NINJA)

glbootstrap:	main_x11.o 
	$(CC) $(CFLAGS) -L/usr/local/lib -o glbootstrap main_x11.o -lX11 -lEGL -lGL

main_x11.o:	src/main_x11.c
	$(CC) $(DEFS) $(CFLAGS) -c -I/usr/local/include -I./inc src/main_x11.c

clean:	
	rm ./glbootstrap ./main_x11.o

oclint:		src/main_x11.c
	oclint --enable-global-analysis src/main_x11.c -- -c $(DEFS) -I./inc

tags:
	ctags -R

astyle:
	astyle --options=./astylerc src/main_x11.c

cppcheck:
	cppcheck -q --template=gcc --std=c89 --enable=all $(DEFS) -I./inc src/main_x11.c

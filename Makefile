glbootstrap:	main_x11.o
	gcc --std=c89 -O0 -g $(C_NINJA) -o glbootstrap main_x11.o -lX11 -lGL
main_x11.o:	X11/main_x11.c
	gcc --std=c89 -O0 -g $(C_NINJA) -c -I./inc X11/main_x11.c

clean:	
	rm ./glbootstrap ./main_x11.o

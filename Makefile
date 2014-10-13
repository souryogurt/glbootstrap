CC?=gcc
CFLAGS?=-O0 -g --std=c89 -funsafe-loop-optimizations $(C_NINJA)
CPPFLAGS=-DHAVE_CONFIG_H -I./inc -I./eglproxy/inc -I/usr/local/include
LDFLAGS=-L./eglproxy -L/usr/local/lib
LDLIBS=-leglproxy -lX11 -lGL
RM?=rm -f

SOURCES = src/main_x11.c
OBJECTS = $(SOURCES:.c=.o)

glbootstrap: $(OBJECTS) eglproxy.a
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

eglproxy.a:
	$(MAKE) -C eglproxy

.PHONY : clean distclean oclint tags astyle cppcheck
distclean: clean
	$(MAKE) -C eglproxy clean

clean:	
	$(RM) ./glbootstrap $(OBJECTS)

oclint:
	oclint --enable-global-analysis $(SOURCES) -- -c $(CPPFLAGS)

tags:
	ctags -R

astyle:
	astyle --options=./astylerc $(SOURCES) src/main_win32.c

cppcheck:
	cppcheck -q --template=gcc --std=c89 --enable=all $(CPPFLAGS) -D__unix__ -I/usr/include $(SOURCES)

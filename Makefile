CC?=gcc
CFLAGS?=-O0 -g --std=c89 -funsafe-loop-optimizations $(C_NINJA)
CPPFLAGS=-DHAVE_CONFIG_H -I./inc -I/usr/local/include
LDFLAGS=-L/usr/local/lib
LDLIBS=-lX11 -lGL
RM?=rm -f

SOURCES = src/main_x11.c src/egl_glx.c
OBJECTS = $(SOURCES:.c=.o)

glbootstrap: $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $@

$(OBJECTS): inc/config.h inc/EGL/egl.h inc/EGL/eglext.h inc/EGL/eglplatform.h \
	inc/KHR/khrplatform.h

.c.o:
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

.PHONY : clean oclint tags astyle cppcheck
clean:	
	$(RM) ./glbootstrap $(OBJECTS)

oclint:
	oclint --enable-global-analysis $(SOURCES) -- -c $(CPPFLAGS)

tags:
	ctags -R

astyle:
	astyle --options=./astylerc $(SOURCES)

cppcheck:
	cppcheck -q --template=gcc --std=c89 --enable=all $(CPPFLAGS) -D__unix__ -I/usr/include $(SOURCES)

CC=gcc
COPTS=-g -std=gnu99 -Wall

all: wineGlassTexture

JUNK=*.o *~ core *.dSYM

clean:
  -rm -rf $(JUNK)

clobber:
	-rm -rf $(JUNK) $(ALL)

ifeq "$(shell uname)" "Darwin"
LIBS=-framework GLUT -framework OpenGL -framework Cocoa
else
LIBS=-L/usr/X11R6/lib -lglut -lGLU -lGL -lXext -lX11 -lm
endif

wineGlassTexture: wineGlassTexture.o matrix.o glass.o
	$(CC) $(COPTS) $^ -o $@ $(LIBS)

.c.o:
	$(CC) -c $(COPTS) $<

wineGlassTexture.o: wineGlassTexture.c matrix.h glass.h
matrix.o: matrix.c matrix.h
glass.o: glass.c glass.h

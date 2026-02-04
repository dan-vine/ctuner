################################################################################
#
#  Makefile for Tuner
#
################################################################################

GCC	= gcc
GXX     = g++
WINDRES = windres

all:	Tuner.exe

Tuner.exe:	Tuner.cpp fft.cpp drawing.cpp audio.cpp Tuner.o

Tuner.o:	Tuner.rc Tuner.h fft.h drawing.h audio.h Tuner.manifest Tuner.ico Toolbar.bmp \
		Temperaments.h Makefile

clean:
	rm *.exe *.o

%.exe:	%.cpp
	$(GXX) -o $@ $^ -Os -std=gnu++11 -static \
	-mwindows -lgdiplus -lcomctl32 -lwinmm -lmsimg32

%.o:	%.rc
	$(WINDRES) -o $@ $<

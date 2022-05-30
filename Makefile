TARGET      ?= hocoslamfy-od

ifeq ($(platform), od)
  TARGET    := hocoslamfy-od
  CC        := mipsel-linux-gcc
  STRIP     := mipsel-linux-strip
  OBJS       = platform/opendingux.o
  DEFS      := -DOPK
else ifeq ($(platform), miyoomini)
  TARGET    := hocoslamfy
  CC        := arm-linux-gnueabihf-gcc
  STRIP     := arm-linux-gnueabihf-strip
  OBJS       = platform/opendingux.o
  DEFS      := -DNO_SHAKE -DUSE_HOME -DMIYOOMINI 
else ifeq ($(platform), sdl2)
  TARGET    := hocoslamfy
  CC        := gcc
  STRIP     := strip
  OBJS       = platform/opendingux.o
  DEFS      := -DNO_SHAKE -DUSE_HOME -DSDL2

else
  TARGET    := hocoslamfy
  CC        := gcc
  STRIP     := strip
  OBJS       = platform/general.o
  DEFS      := 
endif

SYSROOT     := $(shell $(CC) --print-sysroot)
ifeq ($(platform), sdl2)
SDL_CONFIG  ?= sdl2-config
else
SDL_CONFIG  ?= $(SYSROOT)/usr/bin/sdl-config
endif
SDL_CFLAGS  := $(shell $(SDL_CONFIG) --cflags)
SDL_LIBS    := $(shell $(SDL_CONFIG) --libs)

OBJS        += main.o init.o title.o game.o score.o audio.o bg.o text.o unifont.o
              
HEADERS     += main.h init.h platform.h title.h game.h score.h audio.h bg.h text.h unifont.h

INCLUDE     := -I.
DEFS        +=

CFLAGS       = $(SDL_CFLAGS) -Wall -Wno-unused-variable \
               -O2 -fomit-frame-pointer $(DEFS) $(INCLUDE)
ifeq ($(platform), sdl2)
LDFLAGS     := $(SDL_LIBS) -lm -lSDL2_image -lSDL2_mixer 
else			   
LDFLAGS     := $(SDL_LIBS) -lm -lSDL_image -lSDL_mixer 
endif

#-lshake
ifneq (, $(findstring MINGW, $(shell uname -s)))
	CFLAGS+=-DDONT_USE_PWD
endif

include Makefile.rules

.PHONY: all opk

all: $(TARGET)

$(TARGET): $(OBJS)

opk: $(TARGET).opk

$(TARGET).opk: $(TARGET)
	$(SUM) "  OPK     $@"
	$(CMD)rm -rf .opk_data
	$(CMD)cp -r data .opk_data
	$(CMD)cp COPYRIGHT .opk_data/COPYRIGHT
	$(CMD)cp $< .opk_data/$(TARGET)
	$(CMD)$(STRIP) .opk_data/$(TARGET)
	$(CMD)mksquashfs .opk_data $@ -all-root -noappend -no-exports -no-xattrs -no-progress >/dev/null

# The two below declarations ensure that editing a .c file recompiles only that
# file, but editing a .h file recompiles everything.
# Courtesy of Maarten ter Huurne.

# Each object file depends on its corresponding source file.
$(C_OBJS): %.o: %.c

# Object files all depend on all the headers.
$(OBJS): $(HEADERS)

# -------------------------------------------------------------------------------------------------------------
# -- build options
# -------------------------------------------------------------------------------------------------------------

BINARY = cgamex86

SRC_FILES = $(shell ls *.c)
HDR_FILES = $(shell ls *.h)

GIT_VERSION=$(shell git log -1 --pretty=%h)

DATE=$(shell date +%F)
HOUR=$(shell date +%H)
MIN=$(shell date +%M)
COMPILE_DATE="$(DATE) $(HOUR):$(MIN) CET"
CC=gcc

BASE_CFLAGS=-pipe -Wall -pedantic

BROOT=linux
BR=$(BROOT)/release
BD=$(BROOT)/debug

OBJR_Q3A=$(SRC_FILES:%.c=$(BR)$(B_Q3A)/%.o)
OBJD_Q3A=$(SRC_FILES:%.c=$(BD)$(B_Q3A)/%.o)

DEBUG_DEFINES=-DGIT_VERSION=\"$(GIT_VERSION)\" -DCOMPILE_DATE=\"$(COMPILE_DATE)\"
RELEASE_DEFINES=-DGIT_VERSION=\"$(GIT_VERSION)\" -DCOMPILE_DATE=\"$(COMPILE_DATE)\"

DEBUG_CFLAGS=$(BASE_CFLAGS) -ggdb -g -pg $(DEBUG_DEFINES) -I/usr/include/libxml2
RELEASE_CFLAGS=$(BASE_CFLAGS) -Os $(RELEASE_DEFINES) -I/usr/include/libxml2

# if we on 64bit system...

ifeq ($(shell uname -m),x86_64)
    DEBUG_DEFINES += -m32
    RELEASE_DEFINES += -m32
endif


SHLIBCFLAGS=-fPIC -std=c99
SHLIBLDFLAGS=-Wl,--no-undefined -shared
EXTLIBS = -lm

# -------------------------------------------------------------------------------------------------------------
# -- build rules
# -------------------------------------------------------------------------------------------------------------

release: banner $(BR)/$(BINARY).dll
debug: banner $(BD)/$(BINARY).dll
all: banner release




# -------------------------------------------------------------------------------------------------------------
# -- optional build
# -------------------------------------------------------------------------------------------------------------

banner:
	@printf "\n"
	@printf "\033\13301;31mmDd \033\13301;33mRS Makefile  (rev-$(GIT_VERSION)) (%s)\n" $(COMPILE_DATE)
	@printf "\n\033\13301;32m----------------------------------------------------------\n\033\13300;39m"

clean:
	@rm -rf $(BROOT)




# -------------------------------------------------------------------------------------------------------------
# -- release build
# -------------------------------------------------------------------------------------------------------------

$(BR)/$(BINARY).dll: $(BR) $(OBJR_Q3A)
	@printf "\n"
	@printf "\033\13301;32m->\033\13301;37m linking to $(BR)$(B_Q3A)/$(BINARY).dll\n\033\13300;39m"
	@$(CC) $(RELEASE_CFLAGS) $(SHLIBLDFLAGS) -o $@ $(OBJR_Q3A)
	@printf "\033\13301;32m->\033\13301;37m stripping $(BR)$(B_Q3A)/$(BINARY).dll\n\033\13300;39m"
	strip $(BR)/$(BINARY).dll

$(BR)/%.o: %.c $(HDR_FILES)
	@printf "R \033\13301;33m-\033\13301;37m %-20s %s\033\13300;39m\n" $< $@
	@$(CC) $(INCLUDES) $(RELEASE_CFLAGS) $(RELEASE_DEFINES) $(SHLIBCFLAGS) -o $@  -c $<

$(BR):
	@if [ ! -d $(BROOT) ];then mkdir $(BROOT);fi
	@if [ ! -d $(@) ];then mkdir $@;fi



# -------------------------------------------------------------------------------------------------------------
# -- debug build
# -------------------------------------------------------------------------------------------------------------

$(BD)/$(BINARY).dll: $(BD) $(OBJD_Q3A)
	@printf "\n"
	@printf "\033\13301;32m->\033\13301;37m linking to $(BD)/$(BINARY).dll\n\033\13300;39m"
	@$(CC) $(DEBUG_CFLAGS) $(SHLIBLDFLAGS) -o $@ $(EXTLIBS) $(OBJD_Q3A)


$(BD)/%.o: %.c $(HDR_FILES)
	@printf "D \033\13301;33m-\033\13301;37m %-20s %s\033\13300;39m\n" $< $@
	@$(CC) $(INCLUDES) $(DEBUG_DEFINES) $(SHLIBCFLAGS) -o $@ -c $<

$(BD):
	@if [ ! -d $(BROOT) ];then mkdir $(BROOT);fi
	@if [ ! -d $(@) ];then mkdir $@;fi


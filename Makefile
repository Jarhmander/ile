#
#
#

CC      := gcc
LD      := gcc
CFLAGS  += -g3 -Wall
LDFLAGS += -lm
INCLUDES+= -Ilua-5.3.0-work2/src/ \
           -Ilpeg-0.12/

#######

LUASRC := lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c \
          lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c \
          ltm.c lundump.c lvm.c lzio.c \
          lauxlib.c lbaselib.c lbitlib.c lcorolib.c ldblib.c liolib.c \
          lmathlib.c loslib.c lstrlib.c ltablib.c lutf8lib.c loadlib.c linit.c

LPEGSRC:=   lpcap.c lpcode.c lpprint.c lptree.c lpvm.c 

LUADIR :=  lua-5.3.0-work2/src/
LPEGDIR:=  lpeg-0.12/
SRC    :=  $(LUASRC) \
           $(LPEGSRC) \
           main.c

OBJDIR = .obj
OBJS   = $(addprefix $(OBJDIR)/,$(SRC:.c=.o))

VPATH  += $(LUADIR):$(LPEGDIR):. 

########


ile : $(OBJS)
	@echo Linking...
	$(LD) -o ile $(LDFLAGS) $(OBJS)

$(OBJDIR)/%.o : %.c
	$(CC) -c $< -o $@ $(CFLAGS) $(INCLUDES)

$(OBJS) : | $(OBJDIR)

$(OBJDIR):
	-mkdir -p $(OBJDIR)

$(OBJS) : Makefile

.PHONY: clean

clean: 
	-rm -r $(OBJDIR) ile 2> /dev/null

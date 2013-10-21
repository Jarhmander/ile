#
#
#

CC      := gcc
LD      := gcc
CFLAGS  += -g
LDFLAGS += -lm
INCLUDES+= -Ilua-5.2.2/src/ \
           -Ilpeg-0.12/

#######

LUASRC :=   lapi.c lauxlib.c lbaselib.c lbitlib.c lcode.c lcorolib.c lctype.c \
            ldblib.c ldebug.c ldo.c ldump.c lfunc.c lgc.c linit.c liolib.c \
            llex.c lmathlib.c lmem.c loadlib.c lobject.c lopcodes.c loslib.c \
            lparser.c lstate.c lstring.c lstrlib.c ltable.c ltablib.c ltm.c \
            lundump.c lvm.c lzio.c 

LPEGSRC:=   lpcap.c lpcode.c lpprint.c lptree.c lpvm.c 

LUADIR :=  lua-5.2.2/src/
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

.PHONY: clean

clean: 
	-rm -r $(OBJDIR) ile 2> /dev/null

#
#
#

TARGET :=  ile
LUADIR :=  lua/src/
LPEGDIR:=  modules/lpeg-0.12/

CC      := gcc
LD      := gcc
CFLAGS  += -g3 -Wall
LDFLAGS += -lm
INCLUDES+= -I$(LUADIR) \
           -I$(LPEGDIR)

#######

.PHONY: all clean tags

LUASRC := lapi.c lcode.c lctype.c ldebug.c ldo.c ldump.c lfunc.c lgc.c llex.c \
          lmem.c lobject.c lopcodes.c lparser.c lstate.c lstring.c ltable.c \
          ltm.c lundump.c lvm.c lzio.c \
          lauxlib.c lbaselib.c lbitlib.c lcorolib.c ldblib.c liolib.c \
          lmathlib.c loslib.c lstrlib.c ltablib.c lutf8lib.c loadlib.c linit.c \
          lua.c

LPEGSRC:=   lpcap.c lpcode.c lpprint.c lptree.c lpvm.c

SRC    :=  $(LUASRC) \
           $(LPEGSRC)

OBJDIR := .obj
OBJS   := $(addprefix $(OBJDIR)/,$(SRC:.c=.o))
DEPS   := $(OBJS:.o=.dep)

VPATH  += $(LUADIR):$(LPEGDIR):.

########

all : $(TARGET)

$(TARGET) : $(OBJS)
	@echo Linking...
	$(LD) -o ile $(LDFLAGS) $(OBJS)

$(OBJDIR)/%.o : %.c
	$(CC) -c $< -o $@ -MD -MP -MF $(@:$(suffix $@)=.dep) $(CFLAGS) $(INCLUDES)

$(OBJS) : | $(OBJDIR)

$(OBJDIR):
	-mkdir -p $(OBJDIR)

$(OBJS) : Makefile


clean:
	rm -rf $(OBJDIR) $(TARGET)

tags:
	rm -f tags
	find * -name '*.[ch]' -print -exec ctags -a {} \;

-include $(DEPS)

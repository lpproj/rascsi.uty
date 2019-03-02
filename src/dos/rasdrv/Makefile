#
#
#

RM=rm
CP=cp
O=obj

#ASFLAGS_DEBUG = -DDEBUG -l $*.lst
LDFLAGS_DEBUG = op map

ASFLAGS = $(ASFLAGS_DEBUG)
LDFLAGS =  op quiet $(LDFLAGS_DEBUG) op stack=256 op caseexact op nodefaultlibs

.SUFFIXES:
.SUFFIXES: .sys .com .exe .$(O) .asm

.asm.$(O):
	nasm -f obj $(ASFLAGS) -o $@ $<


OBJS = rd_dvhdr.$(O) rd_int2f.$(O) rd_init.$(O) rd_misc.$(O) rd_funcs.$(O) rd_msg.$(O) rd_fsdbg.$(O)

all: rasdrv55.exe rasdrvas.exe

clean: .SYMBOLIC
	-$(RM) rasdrv55.exe
	-$(RM) *.$(O) *.map *.lst

rasdrv55.exe: $(OBJS) rd_scsi5.$(O)
	wlink name $@ format dos $(LDFLAGS) file { $(OBJS) rd_scsi5.$(O) }

rasdrvas.exe: $(OBJS) rd_scsia.$(O)
	wlink name $@ format dos $(LDFLAGS) file { $(OBJS) rd_scsia.$(O) }


rd_scsi5.$(O): rd_scsi.asm scsi_55.asm
	nasm -f obj $(ASFLAGS) -DSCSI_55BIOS -o $@ rd_scsi.asm

rd_scsia.$(O): rd_scsi.asm scsi_as.asm
	nasm -f obj $(ASFLAGS) -DSCSI_ASPI -o $@ rd_scsi.asm



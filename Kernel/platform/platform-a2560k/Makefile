
# see sys_general.h for model number, 8 == A2560X
# MODEL_FOENIX_A2560X         8 (GENX)
# MODEL_FOENIX_A2560U         9
# MODEL_FOENIX_A2560K         11
# MODEL_FOENIX_A2560M         12
# MODEL_FOENIX_FA2560K2       13
MODEL ?= 11

CSRCS  = devtty.c devrtc.c
CSRCS += devices.c main.c libc.c
CSRCS += screen.c devgfx.c
CSRCS += vt.c
CSRCS += devinput.c timers.c interrupt.c
CSRCS += superio.c ps2.c
CSRCS += a2560x_sdcard_driver.c
#CSRCS += devrd.c
COBJS = $(CSRCS:.c=$(BINEXT))

ASRCS = p68000.S crt0.S
ASRCS += tricks.S
AOBJS = $(ASRCS:.S=.o)

DSRCS =  ../../dev/devide.c ../../dev/mbr.c
DSRCS += ../../dev/blkdev.c ../../dev/devide_discard.c 
DSRCS += ../../dev/devsd.c ../../dev/devsd_discard.c
DSRCS += ../../dev/bq4845.c ../../dev/ps2kbd.c
#DSRCS += ../../dev/devrd.c
DOBJS = $(patsubst ../../dev/%.c,%.o, $(DSRCS))

LSRCS = ../../lib/68000exception.c ../../lib/68000usercopy.c ../../lib/68000relocate.c
LOBJS = $(patsubst ../../lib/%.c,%.o, $(LSRCS))

OBJS  = $(COBJS) $(AOBJS) $(DOBJS) $(LOBJS)

CROSS_CCOPTS += -I../../dev/ -I./include/ -DMODEL=$(MODEL)

all:	$(OBJS)

$(COBJS): %.o: %.c
	$(CROSS_CC) $(CROSS_CCOPTS) -c $<

$(AOBJS): %.o: %.S
	$(CROSS_AS) $(ASOPTS) $< -o $*.o

$(DOBJS): %.o: ../../dev/%.c
	$(CROSS_CC) $(CROSS_CCOPTS) -c $<

$(LOBJS): %.o: ../../lib/%.c
	$(CROSS_CC) $(CROSS_CCOPTS) -c $<

tricks.S: ../../lib/68000flat.S

clean:
	rm -f *.o fuzix.elf loader.elf loader.map loader.bin core *~ 
	rm -f fuzix.rom fuzix.s68

image:
	$(CROSS_LD) -M -o fuzix.elf -T fuzix.ld \
	$(LOBJS) $(COBJS) $(DOBJS) \
	p68000.o tricks.o ../../start.o ../../version.o ../../cpu-68000/lowlevel-68000.o \
	../../timer.o ../../kdata.o \
	../../devio.o ../../filesys.o ../../process.o ../../inode.o ../../syscall_fs.o \
	../../syscall_proc.o ../../syscall_other.o ../../mm.o ../../flat.o \
	../../blk512.o ../../tty.o ../../devsys.o ../../usermem.o ../../syscall_fs2.o \
	../../syscall_fs3.o ../../syscall_exec32.o ../../syscall_exec.o ../../font8x8.o \
	../../vt.o ../../devinput.o \
	../../cpu-68000/usermem_std-68000.o ../../malloc.o \
	../../level2.o ../../syscall_level2.o ../../select.o > ../../fuzix.map
	$(CROSS_COMPILE)objcopy fuzix.elf -O binary ../../fuzix.bin
	dd if=../../fuzix.bin of=fuzix.rom bs=65536 conv=sync
	srec_cat fuzix.rom -binary -offset 0 -o fuzix.s68 -Motorola

IMAGES = $(FUZIX_ROOT)/Images/$(TARGET)

diskimage:
	# Make a blank disk image with partition
	dd if=$(FUZIX_ROOT)/Standalone/filesystem-src/parttab.40M of=$(IMAGES)/disk.img bs=40017920 conv=sync
	# Add the file system
	dd if=$(IMAGES)/filesys.img of=$(IMAGES)/disk.img bs=512 seek=2048 conv=notrunc
	# Make an emulator image of it
	cat $(FUZIX_ROOT)/Standalone/filesystem-src/idehdr.40M $(IMAGES)/disk.img > $(IMAGES)/emu-ide.img
	cp fuzix.rom fuzix.s68 $(IMAGES)

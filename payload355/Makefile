PPU_CC = ppu-gcc
PPU_OBJCOPY = ppu-objcopy

all: payload_syscall36.bin payload_sky.bin

%.o : %.S
	$(PPU_CC) -c $< -o $@

%.bin : %.o
	$(PPU_OBJCOPY) -O binary $< $@
	cp $@ ../data355/$@

clean:
	rm -f *.o *.bin

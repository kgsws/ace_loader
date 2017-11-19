PROGRAM := ace
LLVM_POSTFIX := -5.0
LD := ld.lld$(LLVM_POSTFIX)
CC := clang$(LLVM_POSTFIX)
AS := llvm-mc$(LLVM_POSTFIX)
LD_FLAGS := -Bsymbolic --shared --emit-relocs --no-gc-sections --no-undefined -T ../../link.T
CC_FLAGS := -g -fPIC -ffreestanding -fexceptions -target aarch64-none-linux-gnu -O0 -mtune=cortex-a53 -I ../../include/ -I ../../newlib/newlib/libc/include/ -I ../../newlib/newlib/libc/sys/switch/include/ -Wall
AS_FLAGS := -arch=aarch64 -triple aarch64-none-switch
PYTHON2 := python2
MEPHISTO := ~/programy/Mephisto/ctu
# all compiled files
OBJ := main.o asm.o http.o

.SUFFIXES: # disable built-in rules

all: $(PROGRAM).nro

test: $(PROGRAM).nro
	$(MEPHISTO) --enable-sockets --load-nro $<

%.o: %.c
	$(CC) $(CC_FLAGS) -c -o $@ $<

%.o: %.S
	mkdir -p $(@D)
	$(AS) $(AS_FLAGS) $< -filetype=obj -o $@

$(PROGRAM).nro: $(PROGRAM).nro.so
	$(PYTHON2) ../../tools/elf2nxo.py $< $@ nro

$(PROGRAM).nso: $(PROGRAM).nso.so
	$(PYTHON2) ../../tools/elf2nxo.py $< $@ nso

$(PROGRAM).nro.so: ${OBJ}
	$(LD) $(LD_FLAGS) -o $@ ${OBJ} --whole-archive ../../build/lib/libtransistor.nro.a --no-whole-archive ../../newlib/aarch64-none-switch/newlib/libc.a

$(PROGRAM).nso.so: ${OBJ}
	$(LD) $(LD_FLAGS) -o $@ ${OBJ} --whole-archive ../../build/lib/libtransistor.nso.a --no-whole-archive ../../newlib/aarch64-none-switch/newlib/libc.a

clean:
	rm -rf *.o *.nso *.nro *.so

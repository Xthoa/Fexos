OBJS_BOOTPACK = bootpack.obj naskfunc.obj hankaku.obj
TOOLPATH =
INCPATH  = C:\Users\admin\Desktop\Programs\GrouOS9\30天自制操作系统\tolset\z_tools/haribote/

app = test
include appbin.txt

MAKE     = $(TOOLPATH)make.exe -r
NASK     = $(TOOLPATH)nask.exe
CC1      = $(TOOLPATH)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
MAKEFONT = $(TOOLPATH)makefont.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
RULEFILE = $(TOOLPATH)haribote.rul
EDIMG    = $(TOOLPATH)edimg.exe
IMGTOL   = $(TOOLPATH)imgtol.com
COPY     = copy
DEL      = del

# 鍍ㄥ兗鍍呭剸鍍╂懄宥?

default :
	$(MAKE) img

# 鍍煎偨鍍�鍎栨儣鎯夊┋鎳?

ipl10.bin : ipl10.nas Makefile
	$(NASK) ipl10.nas ipl10.bin ipl10.lst

asmhead.bin : asmhead.nas Makefile
	$(NASK) asmhead.nas asmhead.bin asmhead.lst

hankaku.bin : hankaku.txt Makefile
	$(MAKEFONT) hankaku.txt hankaku.bin

hankaku.obj : hankaku.bin Makefile
	$(BIN2OBJ) hankaku.bin hankaku.obj _hankaku

bootpack.bim : $(OBJS_BOOTPACK) Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3200k map:bootpack.map \
		$(OBJS_BOOTPACK)
# 3MB+64KB=3136KB

bootpack.hrb : bootpack.bim Makefile
	$(BIM2HRB) bootpack.bim bootpack.hrb 0

haribote.sys : asmhead.bin bootpack.hrb Makefile
	copy /B asmhead.bin+bootpack.hrb haribote.sys

fs.bin : test.bin t.txt rule.txt makefile
	fexfs rule.txt fs.bin

fexos.img : ipl10.bin haribote.sys fs.bin Makefile
	$(EDIMG)   imgin:C:\Users\admin\Desktop\Programs\GrouOS9\30天自制操作系统\tolset\z_tools/fdimg0at.tek \
		wbinimg src:ipl10.bin len:512 from:0 to:0 \
		copy from:haribote.sys to:@: \
		copy from:fs.bin to:@: \
		imgout:fexos.img

# 鍫︽枓濠噧

%.gas : %.c bootpack.h Makefile
	$(CC1) -o $*.gas $*.c

%.nas : %.gas Makefile
	$(GAS2NASK) $*.gas $*.nas

%.obj : %.nas Makefile
	$(NASK) $*.nas $*.obj $*.lst

# 鍍愬剠鍎炲儶

img :
	$(MAKE) fexos.img

run :
	$(MAKE) img
	$(COPY) fexos.img C:\Users\admin\Desktop\Programs\GrouOS9\30天自制操作系统\tolset\z_tools\qemu\fdimage0.bin
	$(MAKE) -C C:\Users\admin\Desktop\Programs\GrouOS9\30天自制操作系统\tolset\z_tools/qemu

install :
	$(MAKE) img
	$(IMGTOL) w a: haribote.img

clean :
	-$(DEL) *.bin
	-$(DEL) *.lst
	-$(DEL) *.obj
	-$(DEL) bootpack.map
	-$(DEL) bootpack.bim
	-$(DEL) bootpack.hrb
	-$(DEL) haribote.sys

src_only :
	$(MAKE) clean
	-$(DEL) haribote.img

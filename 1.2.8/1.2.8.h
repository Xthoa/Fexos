struct BOOTINFO { /* 0x0ff0-0x0fff */
	char cyls; /* ブートセクタはどこまでディスクを読んだのか */
	char leds; /* ブート時のキーボードのLEDの状態 */
	char vmode; /* ビデオモード  何ビットカラーか */
	char reserve;
	short scrnx, scrny; /* 画面解像度 */
	char *vram;
};
#define ADR_BOOTINFO	0x00000ff0
void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
int io_in8(int port);
void io_out8(int port, int data);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
void load_tr(int tr);
int load_cr0(void);
void store_cr0(int cr0);
void asm_inthandler20(void);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
unsigned int memtest_sub(unsigned int start, unsigned int end);
struct FIFO32 {
	int *buf;//4
	int p, q, size, free, flags;//p:put q:get//20
	struct TASK* task;//for task reactivate//4
};//28
typedef struct FIFO32 Fifo,*HFIFO,*Hfifo;
void fifo32_init(struct FIFO32 *fifo, int size, int *buf,struct TASK* task);
int fifo32_put(struct FIFO32 *fifo, int data);
int fifo32_get(struct FIFO32 *fifo);
int fifo32_status(struct FIFO32 *fifo);
void init_palette(void);
void set_palette(int start, int end, unsigned char *rgb);
void memfill(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);
void drawbg(char *vram, int x, int y);
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s);
void init_mouse_cursor(char *mouse, char bc);
void putblock8_8(char *vram, int vxsize, int pxsize,
	int pysize, int px0, int py0, char *buf, int bxsize);
#define COL8_000000		0
#define COL8_FF0000		1
#define COL8_00FF00		2
#define COL8_FFFF00		3
#define COL8_0000FF		4
#define COL8_FF00FF		5
#define COL8_00FFFF		6
#define COL8_FFFFFF		7
#define COL8_C6C6C6		8
#define COL8_840000		9
#define COL8_008400		10
#define COL8_848400		11
#define COL8_000084		12
#define COL8_840084		13
#define COL8_008484		14
#define COL8_848484		15
#define BGCOL			10
struct SEGMENT_DESCRIPTOR {
	short limit_low, base_low;
	char base_mid, access_right;
	char limit_high, base_high;
};
struct GATE_DESCRIPTOR {
	short offset_low, selector;
	char dw_count, access_right;
	short offset_high;
};
void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
#define ADR_IDT			0x0026f800
#define LIMIT_IDT		0x000007ff
#define ADR_GDT			0x00270000
#define LIMIT_GDT		0x0000ffff
#define ADR_BOTPAK		0x00280000
#define LIMIT_BOTPAK	0x0007ffff
#define AR_DATA32_RW	0x4092
#define AR_CODE32_ER	0x409a
#define AR_TSS32		0x0089
#define AR_INTGATE32	0x008e
void init_pic(void);
void inthandler27(int *esp);
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1
void inthandler21(int *esp);
void wait_KBC_sendready(void);
void init_keyboard(struct FIFO32 *fifo, int data0);
#define PORT_KEYDAT		0x0060
#define PORT_KEYCMD		0x0064
struct MOUSE_DEC {
	unsigned char buf[3], phase;
	int x, y, btn;
};
void inthandler2c(int *esp);
void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

/* memory.c */
#define MEMMAN_FREES		4090	/* これで約32KB */
#define MEMMAN_ADDR			0x003c0000
struct FREEINFO {	/* あき情報 */
	unsigned int addr, size;
};
struct MEMMAN {		/* メモリ管理 */
	int frees, maxfrees, lostsize, losts;
	struct FREEINFO free[MEMMAN_FREES];
};
extern unsigned int memtotal;
struct MEMMAN *get_sys_memman();
unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size);

/* sheet.c */
#define MAX_SHEETS		256
struct SHEET {
	unsigned char *buf;
	int bxsize, bysize, vx0, vy0, col_inv, height, flags;
	Hfifo fifop;
	struct UIStruct ** uilst;
};
typedef struct SHEET Sheet,*HSHEET,*Hsheet;
struct SHTCTL {
	unsigned char *vram, *map;
	int xsize, ysize, top;
	struct SHEET *sheets[MAX_SHEETS];
	struct SHEET sheets0[MAX_SHEETS];
};
extern struct SHTCTL* shtctl;
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize);
struct SHEET *sheet_alloc();
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv);
void sheet_updown(struct SHEET *sht, int height);
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1);
void sheet_slide(struct SHEET *sht, int vx0, int vy0);
void sheet_free(struct SHEET *sht);

/* timer.c */
#define MAX_TIMER		500
struct TIMER {
	struct TIMER *next;
	unsigned int timeout, flags;
	struct FIFO32 *fifo;
	int data;
};
typedef struct TIMER Timer,*HTIMER,*Htimer;
struct TIMERCTL {
	unsigned int count, next;
	struct TIMER *t0;
	struct TIMER timers0[MAX_TIMER];
};
extern struct TIMERCTL timerctl;
void init_pit(void);
struct TIMER *timer_alloc(void);
void timer_free(struct TIMER *timer);
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data);
void timer_settime(struct TIMER *timer, unsigned int timeout);
void inthandler20(int *esp);
void make_window(unsigned char *buf, int xsize, int ysize, char *title);
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l);
void make_textbox(struct SHEET *sht, int x0, int y0, int sx, int sy, int c);

//xthoa fexos pit&tit
#define MAX_TASKS 1024
void farjmp(int eip,int cs);
struct TSS32{
	int backlink,esp0,ss0,esp1,ss1,esp2,ss2,cr3;
	int eip,eflags,eax,ecx,edx,ebx,esp,ebp,esi,edi;
	int es,cs,ss,ds,fs,gs;
	int ldtr,iomap;
};
struct TASK{
	struct TSS32 tss;
	int tsss;
	int fesp;
	int tid,acs;
	int flags;
	char* name;
	int dwinfo;
};
struct TASKCTL{
	struct TASK *ready[MAX_TASKS];
	struct TASK tasks0[MAX_TASKS];
	int taskcnt,readycnt;
	int now;
};
typedef struct TASK Task,*HTASK,*Htask;
extern Htimer task_timer;
void inittask(Htask task,int acs,int cs,int es);
void schedule();
Htask alloctask();
void task_ready(Htask task);
Htask task_now();
void task_sleep(Htask task);
Htask initmt(struct MEMMAN *memman);
#define task_suspend task_sleep

//xthoa fexos console
#define CONS_START_POS 20,20
#define NEW_SHT_HEIGHT shtctl->top
struct coord{
	int x;
	int y;
};
typedef struct coord Position;
struct area{
	Position lu;
	Position rd;
};
typedef struct area Area;
struct CONSOLE{
	struct UIStruct *winui;
	Hfifo fifop;
	Htimer cursor;
	Htask taskp;
	char* commands;
	Position* curspos;
	void* cmdfunc;
};
typedef struct CONSOLE Console,*Hcons;

void cons_putchar(Hcons cons,char i);
void cons_putstr(Hcons cons,const char *string);
void cons_putline(Hcons cons,const char* string);
void cons_printf(Hcons cons,char* format,...);

extern int segcnt;

void draw_color(char* buf,int x,int y,int color);

#define BLACK 0
#define RED 1
#define GREEN 2
#define BLUE 4
#define DARK 8
#define WHITE 7

#define USE_STACK(x)

#define FLAGS_OVERRUN		0x0001
#define PORT_KEYSTA				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47
#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000
#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4
#define SHEET_USE		1
#define PIT_CTRL	0x0043
#define PIT_CNT0	0x0040
#define TIMER_FLAGS_ALLOC		1	/* 確保した状態 */
#define TIMER_FLAGS_USING		2	/* タイマ作動中 */

#define MSG_MOUSELD 1
#define MSG_MOUSELU 2
#define MSG_MOUSERD 4
#define MSG_MOUSERU 8
#define MSG_MOUSECD 16
#define MSG_MOUSECU 32

//UI
#define MAX_UI_COUNT 2048
#define WIN_UI_CREATE_POS 40,40

struct UIStruct{
	Hsheet sht;//4
	Area pos_sht;//16
	Hfifo fifop;//4
	struct UIStruct ** childlst;//4
	int childcnt;//4
	int flag;//4
};//32
typedef struct UIStruct UI,*HUI,*Hui;

extern UI* uilst;

void initui();
UI* allocui(Hsheet sht,Hfifo fifop,Area area);
UI* create_window(Hfifo fifop,Position xylen,char* title);
UI* create_textbox(UI* win,Position xylen);

void msgbox(char* title,char* msg,Position xylen);

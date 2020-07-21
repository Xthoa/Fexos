#include "bootpack.h"
#include <stdio.h>
#include<string.h>
struct TASKCTL *taskctl;
struct SHTCTL *shtctl;
Hsheet sht_back;
Htimer task_timer;
Htask task_now(){
	return taskctl->ready[taskctl->now];
}
void init_screen8(char *vram, int x, int y){
	memfill(vram, x, 42,  0,     0,      x -  1, y - 1);
	return;
}
void putfonts8_ctrl_sht(Hsheet sht,int x,int y,int c,int b,char* s,int len){
	int i,j;
	for(i=0,j=0;i<len;i++){
		if(s[i]=='\n'){
			putfonts8_asc_sht(sht,x,y,c,b,s,i);
			j=i;y+=16;
		}
	}
}
void cons_putchar(Hcons cons,char i) USE_STACK(8) {
	if (i=='\b' && cons->curspos->x > 8)cons->curspos->x -= 8;
	else if (i=='\n'){
		cons->curspos->x=8;
		if(cons->curspos->y<140)cons->curspos->y+=16;
		else{
			int x,y;
			for(y=28;y<140;y++){
				for(x=8;x<248;x++)cons->winui->sht->buf[x+y*256]=cons->winui->sht->buf[x+(y+16)*256];
			}
			memfill(cons->winui->sht->buf,256,BLACK,8,140,248,156);
			sheet_refresh(cons->winui->sht,8,28,248,156);
		}
	}
	else if(cons->curspos->x < 208 && i=='\t'){//tab
		putfonts8_asc_sht(cons->winui->sht,cons->curspos->x,cons->curspos->y,COL8_FFFFFF,COL8_000000,"    ",4);
		cons->curspos->x+=32;
	}
	else if(cons->curspos->x < 240 && ((unsigned char)i)>=0x20){
		putfonts8_asc_sht(cons->winui->sht, cons->curspos->x,cons->curspos->y, COL8_FFFFFF, COL8_000000, &i, 1);
		cons->curspos->x += 8;
	}
}
void cons_putline(Hcons cons,const char* string){
	cons_putstr(cons,string);
	cons_putchar(cons,'\n');
}
void cons_putstr(Hcons cons,const char* string) USE_STACK(4) {
	int i;
	for(i=0;string[i]!=0;i++)cons_putchar(cons,string[i]);
}
void cons_printf(Hcons cons,char* format,...)
USE_STACK(128) {
	va_list v;
	va_start(v,format);
	char s[128];
	vsprintf(s,format,v);
	cons_putstr(cons,s);
	va_end(v);
}
void apptaskfn(void);
int shellcmd(char* cmd,Hcons cons)
USE_STACK(36) {
	char s[24];
	struct MEMMAN* memman=get_sys_memman();
	int total=memman_total(memman);
	if(strcmp(cmd,"cls")==0){
		memfill(cons->winui->sht->buf,cons->winui->sht->bxsize,BLACK,7,27,247,155);
		cons->curspos->y=28;
		sheet_refresh(cons->winui->sht,8,28,248,156);
	}else if(strcmp(cmd,"mem")==0){
		sprintf(s,"total %9dMB\n",memtotal>>20);
		cons_putstr(cons,s);
		sprintf(s,"used  %9dMB\n",(memtotal-total)>>20);
		cons_putstr(cons,s);
		sprintf(s,"free  %9dMB\n",total>>20);
		cons_putstr(cons,s);
	}else if(strcmp(cmd,"nothing")==0)total=total;
	else if(strncmp(cmd,"echo ",5)==0)cons_printf(cons,"%s\n",cmd+5);
	else if(strcmp(cmd,"cmd")==0){
		Htask apptask=alloctask("Console");
		inittask(apptask,1,2,1);
		apptask->tss.eip = apptaskfn;
		apptask->tss.esp = (apptask->fesp=memman_alloc_4k(memman,64*1024))+64*1024;
		apptask->dwinfo  = shellcmd;
		task_ready(apptask);
	}
	else if(strcmp(cmd,"exit")==0)return 0;
	else cons_printf(cons,"Unknown command.\n");
	return 1;
}
int setcmd(char* cmd,Hcons cons){
	int stat;
	if(strcmp(cmd,"query")==0)stat=0,cmd+=6;
	else if(strcmp(cmd,"set")==0)
}
void optfn(void){
	Position pos={216,48};
	msgbox("Welcome","Welcome to Fexos 1.2.8!",pos);
	while(1);
}
void apptaskfn(void){
	Console cons;
	Fifo fifo;
	Htask task=task_now();
	int(*runcmd)(char*,Hcons)=cons.cmdfunc=task->dwinfo;
	int fifobuf[32];
	int i,j=0;
	Position curspos,winpos={256,165};
	UI* ui=create_window(&fifo,winpos,"Console");
	make_textbox(ui->sht,8,28,240,128,BLACK);
	sheet_refresh(ui->sht,0,0,256,165);
	Htimer cursor=timer_alloc();
	cons.cursor=cursor;
	cons.fifop=&fifo;
	cons.taskp=task;
	cons.winui=ui;
	struct MEMMAN* memman=get_sys_memman()
	cons.commands=(char*)memman_alloc_4k(memman,1024*4);
	char *u=cons.commands,*up=u;
	fifo32_init(&fifo,32,fifobuf,task);
	timer_init(cursor,&fifo,1);
	cons.curspos=&curspos;
	putfonts8_asc_sht(ui->sht,8,28,WHITE,BLACK,">",1);
	curspos.x=16;
	curspos.y=28;
	ui->sht->fifop=&fifo;
	timer_settime(cursor,50);
	while(1){
		io_cli();
		if(fifo32_status(&fifo)==0){
			task_sleep(task);
			io_sti();
		}else{
			i=fifo32_get(&fifo);
			io_sti();
			if(i==1){
				timer_settime(cursor,50);j^=1;
				putfonts8_asc_sht(ui->sht,curspos.x,curspos.y,WHITE,BLACK,j?"\xdb":" ",1);
			}
			else{
				if (i=='\b' && curspos.x > 16) { /* \b */
					putfonts8_asc_sht(ui->sht,curspos.x,curspos.y,WHITE,BLACK, " ",1);
					curspos.x -= 8;up--;
				}
				else if (i=='\n') { /* enter */
					putfonts8_asc_sht(ui->sht,curspos.x,curspos.y,WHITE,BLACK, " ",1);
					curspos.x = 8;
					if(curspos.y<140)curspos.y+=16;
					else{
						int x,y;
						for(y=28;y<140;y++){
							for(x=8;x<248;x++)ui->sht->buf[x+y*256]=ui->sht->buf[x+(y+16)*256];
						}
						memfill(ui->sht->buf,256,BLACK,8,140,247,155);
						sheet_refresh(ui->sht,8,28,248,156);
					}
					*(up++)=0;
					if(runcmd(u,&cons)==0){
						break;
					}
					u=up;curspos.x+=8;
					putfonts8_asc_sht(ui->sht,8,curspos.y,WHITE,BLACK,">",1);
				}
				else if(curspos.x < 208 && i=='\t'){//tab
					putfonts8_asc_sht(ui->sht,curspos.x,curspos.y,COL8_FFFFFF,COL8_000000,"    ",4);
					curspos.x+=32;
					for(i=4;i>0;i--)*(up++)=0x20;
				}
				else if(curspos.x < 240 && ((unsigned char)i)>=0x20){
					putfonts8_asc_sht(ui->sht, curspos.x,curspos.y, COL8_FFFFFF, COL8_000000, &i, 1);
					curspos.x += 8;
					*(up++)=i;
				}
			}
		}
	}
	memman_free_4k(memman,cons.commands,4*1024);
	sheet_free(ui->sht);
	freeui(ui);
	task_delete(task,&fifo);
}
unsigned int memtotal;
void HariMain(void){
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	struct FIFO32 fifo;
	char s[40];
	Hsheet sht_mouse,sht_win;
	int fifobuf[128],mx, my, i, cursor_x=8, cursor_c=1,shift,appstack;
	struct MOUSE_DEC mdec;
	struct MEMMAN *memman = (struct MEMMAN *) MEMMAN_ADDR;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	unsigned char buf_mouse[256],*buf_back,*buf_win;
	Htimer timer3;
	Htask systask,apptask;
	Hsheet winpt;int mvx,mvy;
	static char keytable[0x59] = {
		0,  0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',  '\t',
		'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',   0,   'a', 's',
		'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',  0,   '\\', 'z', 'x', 'c', 'v',
		'b', 'n', 'm', ',', '.', '/', 0,   '*', 0,   ' ', 0,   '\xe1',  '\xe2',   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.',0,0,0,0xfd,0xfe
	};
	static char Keytable[0x59] = {
		0,   0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',  '\b', '\t',
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']', '\n',   0, 'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'','\xe0',0,'\\', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', ',', '.', '/', 0,   '*', 0,   ' ',   0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.',0,0,0,0xfd,0xfe
	};
	static char keyTable[0x59] = {
		0,   0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b', '\t',
		'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',   0,   'A', 'S',
		'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '\"', '~',   0,   '|', 'Z', 'X', 'C', 'V',
		'B', 'N', 'M', '<', '>', '?', 0,   '*', 0,   ' ', 0,   0,   0,   0,   0,   0,
		0,   0,   0,   0,   0,   0,   0,   '7', '8', '9', '-', '4', '5', '6', '+', '1',
		'2', '3', '0', '.',0,0,0,0xfd,0xfe
	};
	init_gdtidt();
	init_pic();
	io_sti();
	fifo32_init(&fifo, 128, fifobuf,0);
	init_pit();
	init_keyboard(&fifo, 256);
	enable_mouse(&fifo, 512, &mdec);
	io_out8(PIC0_IMR, 0xf8); /* pit pic1 keyboard enable */
	io_out8(PIC1_IMR, 0xef); /* mouse enable */
	timer3=timer_alloc();
	timer_init(timer3,&fifo,1);
	memtotal = memtest(0x00400000, 0xbfffffff);memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); /* 0x00001000 - 0x0009efff */
	memman_free(memman, 0x00400000, memtotal - 0x00400000);
	init_palette();
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	initui();
	sht_back  = sheet_alloc(shtctl);
	sht_mouse = sheet_alloc(shtctl);
//	sht_win   = sheet_alloc(shtctl);
	buf_back  = (unsigned char *) memman_alloc_4k(memman, binfo->scrnx * binfo->scrny);
//	buf_win   = memman_alloc_4k(memman,160*136);
//	make_window(buf_win,160,136,"Fexos 1.2.6");
	init_screen8(buf_back, binfo->scrnx, binfo->scrny);
	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1); /* 透明色なし */
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
//	sheet_setbuf(sht_win,buf_win,160,136,-1);
	sheet_slide(sht_back, 0, 0);
	init_mouse_cursor(buf_mouse, 99);
	mx=my=10;
	sheet_slide(sht_mouse,mx,my);
//	sheet_slide(sht_win,160,160);
	sheet_updown(sht_back,0);
//	sheet_updown(sht_win,1);
	sheet_updown(sht_mouse,2);
//	make_textbox(sht_win, 8, 28, 144, 16, WHITE);
//	sheet_refresh(sht_win,6,26,154,46);
	fifo.task=systask=initmt(memman);
/*	sprintf(s, "total %9dMB",memtotal>>20 );
	putfonts8_asc_sht(sht_win, 8, 68, COL8_000000, COL8_C6C6C6, s, 17);
	sprintf(s, "used  %9dMB",(memtotal-memman_total(memman))>>20);
	putfonts8_asc_sht(sht_win, 8, 84, COL8_000000, COL8_C6C6C6, s, 17);
	sprintf(s, "free  %9dMB",memman_total(memman)>>20);
	putfonts8_asc_sht(sht_win, 8, 100, COL8_000000, COL8_C6C6C6, s, 17);
*/	
	create_console(shellcmd,"Console");
	Htask task_opt=alloctask("Options");
	inittask(task_opt,1,2,1);
	task_opt->tss.eip=optfn;
	task_opt->tss.esp=(task_opt->fesp=memman_alloc_4k(memman,64*1024))+64*1024;
	task_ready(task_opt);
	timer_settime(timer3,50);
/*	Htimer tmr=timer_alloc();
	timer_init(tmr,&fifo,10);
	timer_settime(tmr,20);*/
	for (;;) {
		io_cli();
		if (fifo32_status(&fifo) == 0)io_stihlt();
		else {
			i = fifo32_get(&fifo);
			io_sti();
			if (256 <= i && i <= 511) { /* keyboard data */
			//	sprintf(s, "%02X", i - 256);
			//	putfonts8_asc_sht(sht_win, 8, 116, COL8_000000, COL8_C6C6C6, s, 2);
				if(i==256+0x36 || i==256+0x2a)shift|=2;
				if(i==256+0xb6 || i==256+0xaa)shift&=~2;
				if(i==256+0x3a)shift^=1;
				if(i-256>=0x80)continue;
				if(shift==0)/**(up++)=*/s[0] = keytable[i - 256];
				else if((shift&1)==1)/**(up++)=*/s[0]=Keytable[i-256];
				else /**(up++)=*/s[0]=keyTable[i-256];
				/*if(shtctl->sheets[shtctl->top-1]!=sht_win)*/fifo32_put(shtctl->sheets[shtctl->top-1]->fifop,s[0]);
				/*else{
					if (s[0]=='\b' && cursor_x > 8) { /* \b *
						putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, " ", 1);
						cursor_x -= 8;
					}
					else if (s[0]=='\n') { /* enter *
						putfonts8_asc_sht(sht_win,8,28,COL8_000000,COL8_FFFFFF,"                 ",cursor_x>>3);
						cursor_x = 8; 
					}
					else if(s[0]=='\t' && cursor_x < 112){//tab
						putfonts8_asc_sht(sht_win,cursor_x,28,COL8_000000,COL8_FFFFFF,"    ",4);
						cursor_x+=32;
					}
					else if (((unsigned char)s[0])>=0x20 && cursor_x < 144) { /* normal char *
						putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF, s, 1);
						cursor_x += 8;
					}
				}*/
			} else if (512 <= i && i <= 767) { /* マウスデータ */
				if (mouse_decode(&mdec, i - 512) != 0) {
					/* データが3バイト揃ったので表示 */
					mx += mdec.x;my += mdec.y;
					if (mx < 0)mx = 0;
					if (my < 0)my = 0;
					if (mx > binfo->scrnx - 1)mx = binfo->scrnx - 1;
					if (my > binfo->scrny - 1)my = binfo->scrny - 1;
			//		sprintf(s, "(%3d, %3d)", mx, my);
			//		putfonts8_asc_sht(sht_win, 8, 52, COL8_000000, COL8_C6C6C6, s, 10);
					sheet_slide(sht_mouse, mx, my);
			//		sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					if ((mdec.btn & 0x01) != 0){//left
			//			s[1] = 'L';
						if(mvx<0){
							int ht;
							Hsheet sht;
							for(ht=shtctl->top-1;ht>0;ht--){
								sht=shtctl->sheets[ht];
								int x=mx-sht->vx0,y=my-sht->vy0;
								if(0<=x && x<sht->bxsize && 0<=y && y<sht->bysize){
									sheet_updown(sht,shtctl->top-1);
								//	fifo32_put(sht->fifop,MSG_MOUSELD);//mouse left down
									if(1<=x && x<sht->bxsize-1 && 1<=y && y<21){
										mvx=mx;mvy=my;
										winpt=sht;
									}
									break;
								}
							}
						}else{
							sheet_slide(winpt,winpt->vx0+mx-mvx,winpt->vy0+my-mvy);
							mvx=mx;mvy=my;
						}
					}else{
						mvx=-1;
						//fifo32_put(winpt->fifop,MSG_MOUSELU);//mouse left up
					} 
					if ((mdec.btn & 0x02) != 0){
			//			s[3] = 'R';
					//	fifo32_put(shtctl->sheets[ht]->fifop,MSG_MOUSER);//mouse right
					}
					if ((mdec.btn & 0x04) != 0){
			//			s[2] = 'C';
					//	fifo32_put(shtctl->sheets[ht]->fifop,MSG_MOUSEC);//mouse middle
					}
					putfonts8_asc_sht(sht_win, 32, 116, COL8_000000, COL8_C6C6C6, s, 15);
				}
			}else if (i <= 1) { /* カーソル用タイマ */
				putfonts8_asc_sht(sht_win, cursor_x, 28, COL8_000000, COL8_FFFFFF,cursor_c?"\xdb":" ", 1);
				cursor_c^=1;
				timer_settime(timer3, 50);
			}/*else if(i==10){
				Position pos={200,60};
				msgbox("Welcome","Welcome to Fexos 1.2.8!",pos);
			}*/
		}
	}
}
/******************
 *       UI       *
 ******************/
UI* uilst;
void initui(){
	uilst=memman_alloc_4k(get_sys_memman(),sizeof(UI)*2048);//32*2048=64k
}
UI* allocui(Hsheet sht,Hfifo fifop,Area area){
	int i;
	UI* ui;
	for(i=0;i<MAX_UI_COUNT;i++){
		if(uilst[i].flag==0){
			ui=&uilst[i];
			ui->fifop=fifop;
			ui->pos_sht=area;
			ui->sht=sht;
			ui->childlst=memman_alloc(get_sys_memman(),128);
			ui->childcnt=0;
			ui->flag=1;//Using
			return ui;
		}
	}
	return NULL;
}
UI* create_window(Hfifo fifop,Position xylen,char* title){
	Area a={{0,xylen.x},{0,xylen.y}};
	Hsheet sht=sheet_alloc(shtctl);
	char* buf=memman_alloc_4k(get_sys_memman(),xylen.x*xylen.y);
	sheet_setbuf(sht,buf,xylen.x,xylen.y,-1);
	sheet_slide(sht,WIN_UI_CREATE_POS);
	sheet_updown(sht,NEW_SHT_HEIGHT);
	make_window(buf,xylen.x,xylen.y,title);
	sheet_refresh(sht,0,0,xylen.x,xylen.y);
	return allocui(sht,fifop,a);
}
void freeui(UI* ui){
	int i=0;
	for(;i<ui->childcnt;i++)freeui(ui->childlst[i]);
	ui->flag=0;
}
//EasyUI
void msgbox(char* title,char* msg,Position xylen)
USE_STACK(56) {
	Fifo fifo;//28
	int fifobuf[4];//16
	Htask task=task_now();//4
	fifo32_init(&fifo,4,fifobuf,task);
	UI* ui=create_window(&fifo,xylen,title);//4
	putfonts8_asc_sht(ui->sht,8,28,BLACK,DARK,msg,strlen(msg));
	ui->sht->fifop=&fifo;
	while(1){
		io_cli();
		if(fifo32_status(&fifo)==0){
			task_sleep(task);
			io_sti();
		}else{
			int i=fifo32_get(&fifo);//4
			io_sti(); 
			if(i=='\n')break;
		}
	}
	sheet_free(ui->sht);
	freeui(ui);
	return;
}
/*************************
 *   Graphics & GDTIDT   *
 *************************/
void create_console(void* cmdfunc,char* title){
	Htask task=alloctask(title);
	inittask(task,1,2,1);
	task->tss.eip=apptaskfn;
	task->tss.esp=(task->fesp=memman_alloc_4k(get_sys_memman(),64*1024))+64*1024;
	task->dwinfo=cmdfunc;
	task_ready(task);
}
void make_window(unsigned char *buf, int xsize, int ysize, char *title){
	static char closebtn[9][10] = {
		"@@QQQQQQ@@",
		"Q@@QQQQ@@Q",
		"QQ@@QQ@@QQ",
		"QQQ@@@@QQQ",
		"QQQQ@@QQQQ",
		"QQQ@@@@QQQ",
		"QQ@@QQ@@QQ",
		"Q@@QQQQ@@Q",
		"@@QQQQQQ@@"
	};
	int x, y;
	memfill(buf, xsize, BLACK, 0,         0,         xsize - 1, 0        );
	memfill(buf, xsize, BLACK, 0,         0,         0,         ysize - 1);
	memfill(buf, xsize, BLACK, xsize - 1, 0,         xsize - 1, ysize - 1);
	memfill(buf, xsize, BLACK, 0,         ysize - 1, xsize - 1, ysize - 1);
	memfill(buf, xsize, DARK , 1, 21,  xsize - 2, ysize - 2);
	memfill(buf, xsize, WHITE, 1,  1,  xsize - 2, 20 );
	putfonts8_asc(buf, xsize, 12, 4, BLACK, title);
	for (y = 0; y < 9; y++) {
		for (x = 0; x < 10; x++) {
			if (closebtn[y][x] == '@')buf[(7 + y) * xsize + (xsize - 18 + x)] = BLACK;
		}
	}
	return;
}
void putfonts8_asc_sht(struct SHEET *sht, int x, int y, int c, int b, char *s, int l){
	memfill(sht->buf, sht->bxsize, b, x, y, x + l * 8 - 1, y + 15);
	putfonts8_asc(sht->buf, sht->bxsize, x, y, c, s);
	sheet_refresh(sht, x, y, x + l * 8, y + 16);
	return;
}
void make_textbox(struct SHEET *sht, int x0, int y0, int sx, int sy, int c){
	int x1 = x0 + sx, y1 = y0 + sy;
	memfill(sht->buf, sht->bxsize, BLACK, x0 - 1, y0 - 2, x1 + 0, y0 - 2);
	memfill(sht->buf, sht->bxsize, BLACK, x0 - 2, y0 - 2, x0 - 2, y1 + 0);
	memfill(sht->buf, sht->bxsize, BLACK, x0 - 2, y1 + 1, x1 + 0, y1 + 1);
	memfill(sht->buf, sht->bxsize, BLACK, x1 + 1, y0 - 2, x1 + 1, y1 + 1);
	memfill(sht->buf, sht->bxsize, c,     x0 - 1, y0 - 1, x1 + 0, y1 + 0);
	return;
}
void init_gdtidt(void){
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	struct GATE_DESCRIPTOR    *idt = (struct GATE_DESCRIPTOR    *) ADR_IDT;
	int i;
	set_segmdesc(gdt,0,0,0);
	set_segmdesc(gdt + 1, 0xffffffff,   0x00000000, AR_DATA32_RW);
	set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);
	for (i = 3; i <= LIMIT_GDT / 8; i++)set_segmdesc(gdt + i, 0, 0, 0);
	load_gdtr(LIMIT_GDT, ADR_GDT);
	load_idtr(LIMIT_IDT, ADR_IDT);
	for (i = 0; i <= LIMIT_IDT / 8; i++)set_gatedesc(idt + i, 0, 0, 0);
	set_gatedesc(idt + 0x20, (int) asm_inthandler20, 2 * 8, AR_INTGATE32);
	set_gatedesc(idt + 0x21, (int) asm_inthandler21, 2 * 8, AR_INTGATE32);
	set_gatedesc(idt + 0x27, (int) asm_inthandler27, 2 * 8, AR_INTGATE32);
	set_gatedesc(idt + 0x2c, (int) asm_inthandler2c, 2 * 8, AR_INTGATE32);
	return;
}
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar){
	if (limit > 0xfffff) {
		ar |= 0x8000; /* G_bit = 1 */
		limit /= 0x1000;
	}
	sd->limit_low    = limit & 0xffff;
	sd->base_low     = base & 0xffff;
	sd->base_mid     = (base >> 16) & 0xff;
	sd->access_right = ar & 0xff;
	sd->limit_high   = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
	sd->base_high    = (base >> 24) & 0xff;
	return;
}
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar){
	gd->offset_low   = offset & 0xffff;
	gd->selector     = selector;
	gd->dw_count     = (ar >> 8) & 0xff;
	gd->access_right = ar & 0xff;
	gd->offset_high  = (offset >> 16) & 0xffff;
	return;
}
void init_palette(void){
	static unsigned char table_rgb[16 * 3] = {
		0x00, 0x00, 0x00,	/*  0:黒 */
		0xff, 0x00, 0x00,	/*  1:明るい赤 */
		0x00, 0xff, 0x00,	/*  2:明るい緑 */
		0xff, 0xff, 0x00,	/*  3:明るい黄色 */
		0x00, 0x00, 0xff,	/*  4:明るい青 */
		0xff, 0x00, 0xff,	/*  5:明るい紫 */
		0x00, 0xff, 0xff,	/*  6:明るい水色 */
		0xff, 0xff, 0xff,	/*  7:白 */
		0xc6, 0xc6, 0xc6,	/*  8:明るい灰色 */
		0x84, 0x00, 0x00,	/*  9:暗い赤 */
		0x00, 0x84, 0x00,	/* 10:暗い緑 */
		0x84, 0x84, 0x00,	/* 11:暗い黄色 */
		0x00, 0x00, 0x84,	/* 12:暗い青 */
		0x84, 0x00, 0x84,	/* 13:暗い紫 */
		0x00, 0x84, 0x84,	/* 14:暗い水色 */
		0x84, 0x84, 0x84	/* 15:暗い灰色 */
	};
	set_palette(0, 15, table_rgb);
	unsigned char table2[216*3];
	int r,g,b,t;
	for(b=0;b<6;b++){
		for(g=0;g<6;g++){
			for(r=0;r<6;r++){
				t=(r+g*6+b*36)*3;
				table2[t]=r*51;
				table2[t+1]=g*51;
				table2[t+2]=b*51;
			}
		}
	}
	set_palette(16,231,table2);
	return;
}
void set_palette(int start, int end, unsigned char *rgb){
	int i, eflags;
	eflags = io_load_eflags();	/* 割り込み許可フラグの値を記録する */
	io_cli(); 					/* 許可フラグを0にして割り込み禁止にする */
	io_out8(0x03c8, start);
	for (i = start; i <= end; i++) {
		io_out8(0x03c9, rgb[0] / 4);
		io_out8(0x03c9, rgb[1] / 4);
		io_out8(0x03c9, rgb[2] / 4);
		rgb += 3;
	}
	io_store_eflags(eflags);	/* 割り込み許可フラグを元に戻す */
	return;
}
void memfill(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1){
	int x, y;
	for (y = y0; y <= y1; y++) {
		for (x = x0; x <= x1; x++)vram[y * xsize + x] = c;
	}
	return;
}
void putfont8(char *vram, int xsize, int x, int y, char c, char *font){
	int i;
	char *p, d /* data */;
	for (i = 0; i < 16; i++) {
		p = vram + (y + i) * xsize + x;
		d = font[i];
		if ((d & 0x80) != 0) { p[0] = c; }
		if ((d & 0x40) != 0) { p[1] = c; }
		if ((d & 0x20) != 0) { p[2] = c; }
		if ((d & 0x10) != 0) { p[3] = c; }
		if ((d & 0x08) != 0) { p[4] = c; }
		if ((d & 0x04) != 0) { p[5] = c; }
		if ((d & 0x02) != 0) { p[6] = c; }
		if ((d & 0x01) != 0) { p[7] = c; }
	}
	return;
}
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s){
	extern char hankaku[4096];
	for (; *s != 0x00; s++) {
		putfont8(vram, xsize, x, y, c, hankaku + *s * 16);
		x += 8;
	}
	return;
}
void putblock8_8(char *vram, int vxsize, int pxsize,int pysize, int px0, int py0, char *buf, int bxsize){
	int x, y;
	for (y = 0; y < pysize; y++) {
		for (x = 0; x < pxsize; x++)vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x];
	}
	return;
}
void init_pic(void){
	io_out8(PIC0_IMR,  0xff  ); /* 全ての割り込みを受け付けない */
	io_out8(PIC1_IMR,  0xff  ); /* 全ての割り込みを受け付けない */
	io_out8(PIC0_ICW1, 0x11  ); /* エッジトリガモード */
	io_out8(PIC0_ICW2, 0x20  ); /* IRQ0-7は、INT20-27で受ける */
	io_out8(PIC0_ICW3, 1 << 2); /* PIC1はIRQ2にて接続 */
	io_out8(PIC0_ICW4, 0x01  ); /* ノンバッファモード */
	io_out8(PIC1_ICW1, 0x11  ); /* エッジトリガモード */
	io_out8(PIC1_ICW2, 0x28  ); /* IRQ8-15は、INT28-2fで受ける */
	io_out8(PIC1_ICW3, 2     ); /* PIC1はIRQ2にて接続 */
	io_out8(PIC1_ICW4, 0x01  ); /* ノンバッファモード */
	io_out8(PIC0_IMR,  0xfb  ); /* 11111011 PIC1以外は全て禁止 */
	io_out8(PIC1_IMR,  0xff  ); /* 11111111 全ての割り込みを受け付けない */
	return;
}
void inthandler27(int *esp){
	io_out8(PIC0_OCW2, 0x67); /* IRQ-07受付完了をPICに通知 */
	return;
}
/****************
 *     Fifo     *
 ****************/
void fifo32_init(struct FIFO32 *fifo, int size, int *buf,Htask task){
	fifo->size = size;
	fifo->buf = buf;
	fifo->free = size; /* 空き */
	fifo->flags = 0;
	fifo->p = 0; /* 書き込み位置 */
	fifo->q = 0; /* 読み込み位置 */
	fifo->task=task;
	return;
}
int fifo32_put(struct FIFO32 *fifo, int data){
	if(fifo==NULL)return 0;
	if (fifo->free == 0) {
		fifo->flags |= FLAGS_OVERRUN;
		return -1;
	}
	fifo->buf[fifo->p] = data;
	fifo->p++;
	if (fifo->p == fifo->size)fifo->p = 0;
	fifo->free--;
	if(fifo->task!=0 && fifo->task->flags!=2)task_ready(fifo->task);
	return 0;
}
int fifo32_get(struct FIFO32 *fifo){
	int data;
	if (fifo->free == fifo->size)return -1;
	data = fifo->buf[fifo->q];
	fifo->q++;
	if (fifo->q == fifo->size)fifo->q = 0;
	fifo->free++;
	return data;
}
int fifo32_status(struct FIFO32 *fifo){
	return fifo->size - fifo->free;
}
/******************
 *    KeyBoard    *
 ******************/
struct FIFO32 *keyfifo;
int keydata0;
void inthandler21(int *esp){
	int data;
	io_out8(PIC0_OCW2, 0x61);	/* IRQ-01受付完了をPICに通知 */
	data = io_in8(PORT_KEYDAT);
	fifo32_put(keyfifo, data + keydata0);
	return;
}
void wait_KBC_sendready(void){
	while((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) != 0);
	return;
}
void init_keyboard(struct FIFO32 *fifo, int data0){
	keyfifo = fifo;
	keydata0 = data0;
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
	return;
}
/******************
 *     Memory     *
 ******************/
unsigned int memtest(unsigned int start, unsigned int end){
	char flg486 = 0;
	unsigned int eflg, cr0, i;
	eflg = io_load_eflags();
	eflg |= EFLAGS_AC_BIT; /* AC-bit = 1 */
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	if ((eflg & EFLAGS_AC_BIT) != 0)flg486 = 1;
	eflg &= ~EFLAGS_AC_BIT; /* AC-bit = 0 */
	io_store_eflags(eflg);
	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE; /* キャッシュ禁止 */
		store_cr0(cr0);
	}
	i = memtest_sub(start, end);
	if (flg486 != 0) {
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE; /* キャッシュ許可 */
		store_cr0(cr0);
	}
	return i;
}
void memman_init(struct MEMMAN *man){
	man->frees = 0;			/* あき情報の個数 */
	man->maxfrees = 0;		/* 状況観察用：freesの最大値 */
	man->lostsize = 0;		/* 解放に失敗した合計サイズ */
	man->losts = 0;			/* 解放に失敗した回数 */
	return;
}
unsigned int memman_total(struct MEMMAN *man){
	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++) {
		t += man->free[i].size;
	}
	return t;
}
struct MEMMAN* get_sys_memman(){return (struct MEMMAN*)MEMMAN_ADDR;}
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size){
	unsigned int i, a;
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].size >= size) {
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if (man->free[i].size == 0) {
				man->frees--;
				for (; i < man->frees; i++)man->free[i] = man->free[i + 1];
			}
			return a;
		}
	}
	return 0; /* あきがない */
}
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size){
	int i, j;
	for (i = 0; i < man->frees && man->free[i].addr <= addr; i++);
	if (i > 0) {
		if (man->free[i - 1].addr + man->free[i - 1].size == addr) {
			man->free[i - 1].size += size;
			if (i < man->frees) {
				if (addr + size == man->free[i].addr) {
					man->free[i - 1].size += man->free[i].size;
					man->frees--;
					for (; i < man->frees; i++)man->free[i] = man->free[i + 1];
				}
			}
			return 0; /* 成功終了 */
		}
	}
	if (i < man->frees) {
		if (addr + size == man->free[i].addr) {
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0; /* 成功終了 */
		}
	}
	if (man->frees < MEMMAN_FREES) {
		for (j = man->frees; j > i; j--)man->free[j] = man->free[j - 1];
		man->frees++;
		if (man->maxfrees < man->frees)man->maxfrees = man->frees;
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0; /* 成功終了 */
	}
	man->losts++;
	man->lostsize += size;
	return -1; /* 失敗終了 */
}
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size){
	unsigned int a;
	size = (size + 0xfff) & 0xfffff000;
	a = memman_alloc(man, size);
	return a;
}
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size){
	int i;
	size = (size + 0xfff) & 0xfffff000;
	i = memman_free(man, addr, size);
	return i;
}
/*****************
 *     Mouse     *
 *****************/
void init_mouse_cursor(char *mouse, char bc){
	static char cursor[16][16] = {
		"**************..",
		"*OOOOOOOOOOO*...",
		"*OOOOOOOOOO*....",
		"*OOOOOOOOO*.....",
		"*OOOOOOOO*......",
		"*OOOOOOO*.......",
		"*OOOOOOO*.......",
		"*OOOOOOOO*......",
		"*OOOO**OOO*.....",
		"*OOO*..*OOO*....",
		"*OO*....*OOO*...",
		"*O*......*OOO*..",
		"**........*OOO*.",
		"*..........*OOO*",
		"............*OO*",
		".............***"};
	int x, y;
	for (y = 0; y < 16; y++) {
		for (x = 0; x < 16; x++) {
			if (cursor[y][x] == '*') {
				mouse[y * 16 + x] = COL8_000000;
			}
			if (cursor[y][x] == 'O') {
				mouse[y * 16 + x] = COL8_FFFFFF;
			}
			if (cursor[y][x] == '.') {
				mouse[y * 16 + x] = bc;
			}
		}
	}
	return;
}
struct FIFO32 *mousefifo;
int mousedata0;
void inthandler2c(int *esp){
	int data;
	io_out8(PIC1_OCW2, 0x64);	/* IRQ-12受付完了をPIC1に通知 */
	io_out8(PIC0_OCW2, 0x62);	/* IRQ-02受付完了をPIC0に通知 */
	data = io_in8(PORT_KEYDAT);
	fifo32_put(mousefifo, data + mousedata0);
	return;
}
void enable_mouse(struct FIFO32 *fifo, int data0, struct MOUSE_DEC *mdec){
	mousefifo = fifo;
	mousedata0 = data0;
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	mdec->phase = 0; /* マウスの0xfaを待っている段階 */
	return;
}
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat){
	if (mdec->phase == 0) {
		if (dat == 0xfa)mdec->phase = 1;
	}
	else if (mdec->phase == 1) {
		if ((dat & 0xc8) == 0x08) {
			mdec->buf[0] = dat;
			mdec->phase = 2;
		}
	}
	else if (mdec->phase == 2) {
		mdec->buf[1] = dat;
		mdec->phase = 3;
	}
	else if (mdec->phase == 3) {
		mdec->buf[2] = dat;
		mdec->phase = 1;
		mdec->btn = mdec->buf[0] & 0x07;
		mdec->x = mdec->buf[1];
		mdec->y = mdec->buf[2];
		if ((mdec->buf[0] & 0x10) != 0)mdec->x |= 0xffffff00;
		if ((mdec->buf[0] & 0x20) != 0)mdec->y |= 0xffffff00;
		mdec->y = - mdec->y; /* マウスではy方向の符号が画面と反対 */
		return 1;
	}
	else return -1;
	return 0;
}
/*****************
 *     Sheet     *
 *****************/
struct SHTCTL *shtctl_init(struct MEMMAN *memman, unsigned char *vram, int xsize, int ysize)
{
	struct SHTCTL *ctl;
	int i;
	ctl = (struct SHTCTL *) memman_alloc_4k(memman, sizeof (struct SHTCTL));
	if (ctl == 0) {
		goto err;
	}
	ctl->map = (unsigned char *) memman_alloc_4k(memman, xsize * ysize);
	if (ctl->map == 0) {
		memman_free_4k(memman, (int) ctl, sizeof (struct SHTCTL));
		goto err;
	}
	ctl->vram = vram;
	ctl->xsize = xsize;
	ctl->ysize = ysize;
	ctl->top = -1; /* シートは一枚もない */
	for (i = 0; i < MAX_SHEETS; i++) {
		ctl->sheets0[i].flags = 0; /* 未使用マーク */
	}
err:
	return ctl;
}
struct SHEET *sheet_alloc(struct SHTCTL *ctl){
	struct SHEET *sht;
	int i;
	for (i = 0; i < MAX_SHEETS; i++) {
		if (ctl->sheets0[i].flags == 0) {
			sht = &ctl->sheets0[i];
			sht->flags = SHEET_USE; /* 使用中マーク */
			sht->height = -1; /* 非表示中 */
			return sht;
		}
	}
	return 0;	/* 全てのシートが使用中だった */
}
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv)
{
	sht->buf = buf;
	sht->bxsize = xsize;
	sht->bysize = ysize;
	sht->col_inv = col_inv;
	return;
}
void sheet_refreshmap(int vx0, int vy0, int vx1, int vy1, int h0)
{struct SHTCTL *ctl=shtctl;
	int h, bx, by, vx, vy, bx0, by0, bx1, by1;
	unsigned char *buf, sid, *map = ctl->map;
	struct SHEET *sht;
	if (vx0 < 0) { vx0 = 0; }
	if (vy0 < 0) { vy0 = 0; }
	if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
	if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }
	for (h = h0; h <= ctl->top; h++) {
		sht = ctl->sheets[h];
		sid = sht - ctl->sheets0; /* 番地を引き算してそれを下じき番号として利用 */
		buf = sht->buf;
		bx0 = vx0 - sht->vx0;
		by0 = vy0 - sht->vy0;
		bx1 = vx1 - sht->vx0;
		by1 = vy1 - sht->vy0;
		if (bx0 < 0) { bx0 = 0; }
		if (by0 < 0) { by0 = 0; }
		if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
		if (by1 > sht->bysize) { by1 = sht->bysize; }
		for (by = by0; by < by1; by++) {
			vy = sht->vy0 + by;
			for (bx = bx0; bx < bx1; bx++) {
				vx = sht->vx0 + bx;
				if (buf[by * sht->bxsize + bx] != sht->col_inv) {
					map[vy * ctl->xsize + vx] = sid;
				}
			}
		}
	}
	return;
}
void sheet_refreshsub( int vx0, int vy0, int vx1, int vy1, int h0, int h1)
{struct SHTCTL *ctl=shtctl;
	int h, bx, by, vx, vy, bx0, by0, bx1, by1;
	unsigned char *buf, *vram = ctl->vram, *map = ctl->map, sid;
	struct SHEET *sht;
	/* refresh範囲が画面外にはみ出していたら補正 */
	if (vx0 < 0) { vx0 = 0; }
	if (vy0 < 0) { vy0 = 0; }
	if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
	if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }
	for (h = h0; h <= h1; h++) {
		sht = ctl->sheets[h];
		buf = sht->buf;
		sid = sht - ctl->sheets0;
		/* vx0～vy1を使って、bx0～by1を逆算する */
		bx0 = vx0 - sht->vx0;
		by0 = vy0 - sht->vy0;
		bx1 = vx1 - sht->vx0;
		by1 = vy1 - sht->vy0;
		if (bx0 < 0) { bx0 = 0; }
		if (by0 < 0) { by0 = 0; }
		if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
		if (by1 > sht->bysize) { by1 = sht->bysize; }
		for (by = by0; by < by1; by++) {
			vy = sht->vy0 + by;
			for (bx = bx0; bx < bx1; bx++) {
				vx = sht->vx0 + bx;
				if (map[vy * ctl->xsize + vx] == sid) {
					vram[vy * ctl->xsize + vx] = buf[by * sht->bxsize + bx];
				}
			}
		}
	}
	return;
}

void sheet_updown(struct SHEET *sht, int height){
	struct SHTCTL *ctl = shtctl;
	int h, old = sht->height;
	if (height > ctl->top + 1) {
		height = ctl->top + 1;
	}
	if (height < -1) {
		height = -1;
	}
	sht->height = height;
	if (old > height) {	/* 以前よりも低くなる */
		if (height >= 0) {
			for (h = old; h > height; h--) {
				ctl->sheets[h] = ctl->sheets[h - 1];
				ctl->sheets[h]->height = h;
			}
			ctl->sheets[height] = sht;
			sheet_refreshmap(sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1);
			sheet_refreshsub(sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height + 1, old);
		} else {	/* 非表示化 */
			if (ctl->top > old) {
				for (h = old; h < ctl->top; h++) {
					ctl->sheets[h] = ctl->sheets[h + 1];
					ctl->sheets[h]->height = h;
				}
			}
			ctl->top--; /* 表示中の下じきが一つ減るので、一番上の高さが減る */
			sheet_refreshmap( sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0);
			sheet_refreshsub(sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, 0, old - 1);
		}
	} else if (old < height) {	/* 以前よりも高くなる */
		if (old >= 0) {
			for (h = old; h < height; h++) {
				ctl->sheets[h] = ctl->sheets[h + 1];
				ctl->sheets[h]->height = h;
			}
			ctl->sheets[height] = sht;
		} else {
			for (h = ctl->top; h >= height; h--) {
				ctl->sheets[h + 1] = ctl->sheets[h];
				ctl->sheets[h + 1]->height = h + 1;
			}
			ctl->sheets[height] = sht;
			ctl->top++; /* 表示中の下じきが一つ増えるので、一番上の高さが増える */
		}
		sheet_refreshmap(sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height);
		sheet_refreshsub(sht->vx0, sht->vy0, sht->vx0 + sht->bxsize, sht->vy0 + sht->bysize, height, height);
	}
	return;
}
void sheet_refresh(struct SHEET *sht, int bx0, int by0, int bx1, int by1)
{
	if (sht->height >= 0) { /* もしも表示中なら、新しい下じきの情報に沿って画面を描き直す */
		sheet_refreshsub(sht->vx0 + bx0, sht->vy0 + by0, sht->vx0 + bx1, sht->vy0 + by1, sht->height, sht->height);
	}
	return;
}
void sheet_slide(struct SHEET *sht, int vx0, int vy0){
	int old_vx0 = sht->vx0, old_vy0 = sht->vy0;
	sht->vx0 = vx0;
	sht->vy0 = vy0;
	if (sht->height >= 0) { /* もしも表示中なら、新しい下じきの情報に沿って画面を描き直す */
		sheet_refreshmap(old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0);
		sheet_refreshmap( vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height);
		sheet_refreshsub( old_vx0, old_vy0, old_vx0 + sht->bxsize, old_vy0 + sht->bysize, 0, sht->height - 1);
		sheet_refreshsub( vx0, vy0, vx0 + sht->bxsize, vy0 + sht->bysize, sht->height, sht->height);
	}
	return;
}
void sheet_free(struct SHEET *sht){
	if (sht->height >= 0) {
		sheet_updown(sht, -1); /* 表示中ならまず非表示にする */
	}
	sht->flags = 0; /* 未使用マーク */
	return;
}
/*****************
 *     Timer     *
 *****************/
struct TIMERCTL timerctl;
void init_pit(void){
	int i;
	struct TIMER *t;
	io_out8(PIT_CTRL, 0x34);
	io_out8(PIT_CNT0, 0x9c);
	io_out8(PIT_CNT0, 0x2e);
	timerctl.count = 0;
	for (i = 0; i < MAX_TIMER; i++)timerctl.timers0[i].flags = 0;
	t = timer_alloc(); /* 一つもらってくる */
	t->timeout = 0xffffffff;
	t->flags = TIMER_FLAGS_USING;
	t->next = 0; /* 一番うしろ */
	timerctl.t0 = t; /* 今は番兵しかいないので先頭でもある */
	timerctl.next = 0xffffffff; /* 番兵しかいないので番兵の時刻 */
	return;
}
struct TIMER *timer_alloc(void){
	int i;
	for (i = 0; i < MAX_TIMER; i++) {
		if (timerctl.timers0[i].flags == 0) {
			timerctl.timers0[i].flags = TIMER_FLAGS_ALLOC;
			return &timerctl.timers0[i];
		}
	}
	return 0; /* 見つからなかった */
}
void timer_free(struct TIMER *timer){
	timer->flags = 0;
	return;
}
void timer_init(struct TIMER *timer, struct FIFO32 *fifo, int data){
	timer->fifo = fifo;
	timer->data = data;
	return;
}
void timer_settime(struct TIMER *timer, unsigned int timeout){
	int e;
	struct TIMER *t, *s;
	timer->timeout = timeout + timerctl.count;
	timer->flags = TIMER_FLAGS_USING;
	e = io_load_eflags();
	io_cli();
	t = timerctl.t0;
	if (timer->timeout <= t->timeout) {
		timerctl.t0 = timer;
		timer->next = t; /* 次はt */
		timerctl.next = timer->timeout;
		io_store_eflags(e);
		return;
	}
	for (;;) {
		s = t;
		t = t->next;
		if (timer->timeout <= t->timeout) {
			s->next = timer; /* sの次はtimer */
			timer->next = t; /* timerの次はt */
			io_store_eflags(e);
			return;
		}
	}
}
void inthandler20(int *esp){
	struct TIMER *timer;
	io_out8(PIC0_OCW2, 0x60);	/* IRQ-00受付完了をPICに通知 */
	timerctl.count++;
	char flag=0;
	if (timerctl.next > timerctl.count)return;
	timer = timerctl.t0; /* とりあえず先頭の番地をtimerに代入 */
	while (timer->timeout <= timerctl.count) {
		timer->flags = TIMER_FLAGS_ALLOC;
		if(timer!=task_timer)fifo32_put(timer->fifo, timer->data);
		else flag=1;
		timer = timer->next;
	}
	timerctl.t0 = timer;
	timerctl.next = timer->timeout;
	if(flag)schedule();
	return;
}
/*****************
 *     Mtask     *
 *****************/
Htask initmt(struct MEMMAN *memman) USE_STACK(12) {
	int i;
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;
	taskctl = (struct TASKCTL *) memman_alloc_4k(memman, sizeof (struct TASKCTL));
	for (i = 0; i < MAX_TASKS; i++) {
		taskctl->tasks0[i].flags = 0;
		taskctl->tasks0[i].tsss = (3 + i) * 8;
		set_segmdesc(gdt + 3 + i, 119, (int) &taskctl->tasks0[i], AR_TSS32);
	}
	Htask task = alloctask("System");
	task->flags = 2; /* 動作中マーク */
	taskctl->readycnt = 1;
	taskctl->now = 0;
	taskctl->ready[0] = task;
	load_tr(task->tsss);
	task_timer = timer_alloc();
	timer_settime(task_timer, 2);
	segcnt=1027;
	return task;
}
Htask alloctask(char* name) USE_STACK(8) {
	int i;Htask task;
	for (i = 0; i < MAX_TASKS; i++) {
		if (taskctl->tasks0[i].flags == 0) {
			task = &taskctl->tasks0[i];
			task->flags = 1; 
			task->tss.eflags = 0x00000202; /* IF = 1; */
			task->tss.eax = task->tss.ecx =task->tss.edx = task->tss.ebx = 0;
			task->tss.es = task->tss.ds = task->tss.fs = task->tss.gs = task->tss.ldtr = 0;
			task->tss.iomap = 0x40000000;
			task->tid=i;
			task->name=name;
			return task;
		}
	}
	return NULL;
}
void task_sleep(struct TASK *task){
	int i;
	char ts = 0;
	if (task->flags == 2) {
		if (task == taskctl->ready[taskctl->now])ts = 1;
		for (i = 0; i < taskctl->readycnt; i++) {
			if (taskctl->ready[i] == task)break;
		}
		taskctl->readycnt--;
		if (i < taskctl->now)taskctl->now--;
		for (; i < taskctl->readycnt; i++)taskctl->ready[i] = taskctl->ready[i + 1];
		task->flags = 1;
		if (ts != 0) {
			if (taskctl->now >= taskctl->readycnt)taskctl->now = 0;
			farjmp(0, taskctl->ready[taskctl->now]->tsss);
		}
	}
	return;
}
void task_delete(Htask task,Hfifo fifop){
	fifop->task=NULL;
	memman_free_4k(get_sys_memman(),task->fesp,64*1024);
	task_sleep(task);
	return;
}
void inittask(Htask task,int acs,int cs,int es){
	task->acs=acs;
	task->tss.cs=cs*8;
	task->tss.ds=task->tss.es=task->tss.fs=task->tss.gs=task->tss.ss=es*8;
	return;
}
void task_ready(Htask task){
	task->flags=2;
	taskctl->ready[taskctl->readycnt++]=task;
	return;
}
void schedule(){
	timer_settime(task_timer,2);
	if(taskctl->readycnt>=2){
		taskctl->now++;
		if(taskctl->now==taskctl->readycnt)taskctl->now=0;
		farjmp(0,taskctl->ready[taskctl->now]->tsss);
	}
	return;
}
//Mtask: Apps
int segcnt;

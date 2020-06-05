/* ----------- console.c ---------- */

#include "dflat.h"

/* ----- table of alt keys for finding shortcut keys ----- */
static int altconvert[] = {
    ALT_A,ALT_B,ALT_C,ALT_D,ALT_E,ALT_F,ALT_G,ALT_H,
    ALT_I,ALT_J,ALT_K,ALT_L,ALT_M,ALT_N,ALT_O,ALT_P,
    ALT_Q,ALT_R,ALT_S,ALT_T,ALT_U,ALT_V,ALT_W,ALT_X,
    ALT_Y,ALT_Z,ALT_0,ALT_1,ALT_2,ALT_3,ALT_4,ALT_5,
    ALT_6,ALT_7,ALT_8,ALT_9
};

unsigned video_mode;
unsigned video_page;

static int near cursorpos[MAXSAVES];
static int near cursorshape[MAXSAVES];
static int cs;

static union REGS regs;

/* ------------- clear the screen -------------- */
void clearscreen(void)
{
	int ht = SCREENHEIGHT;
	int wd = SCREENWIDTH;
	cursor(0, 0);
	regs.h.al = ' ';
	regs.h.ah = 9;
	regs.x.bx = 7;
	regs.x.cx = ht * wd;
	int86(VIDEO, &regs, &regs);
}

void SwapCursorStack(void)
{
	if (cs > 1)	{
		swap(cursorpos[cs-2], cursorpos[cs-1]);
		swap(cursorshape[cs-2], cursorshape[cs-1]);
	}
}

#ifdef __SMALLER_C__
#else
#ifndef MSC
#ifndef WATCOM
#define ZEROFLAG 0x40
/* ---- Test for keystroke ---- */
BOOL keyhit(void)
{
    _AH = 1;
    geninterrupt(KEYBRD);
    return (_FLAGS & ZEROFLAG) == 0;
}
#endif
#endif
#endif

/* ---- Read a keystroke ---- */
int getkey(void)
{
    int c;
    while (keyhit() == FALSE)
        ;
    if (((c = bioskey(0)) & 0xff) == 0)
        c = (c >> 8) | 0x1080;
    else
        c &= 0xff;
    return c & 0x10ff;
}

/* ---------- read the keyboard shift status --------- */
int getshift(void)
{
    regs.h.ah = 2;
    int86(KEYBRD, &regs, &regs);
    return regs.h.al;
}

static volatile int far *clk = MK_FP(0x40,0x6c);
/* ------- macro to wait one clock tick -------- */
#define wait()          \
{                       \
    int now = *clk;     \
    while (now == *clk) \
        ;               \
}

/* -------- sound a buzz tone ---------- */
void beep(void)
{
    wait();
    outp(0x43, 0xb6);               /* program the frequency */
    outp(0x42, (int) (COUNT % 256));
    outp(0x42, (int) (COUNT / 256));
    outp(0x61, inp(0x61) | 3);      /* start the sound */
    wait();
    outp(0x61, inp(0x61) & ~3);     /* stop the sound  */
}

/* -------- get the video mode and page from BIOS -------- */
void videomode(void)
{
    regs.h.ah = 15;
    int86(VIDEO, &regs, &regs);
    video_mode = regs.h.al;
    video_page = regs.x.bx;
    video_page &= 0xff00;
    video_mode &= 0x7f;
}

/* ------ position the cursor ------ */
void cursor(int x, int y)
{
    videomode();
    regs.x.dx = ((y << 8) & 0xff00) + x;
    regs.h.ah = SETCURSOR;
    regs.x.bx = video_page;
    int86(VIDEO, &regs, &regs);
}

/* ------ get cursor shape and position ------ */
static void near getcursor(void)
{
    videomode();
    regs.h.ah = READCURSOR;
    regs.x.bx = video_page;
    int86(VIDEO, &regs, &regs);
}

/* ------- get the current cursor position ------- */
void curr_cursor(int *x, int *y)
{
    getcursor();
    *x = regs.h.dl;
    *y = regs.h.dh;
}

/* ------ save the current cursor configuration ------ */
void savecursor(void)
{
    if (cs < MAXSAVES)    {
        getcursor();
        cursorshape[cs] = regs.x.cx;
        cursorpos[cs] = regs.x.dx;
        cs++;
    }
}

/* ---- restore the saved cursor configuration ---- */
void restorecursor(void)
{
    if (cs)    {
        --cs;
        videomode();
        regs.x.dx = cursorpos[cs];
        regs.h.ah = SETCURSOR;
        regs.x.bx = video_page;
        int86(VIDEO, &regs, &regs);
        set_cursor_type(cursorshape[cs]);
    }
}

/* ------ make a normal cursor ------ */
void normalcursor(void)
{
    set_cursor_type(0x0607);
}

/* ------ hide the cursor ------ */
void hidecursor(void)
{
    getcursor();
    regs.h.ch |= HIDECURSOR;
    regs.h.ah = SETCURSORTYPE;
    int86(VIDEO, &regs, &regs);
}

/* ------ unhide the cursor ------ */
void unhidecursor(void)
{
    getcursor();
    regs.h.ch &= ~HIDECURSOR;
    regs.h.ah = SETCURSORTYPE;
    int86(VIDEO, &regs, &regs);
}

/* ---- use BIOS to set the cursor type ---- */
void set_cursor_type(unsigned t)
{
    videomode();
    regs.h.ah = SETCURSORTYPE;
    regs.x.bx = video_page;
    regs.x.cx = t;
    int86(VIDEO, &regs, &regs);
}

/* ---- test for EGA -------- */
BOOL isEGA(void)
{
    if (isVGA())
        return FALSE;
    regs.h.ah = 0x12;
    regs.h.bl = 0x10;
    int86(VIDEO, &regs, &regs);
    return regs.h.bl != 0x10;
}

/* ---- test for VGA -------- */
BOOL isVGA(void)
{
    regs.x.ax = 0x1a00;
    int86(VIDEO, &regs, &regs);
    return regs.h.al == 0x1a && regs.h.bl > 6;
}

static void Scan350(void)
{
    regs.x.ax = 0x1201;
    regs.h.bl = 0x30;
    int86(VIDEO, &regs, &regs);
	regs.h.ah = 0x0f;
    int86(VIDEO, &regs, &regs);
	regs.h.ah = 0x00;
    int86(VIDEO, &regs, &regs);
}

static void Scan400(void)
{
    regs.x.ax = 0x1202;
    regs.h.bl = 0x30;
    int86(VIDEO, &regs, &regs);
	regs.h.ah = 0x0f;
    int86(VIDEO, &regs, &regs);
	regs.h.ah = 0x00;
    int86(VIDEO, &regs, &regs);
}

/* ---------- set 25 line mode ------- */
void Set25(void)
{
    if (isVGA())	{
        Scan400();
		regs.x.ax = 0x1114;
	}
	else
		regs.x.ax = 0x1111;
    regs.h.bl = 0;
    int86(VIDEO, &regs, &regs);
}

/* ---------- set 43 line mode ------- */
void Set43(void)
{
    if (isVGA())
        Scan350();
    regs.x.ax = 0x1112;
    regs.h.bl = 0;
    int86(VIDEO, &regs, &regs);
}

/* ---------- set 50 line mode ------- */
void Set50(void)
{
    if (isVGA())
        Scan400();
    regs.x.ax = 0x1112;
    regs.h.bl = 0;
    int86(VIDEO, &regs, &regs);
}

/* ------ convert an Alt+ key to its letter equivalent ----- */
int AltConvert(int c)
{
	int i, a = 0;
	for (i = 0; i < 36; i++)
		if (c == altconvert[i])
			break;
	if (i < 26)
		a = 'a' + i;
	else if (i < 36)
		a = '0' + i - 26;
	return a;
}

#if MSC | WATCOM
int getdisk(void)
{
	unsigned int cd;
	_dos_getdrive(&cd);
	cd -= 1;
	return cd;
}
#endif

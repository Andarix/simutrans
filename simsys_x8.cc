#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#include <sys/shm.h>
#include <sys/time.h>
#include <X11/extensions/XShm.h>

#include "simsys.h"
#include "simversion.h"


static int using_shm = FALSE;
static int doing_sync = TRUE;

static int display_depth = 8;
static int is_truecolor = FALSE;

static Cursor standard_cursor;
static Cursor invisible_cursor;

static unsigned long planetab[32];
static unsigned long coltab[256];
static unsigned long colortrans[256];

static Display* md; /* global fuer die Zeichenroutinen */
static Window   mw;
static GC       mgc;
static int      ms;
static unsigned long mbg; /* Vorder/Hintergrund */
static unsigned long mfg;
static XSizeHints mh;
static Colormap cmap;

static int width  = 640;
static int height = 480;


// Time at program start
static struct timeval start;


static void init_cursors(void)
{
	static const char bits[] = {
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
	};

	Pixmap pix;
	XColor cfg;

	// create invisible cursor
	cfg.red = cfg.green = cfg.blue = 0;
	pix = XCreateBitmapFromData(md, mw, bits, 8, 8);
	invisible_cursor = XCreatePixmapCursor(md, pix, pix, &cfg, &cfg, 0, 0);
	XFreePixmap(md, pix);

	// create standard cursor
	standard_cursor = XCreateFontCursor(md, 130);
}


int dr_os_init(const int* parameter)
{
	using_shm  = parameter[0];
	doing_sync = parameter[1];

	// init time count
	gettimeofday(&start, NULL);

	return TRUE;
}


int dr_os_open(int const w, int const h, int)
{
	XSetWindowAttributes attr;

	width = w;
	height = h;

	md = XOpenDisplay(NULL);
	if (md == NULL) {
		fprintf(stderr, "Kann Display nicht oeffnen!");
		exit(1);
	}

	ms = DefaultScreen(md);

	display_depth = DefaultDepth(md, ms);

	fprintf(stderr, "Using %d bpp\n", display_depth);

	if (display_depth == 8) {
		is_truecolor = FALSE;
	} else if(display_depth >= 15) {
		is_truecolor = TRUE;
		fprintf(stderr,"Warning: using experimental HiColor/TrueColor mode\n");
	} else {
		fprintf(
			stderr,
			"Problem: Simutrans braucht ein 8 Bit pseudo color visual (256 Farben mit Palette)\n"
			"         oder ein direct color/true color visual (HiColor)\n"
		);
		exit(2);
	}

	mbg = WhitePixel(md, ms);
	mfg = BlackPixel(md, ms);

	mh.x = 0; /* Fenster sollte im sichtbaren Bereich liegen */
	mh.y = 0;
	mh.width  = width;
	mh.height = height;

	// set window size hints
	// min_size = size = max_size  -> no resizing
	mh.min_width  = mh.max_width  = width;
	mh.min_height = mh.max_height = height;

	mh.flags = PSize | PMinSize | PMaxSize;

	fprintf(stderr, "Opening program window ...\n");

	mw = XCreateSimpleWindow(
		md, DefaultRootWindow(md),
		mh.x, mh.y, mh.width, mh.height, 5,
		mfg, mbg
	);

	XSetStandardProperties(md, mw, SIM_TITLE, "Simu", None, NULL, 0, NULL);

	mgc =XCreateGC(md, mw, 0, 0);
	XSetBackground(md, mgc, mbg);
	XSetForeground(md, mgc, mfg);

	XSelectInput(
		md, mw,
		VisibilityChangeMask |
		ButtonPressMask |
		ButtonReleaseMask |
		KeyPressMask |
		KeyReleaseMask |
		PointerMotionMask
	);

	attr.backing_store = Always;
	XChangeWindowAttributes(md, mw, CWBackingStore, &attr);

	if (!is_truecolor) {
		cmap = XCreateColormap(md, mw, DefaultVisual(md, ms), AllocNone);
		XAllocColorCells(md, cmap, 0, planetab, 0, coltab, 256);
		XSetWindowColormap(md, mw, cmap);
	}

	XMapRaised(md, mw);

	init_cursors();

	XDefineCursor(md, mw, standard_cursor);

	sys_event.mx = width  / 2;
	sys_event.my = height / 2;

	XSetForeground(md, mgc, 1);

	return w;
}


void dr_os_close()
{
	XCloseDisplay(md);
}


static XImage* texturimg;
static XShmSegmentInfo xshminfo;

// this is used if we need to fake an 8 bit array
static unsigned char* data8;


int dr_textur_resize(unsigned short**, int, int)
{
	// XXX TODO implement
	return 0;
}


unsigned short* dr_textur_init(void)
{
	int depth = display_depth;

	if (!using_shm) {
		if (depth == 24) depth = 32;
		if (depth == 15) depth = 16;

		texturimg = XCreateImage(
			md, DefaultVisual(md, ms),
			display_depth, ZPixmap, 0,
			malloc(width * height * (depth / 8)),
			width, height, depth, width * (depth / 8)
		);

		printf("Texturimage: %p\n, ", texturimg);

	} else {
		if (depth == 24) depth = 32;

		xshminfo.shmid = shmget(
			IPC_PRIVATE, width * height * (depth / 8), IPC_CREAT | 0777
		);

		texturimg = XShmCreateImage(
			md, DefaultVisual(md, ms),
			display_depth, ZPixmap, NULL, &xshminfo,
			width, height
		);

		printf("Texturimage: %p, shmid: %d\n", texturimg, xshminfo.shmid);

		texturimg->data = (char*)shmat(xshminfo.shmid, 0, 0);
		printf("Data: %p\n", texturimg->data);

		xshminfo.shmaddr = texturimg->data;
		xshminfo.readOnly = False;

		printf("Attach: %d\n", XShmAttach(md, &xshminfo));
	}

	if (is_truecolor) {
		// fake an 8 bit array and convert data before displaying
		data8 = malloc(width * height);
		return data8;
	} else {
		return texturimg->data;
	}
}


void dr_textur(int xp, int yp, int w, int h)
{
	// clipping unten
	if (yp + h > height) h = height - yp;

	// if we are in truecolor we need to convert our image data
	if (is_truecolor) {
		const unsigned char * src = data8 + xp + yp * width;
		const int skip = width - w;

		if (display_depth <= 16) {
			unsigned short * dest = (unsigned short*)texturimg->data + xp + yp * width;
			int hc = h;

			do {
				const unsigned char* const end = src + w;

				do {
					*dest++ = colortrans[*src++];
					*dest++ = colortrans[*src++];
					*dest++ = colortrans[*src++];
					*dest++ = colortrans[*src++];
				} while (src < end);

				dest += skip;
				src  += skip;
			} while (--hc > 0);
		} else {
			unsigned int* dest = (unsigned int*)texturimg->data + xp + yp * width;
			int hc = h;

			do {
				const unsigned char* const end = src + w;

				do {
					*dest++ = colortrans[*src++];
					*dest++ = colortrans[*src++];
					*dest++ = colortrans[*src++];
					*dest++ = colortrans[*src++];
				} while(src < end);

				dest += skip;
				src  += skip;
			} while (--hc > 0);
		}
	}

	if (using_shm) {
#if 0
		printf("%d %d %d %d\n", xp, yp, w, h);
#endif
		XShmPutImage(md, mw, mgc, texturimg, xp, yp, xp, yp, w, h, 0);
	} else {
		XPutImage(md, mw, mgc, texturimg, xp, yp, xp, yp, w, h);
	}
}


void dr_flush(void)
{
	if (doing_sync) XSync(md, FALSE);
}


void dr_setRGB8multi(int first, int count, unsigned char* data)
{
	int n;

	for (n = 0; n < count; n++) {
		XColor xc;

		xc.pixel = n + first;
		xc.flags = DoRed | DoGreen | DoBlue;
		xc.red   = data[n * 3 + 0] << 8;
		xc.green = data[n * 3 + 1] << 8;
		xc.blue  = data[n * 3 + 2] << 8;

		if (is_truecolor) {
			XAllocColor(md, DefaultColormap(md, ms), &xc);

#if 0
			printf("Storing color %d with value %lx\n", n, xc.pixel);
#endif
			colortrans[n] = xc.pixel;
		} else {
			XStoreColor(md, cmap, &xc);
		}
	}
	XFlush(md);
}


void show_pointer(int yesno)
{
	XDefineCursor(md, mw, yesno ? standard_cursor : invisible_cursor);
}


void move_pointer(int x, int y)
{
	XWarpPointer(md, None, mw, 0, 0, width, height, x, y);
}


void set_pointer(int loading)
{
	// XXX TODO implement
}


int dr_screenshot(const char *filename)
{
	// XXX TODO implement
	return 0;
}


/*
 * Hier sind die Funktionen zur Messageverarbeitung
 */
static void internal_GetEvents(int wait)
{
	XEvent event;

	if (wait) {
		do {
			XNextEvent(md, &event);
		} while (
			event.type == GraphicsExpose ||
			event.type == NoExpose ||
			(XEventsQueued(md, QueuedAlready) > 0 && event.type == MotionNotify)
		);
	} else {
		do {
			XNextEvent(md, &event);
		} while (
			event.type == GraphicsExpose ||
			event.type == NoExpose ||
			(XEventsQueued(md, QueuedAfterFlush) > 0 && event.type == MotionNotify)
		);
	}

	switch (event.type) {
		case ButtonPress:
			sys_event.type = SIM_MOUSE_BUTTONS;
			sys_event.mx   = event.xbutton.x;
			sys_event.my   = event.xbutton.y;
			switch (event.xbutton.button) {
				case 1: sys_event.code = SIM_MOUSE_LEFTBUTTON;  break;
				case 2: sys_event.code = SIM_MOUSE_MIDBUTTON;   break;
				case 3: sys_event.code = SIM_MOUSE_RIGHTBUTTON; break;
				case 4: sys_event.code = SIM_MOUSE_WHEELUP;     break;
				case 5: sys_event.code = SIM_MOUSE_WHEELDOWN;   break;
			}
			break;

		case ButtonRelease:
			sys_event.type = SIM_MOUSE_BUTTONS;
			sys_event.mx   = event.xbutton.x;
			sys_event.my   = event.xbutton.y;
			switch (event.xbutton.button) {
				case 1: sys_event.code = SIM_MOUSE_LEFTUP;  break;
				case 2: sys_event.code = SIM_MOUSE_MIDUP;   break;
				case 3: sys_event.code = SIM_MOUSE_RIGHTUP; break;
			}
			break;

		case KeyPress: {
			KeySym mykey;
			char text[4];

			sys_event.type = SIM_KEYBOARD;

			if (XLookupString(&event.xkey, text, 1, &mykey, 0)) {
				sys_event.code = text[0];
			} else {
				sys_event.code = 0;
			}
			break;
		}

		case MotionNotify:
			sys_event.type = SIM_MOUSE_MOVE;
			sys_event.code = SIM_MOUSE_MOVED;
			sys_event.mx   = event.xmotion.x;
			sys_event.my   = event.xmotion.y;
			break;

		case VisibilityNotify:
			dr_textur(0, 0, width, height);
			sys_event.type = SIM_IGNORE_EVENT;
			sys_event.code = 0;
			break;

		case KeyRelease:
			sys_event.type = SIM_IGNORE_EVENT;
			sys_event.code = 0;
			break;

		default:
			sys_event.type = SIM_IGNORE_EVENT;
			sys_event.code = 0;
			break;
	}
}


void GetEvents(void)
{
	internal_GetEvents(TRUE);
}

void GetEventsNoWait(void)
{
	int n = XEventsQueued(md, QueuedAfterFlush);

	sys_event.type = SIM_NOEVENT;
	sys_event.code = 0;

	if (n > 0) internal_GetEvents(FALSE);
}


void ex_ord_update_mx_my(void)
{
	// wird fuer X nicht benoetigt
}


unsigned long dr_time(void)
{
	struct timeval now;

	gettimeofday(&now, NULL);
	return
		(now.tv_sec  - start.tv_sec)  * 1000 +
		(now.tv_usec - start.tv_usec) / 1000;
}


void dr_sleep(unsigned long usec)
{
	if (usec > 1000) usleep(usec);
}


int main(int argc, char **argv)
{
	return sysmain(argc, argv);
}

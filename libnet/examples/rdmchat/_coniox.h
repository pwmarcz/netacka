#if defined TARGET_ALLEGRO

#include <allegro.h>

int _conio_x, _conio_y, _conio_c, _conio_h;

inline void conio_init (void) { allegro_init(); set_gfx_mode (GFX_SAFE, 640, 480, 0, 0); install_keyboard(); _conio_x = _conio_y = 0; _conio_c = 15; _conio_h = 25; }
inline void conio_exit (void) { }
inline int conio_kbhit (void) { return keypressed(); }
inline int conio_getch (void) { return readkey() & 0xFF; }
inline void conio_gotoxy (int x, int y) { _conio_x = x; _conio_y = y; }
inline void conio_clreol (void) { int y = _conio_y * text_height(font); rectfill (screen, _conio_x * text_length(font, " "), y, SCREEN_W, y+text_height(font), 0); }
inline void conio_textcolor (int c) { _conio_c = c; }
inline void conio_cputs (char *s) { if (*s == '\n') { _conio_x = 0; _conio_y++; if (_conio_y > _conio_h) { _conio_y--; blit (screen, screen, 0, text_height(font), 0, 0, SCREEN_W, 25*text_height(font)); } } else { textout (screen, font, s, _conio_x * text_length(font, " "), _conio_y * text_height(font), _conio_c); _conio_x += strlen(s); } }

#elif defined TARGET_DJGPP

#include <conio.h>

inline void conio_init (void) { clrscr(); }
inline void conio_exit (void) { clrscr(); }
inline int conio_kbhit (void) { return kbhit(); }
inline int conio_getch (void) { return getch(); }
inline void conio_gotoxy (int x, int y) { gotoxy (x, y); }
inline void conio_clreol (void) { clreol(); }
inline void conio_textcolor (int c) { textcolor (c); }
inline void conio_cputs (char *s) { if (*s == '\n') cputs ("\r"); cputs (s); }

#elif defined TARGET_MSVC

void conio_init (void);
void conio_exit (void);
int conio_kbhit (void);
int conio_getch (void);
void conio_gotoxy (int x, int y);
void conio_clreol (void);
void conio_textcolor (int c);
void conio_cputs (char *s);

#else

#include <ncurses.h>

static int __gf__cached__gf__ = 0, __gf__cached_value__gf__ = 0;

inline void conio_init (void)
{
	int x,y;
	initscr();
	start_color();
	cbreak();
	noecho();
	nonl();
//	intrflush (stdscr, FALSE);
	keypad (stdscr, TRUE);
	nodelay (stdscr, TRUE);
	scrollok (stdscr, TRUE);
}

inline void conio_exit (void)
{
	endwin();
}

inline int conio_kbhit (void)
{
  if (__gf__cached__gf__) return 1;
  __gf__cached_value__gf__ = getch();
  if (__gf__cached_value__gf__ != ERR) __gf__cached__gf__ = 1;
  return __gf__cached__gf__;
}

inline int conio_getch (void)
{
  if (__gf__cached__gf__) {
	__gf__cached__gf__ = 0;
	return __gf__cached_value__gf__;
  } else
	return getch();
}

inline void conio_gotoxy (int x, int y) { move (y-1, x-1); }

inline void conio_clreol (void) { clrtoeol(); }

inline void conio_textcolor (int c) 
{
	int colours[16] = {
             COLOR_BLACK,
             COLOR_BLUE,
             COLOR_GREEN,
             COLOR_CYAN,
             COLOR_RED,
             COLOR_MAGENTA,
             COLOR_YELLOW,
             COLOR_WHITE,
             COLOR_BLACK,
             COLOR_BLUE,
             COLOR_GREEN,
             COLOR_CYAN,
             COLOR_RED,
             COLOR_MAGENTA,
             COLOR_YELLOW,
             COLOR_WHITE
	};

	init_pair (colours[c], colours[c], 0);
	attrset (COLOR_PAIR(colours[c]));
	if (c >= 8) attron (A_BOLD);
}

inline void conio_cputs (char *s) { addstr (s); }

#endif

#ifndef END_OF_MAIN
#define END_OF_MAIN()
#endif


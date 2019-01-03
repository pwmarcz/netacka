#ifndef _UI_H
#define _UI_H

#include <allegro.h>
#include "nuklear_flags.h"
#include "nuklear.h"

extern struct nk_context ui;

void ui_init();
void ui_handle_input();
void ui_draw(BITMAP *bmp);
void ui_demo();
void ui_shutdown();

#endif

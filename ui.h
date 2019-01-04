#ifndef _UI_H
#define _UI_H

#include <allegro.h>
#include "nuklear_flags.h"
#include "nuklear.h"

extern struct nk_context ui;
extern struct nk_style_button ui_style_button_important;

extern const struct nk_color UI_BLACK;
extern const struct nk_color UI_DARK;
extern const struct nk_color UI_MEDIUM;
extern const struct nk_color UI_LIGHT;
extern const struct nk_color UI_VERY_LIGHT;
extern const struct nk_color UI_WHITE;

void ui_init();
void ui_handle_input();
void ui_draw(BITMAP *bmp);
void ui_demo();
void ui_shutdown();

#endif

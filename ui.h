#ifndef _UI_H
#define _UI_H

#include <allegro.h>
#include "gui/gui.h"

extern struct ui {
    void *memory;
    struct gui_command_queue queue;
    struct gui_font font;
    struct gui_config config;
    struct gui_input input;
} ui;

void ui_init();
void ui_handle_input();
void ui_draw(BITMAP *bmp);
void ui_demo();
void ui_shutdown();

#endif

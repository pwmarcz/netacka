
#include <stdlib.h>
#include <string.h>
#include <allegro.h>

#include "gui/gui.h"

#include "ui.h"

#define MEMORY_SIZE 1024

static int color(struct gui_color c) {
    return makeacol(c.r, c.g, c.b, c.a);
}

static void scissor(BITMAP *bmp, float x, float y, float w, float h) {
    set_clip_rect(bmp, x, y, x+w, y+h);
}

static gui_size
font_get_width(gui_handle handle, const gui_char *text, gui_size len) {
    return len * 8;
}

static void
draw_rect(BITMAP *bmp, float x, float y, float w, float h, float r, struct gui_color c)
{
    rectfill(bmp, x, y, x+w, y+h, color(c));
}

static void
draw_text(BITMAP *bmp, float x, float y, float w, float h,
    struct gui_color c, struct gui_color bg, const gui_char *string, gui_size len)
{
    rectfill(bmp, x, y, x+w, y+h, color(bg));
    textout_ex(bmp, font, string, x, y + h / 2 - 8/2, color(c), -1);
}

static void
draw_line(BITMAP *bmp, float x0, float y0, float x1, float y1, struct gui_color c)
{
    line(bmp, x0, y0, x1, y1, color(c));
}

static void
draw_circle(BITMAP *bmp, float x, float y, float r, struct gui_color c)
{
    circlefill(bmp, x + r, y + r, r, color(c));
}

static void
draw_triangle(BITMAP *bmp, float x0, float y0, float x1, float y1,
    float x2, float y2, struct gui_color c)
{
    triangle(bmp, x0, y0, x1, y1, x2, y2, color(c));
}

static void
draw_image(BITMAP *bmp, gui_handle img, float x, float y, float w, float h, float r)
{
    // ...
}

struct ui ui;

void ui_init() {
    ui.memory = malloc(MEMORY_SIZE);
    gui_command_queue_init_fixed(&ui.queue, ui.memory, MEMORY_SIZE);

    // font.userdata.ptr = ...;
    ui.font.height = 8;
    ui.font.width = font_get_width;
    gui_config_default(&ui.config, GUI_DEFAULT_ALL, &ui.font);
    gui_config_push_color(&ui.config, GUI_COLOR_PANEL, gui_rgba(0, 0, 0, 255));
    gui_config_push_color(&ui.config, GUI_COLOR_TEXT, gui_rgba(180, 180, 180, 255));
    gui_config_push_property(&ui.config, GUI_PROPERTY_ITEM_SPACING, ((struct gui_vec2) {2, 2}));
    gui_config_push_property(&ui.config, GUI_PROPERTY_ITEM_PADDING, ((struct gui_vec2) {2, 2}));
    gui_config_push_property(&ui.config, GUI_PROPERTY_PADDING, ((struct gui_vec2) {5, 5}));

}


void ui_demo() {
    struct gui_panel panel;
    gui_panel_init(&panel, 50, 50, 220, 180,
                   GUI_PANEL_BORDER|GUI_PANEL_MOVEABLE|GUI_PANEL_SCALEABLE,
                   &ui.queue, &ui.config, &ui.input);

    enum {EASY, HARD};
    gui_size option = EASY;
    gui_size item = 0;

    struct gui_panel_layout layout;
    gui_panel_begin(&layout, &panel);
    {
        const char *items[] = {"Fist", "Pistol", "Railgun", "BFG"};
        gui_panel_row_static(&layout, 20, 80, 1);
        if (gui_panel_button_text(&layout, "button", GUI_BUTTON_DEFAULT)) {
            /* event handling */
        }
        gui_panel_row_dynamic(&layout, 20, 2);
        if (gui_panel_option(&layout, "easy", option == EASY)) option = EASY;
        if (gui_panel_option(&layout, "hard", option == HARD)) option = HARD;
        gui_panel_label(&layout, "Weapon:", GUI_TEXT_LEFT);
        item = gui_panel_selector(&layout, items, 4, item);
    }
    gui_panel_end(&layout, &panel);

    ui_draw(screen);
    readkey();
}

void ui_draw(BITMAP *bmp) {
    const struct gui_command *cmd;
    gui_foreach_command(cmd, &ui.queue) {
        switch (cmd->type) {
            case GUI_COMMAND_NOP: break;
            case GUI_COMMAND_SCISSOR: {
                const struct gui_command_scissor *s = gui_command(scissor, cmd);
                scissor(bmp, s->x, s->y, s->w, s->h);
            } break;
            case GUI_COMMAND_LINE: {
                const struct gui_command_line *l = gui_command(line, cmd);
                draw_line(bmp, l->begin.x, l->begin.y, l->end.x, l->end.y, l->color);
            } break;
            case GUI_COMMAND_RECT: {
                const struct gui_command_rect *r = gui_command(rect, cmd);
                draw_rect(bmp, r->x, r->y, r->w, r->h, r->rounding, r->color);
            } break;
            case GUI_COMMAND_CIRCLE: {
                const struct gui_command_circle *c = gui_command(circle, cmd);
                draw_circle(bmp, c->x, c->y, (float)c->w / 2.0f, c->color);
            } break;
            case GUI_COMMAND_TRIANGLE: {
                const struct gui_command_triangle *t = gui_command(triangle, cmd);
                draw_triangle(bmp, t->a.x, t->a.y, t->b.x, t->b.y, t->c.x,
                              t->c.y, t->color);
            } break;
            case GUI_COMMAND_TEXT: {
                const struct gui_command_text *t = gui_command(text, cmd);
                draw_text(bmp, t->x, t->y, t->w, t->h, t->foreground, t->background, t->string, t->length);
            } break;
            case GUI_COMMAND_IMAGE: {
                const struct gui_command_image *i = gui_command(image, cmd);
                draw_image(bmp, i->img.handle, i->x, i->y, i->w, i->h, 1);
            } break;
            case GUI_COMMAND_MAX:
            default: break;
        }
    }
    gui_command_queue_clear(&ui.queue);
    set_clip_rect(bmp, 0, 0, -1, -1);
}

void ui_shutdown() {
    free(ui.memory);
}

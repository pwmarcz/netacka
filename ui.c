
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <allegro.h>

#include "ui.h"

#define MEMORY_SIZE 1024*1024

static int color(struct nk_color c) {
    return makeacol(c.r, c.g, c.b, c.a);
}

static void scissor(BITMAP *bmp, float x, float y, float w, float h) {
    set_clip_rect(bmp, x, y, x+w, y+h);
}

static float
font_get_width(nk_handle handle, float height, const char *text, int len) {
    return len * 8;
}

static void
draw_text(BITMAP *bmp, float x, float y, float w, float h,
    struct nk_color c, struct nk_color bg, const nk_char *string, nk_size len)
{
    rectfill(bmp, x, y, x+w, y+h, color(bg));
    textout_ex(bmp, font, string, x, y, color(c), -1);
}

static void
draw_image(BITMAP *bmp, nk_handle img, float x, float y, float w, float h, float r)
{
    // ...
}

struct nk_context ui;

static struct nk_user_font ui_font;

void ui_init() {
    ui_font.height = 8;
    ui_font.width = font_get_width;

    nk_init_default(&ui, &ui_font);

    /*
    // font.userdata.ptr = ...;
    nk_config_default(&ui.config, NK_DEFAULT_ALL, &ui.font);
    nk_config_push_color(&ui.config, NK_COLOR_PANEL, nk_rgba(0, 0, 0, 255));
    nk_config_push_color(&ui.config, NK_COLOR_TEXT, nk_rgba(180, 180, 180, 255));
    nk_config_push_property(&ui.config, NK_PROPERTY_ITEM_SPACING, ((struct nk_vec2) {2, 2}));
    nk_config_push_property(&ui.config, NK_PROPERTY_ITEM_PADDING, ((struct nk_vec2) {2, 2}));
    nk_config_push_property(&ui.config, NK_PROPERTY_PADDING, ((struct nk_vec2) {5, 5}));
    */
}


void ui_demo() {
    enum {EASY, HARD};
    int option = EASY;
    int item = 0;

    nk_begin(&ui, "Demo", nk_rect(50, 50, 200, 200),
             NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
             NK_WINDOW_CLOSABLE|NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE);
    {
        const char *items[] = {"Fist", "Pistol", "Railgun", "BFG"};
        nk_layout_row_static(&ui, 20, 80, 1);
        if (nk_button_label(&ui, "button")) {
            /* event handling */
        }
        nk_layout_row_dynamic(&ui, 20, 2);
        if (nk_option_label(&ui, "easy", option == EASY)) option = EASY;
        if (nk_option_label(&ui, "hard", option == HARD)) option = HARD;

        nk_layout_row_dynamic(&ui, 22, 1);
        nk_property_int(&ui, "Compression:", 0, &item, 100, 10, 1);
    }
    nk_end(&ui);

    ui_draw(screen);
    readkey();
}

void ui_draw(BITMAP *bmp) {
    const struct nk_command *cmd;
    nk_foreach(cmd, &ui) {
        switch (cmd->type) {
            case NK_COMMAND_NOP: break;
            case NK_COMMAND_SCISSOR: {
                const struct nk_command_scissor *s = (const struct nk_command_scissor*) cmd;
                scissor(bmp, s->x, s->y, s->w, s->h);
            } break;
            case NK_COMMAND_LINE: {
                const struct nk_command_line *l = (const struct nk_command_line*) cmd;
                line(bmp, l->begin.x, l->begin.y, l->end.x, l->end.y, color(l->color));
            } break;
            case NK_COMMAND_RECT: {
                const struct nk_command_rect *r = (const struct nk_command_rect*) cmd;
                rect(bmp, r->x, r->y, r->x + r->w, r->y + r->h, color(r->color));
            } break;
            case NK_COMMAND_RECT_FILLED: {
                const struct nk_command_rect_filled *r = (const struct nk_command_rect_filled*) cmd;
                rectfill(bmp, r->x, r->y, r->x + r->w, r->y + r->h, color(r->color));
            } break;
            case NK_COMMAND_CIRCLE: {
                const struct nk_command_circle *c = (const struct nk_command_circle*) cmd;
                circle(bmp, c->x + c->w / 2, c->y + c->w / 2, (float)c->w / 2.0f, color(c->color));
            } break;
            case NK_COMMAND_CIRCLE_FILLED: {
                const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled*) cmd;
                circlefill(bmp, c->x + c->w / 2, c->y + c->w / 2, (float)c->w / 2.0f, color(c->color));
            } break;
            case NK_COMMAND_TRIANGLE: {
                const struct nk_command_triangle *c = (const struct nk_command_triangle*)cmd;
                triangle(bmp, c->a.x, c->a.y, c->b.x, c->b.y, c->c.x, c->c.y, color(c->color));
            }
            case NK_COMMAND_TRIANGLE_FILLED: {
                const struct nk_command_triangle_filled *c = (const struct nk_command_triangle_filled*)cmd;
                triangle(bmp, c->a.x, c->a.y, c->b.x, c->b.y, c->c.x, c->c.y, color(c->color));
            }
            case NK_COMMAND_TEXT: {
                const struct nk_command_text *t = (const struct nk_command_text*) cmd;
                draw_text(bmp, t->x, t->y, t->w, t->h, t->foreground, t->background, t->string, t->length);
            } break;
            case NK_COMMAND_IMAGE: {
                const struct nk_command_image *i = (const struct nk_command_image*) cmd;
                draw_image(bmp, i->img.handle, i->x, i->y, i->w, i->h, 1);
            } break;
            default: break;
        }
    }
    nk_clear(&ui);
    set_clip_rect(bmp, 0, 0, -1, -1);
}

void ui_shutdown() {
    nk_free(&ui);
    memset(&ui, 0, sizeof(ui));
}



void ui_handle_input() {
    nk_input_begin(&ui);

    nk_input_motion(&ui, mouse_x, mouse_y);
    nk_input_button(&ui, NK_BUTTON_LEFT, mouse_x, mouse_y, mouse_b & 1);
    nk_input_button(&ui, NK_BUTTON_RIGHT, mouse_x, mouse_y, mouse_b & 2);

    nk_input_key(&ui, NK_KEY_SHIFT, key[KEY_LSHIFT]);
    nk_input_key(&ui, NK_KEY_DEL, key[KEY_DEL]);
    nk_input_key(&ui, NK_KEY_ENTER, key[KEY_ENTER]);
    nk_input_key(&ui, NK_KEY_BACKSPACE, key[KEY_BACKSPACE]);
    nk_input_key(&ui, NK_KEY_LEFT, key[KEY_LEFT]);
    nk_input_key(&ui, NK_KEY_RIGHT, key[KEY_RIGHT]);
    while (keypressed()) {
        int val = ureadkey(NULL);
        if (val > 0x20 && val < 0x80) {
            nk_input_char(&ui, val);
        }
    }
    clear_keybuf();

    nk_input_end(&ui);
}

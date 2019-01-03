
#include <stdlib.h>
#include <string.h>
#include <allegro.h>
#include "gui/gui.h"

#define MEMORY_SIZE 1024

static void scissor(float x, float y, float w, float h) {
}

static gui_size
font_get_width(gui_handle handle, const gui_char *text, gui_size len) {
    return len * 8;
}

static void
draw_rect(float x, float y, float w, float h, float r, struct gui_color c)
{
    rectfill(screen, x, y, x+w, y+h, makeacol(c.r, c.g, c.b, c.a));
}

static void
draw_text(float x, float y, float w, float h,
    struct gui_color c, struct gui_color bg, const gui_char *string, gui_size len)
{
    /*draw_rect(ctx, x,y,w,h,0, bg);
    nvgBeginPath(ctx);
    nvgFillColor(ctx, nvgRGBA(c.r, c.g, c.b, c.a));
    nvgTextAlign(ctx, NVG_ALIGN_MIDDLE);
    nvgText(ctx, x, y + h * 0.5f, string, &string[len]);
    nvgFill(ctx);*/
}

static void
draw_line(float x0, float y0, float x1, float y1, struct gui_color c)
{
    /*
    nvgBeginPath(ctx);
    nvgMoveTo(ctx, x0, y0);
    nvgLineTo(ctx, x1, y1);
    nvgFillColor(ctx, nvgRGBA(c.r, c.g, c.b, c.a));
    nvgFill(ctx);*/
}

static void
draw_circle(float x, float y, float r, struct gui_color c)
{
    /*
    nvgBeginPath(ctx);
    nvgCircle(ctx, x + r, y + r, r);
    nvgFillColor(ctx, nvgRGBA(c.r, c.g, c.b, c.a));
    nvgFill(ctx);*/
}

static void
draw_triangle(float x0, float y0, float x1, float y1,
    float x2, float y2, struct gui_color c)
{
    /*
    nvgBeginPath(ctx);
    nvgMoveTo(ctx, x0, y0);
    nvgLineTo(ctx, x1, y1);
    nvgLineTo(ctx, x2, y2);
    nvgLineTo(ctx, x0, y0);
    nvgFillColor(ctx, nvgRGBA(c.r, c.g, c.b, c.a));
    nvgFill(ctx);*/
}

static void
draw_image(gui_handle img, float x, float y, float w, float h, float r)
{
    /*
    NVGpaint imgpaint;
    imgpaint = nvgImagePattern(ctx, x, y, w, h, 0, img.id, 1.0f);
    nvgBeginPath(ctx);
    nvgRoundedRect(ctx, x, y, w, h, r);
    nvgFillPaint(ctx, imgpaint);
    nvgFill(ctx);*/
}

void ui() {
    struct gui_command_queue queue;
    void *memory = malloc(MEMORY_SIZE);
    gui_command_queue_init_fixed(&queue, memory, MEMORY_SIZE);

    struct gui_font font;
    struct gui_config config;
    // font.userdata.ptr = ...;
    font.height = 8;
    font.width = font_get_width;
    gui_config_default(&config, GUI_DEFAULT_ALL, &font);

    struct gui_input input = {0};

    struct gui_panel panel;
    gui_panel_init(&panel, 50, 50, 220, 180,
                   GUI_PANEL_BORDER|GUI_PANEL_MOVEABLE|GUI_PANEL_SCALEABLE,
                   &queue, &config, &input);

    enum {EASY, HARD};
    gui_size option = EASY;
    gui_size item = 0;

    while(1) {
        struct gui_panel_layout layout;
        gui_panel_begin(&layout, &panel);
        {
            const char *items[] = {"Fist", "Pistol", "Railgun", "BFG"};
            gui_panel_header(&layout, "Demo", GUI_CLOSEABLE, 0, GUI_HEADER_LEFT);
            gui_panel_row_static(&layout, 30, 80, 1);
            if (gui_panel_button_text(&layout, "button", GUI_BUTTON_DEFAULT)) {
                /* event handling */
            }
            gui_panel_row_dynamic(&layout, 30, 2);
            if (gui_panel_option(&layout, "easy", option == EASY)) option = EASY;
            if (gui_panel_option(&layout, "hard", option == HARD)) option = HARD;
            gui_panel_label(&layout, "Weapon:", GUI_TEXT_LEFT);
            item = gui_panel_selector(&layout, items, 4, item);
        }
        gui_panel_end(&layout, &panel);

        const struct gui_command *cmd;
        gui_foreach_command(cmd, &queue) {
            switch (cmd->type) {
                case GUI_COMMAND_NOP: break;
                case GUI_COMMAND_SCISSOR: {
                    const struct gui_command_scissor *s = gui_command(scissor, cmd);
                    scissor(s->x, s->y, s->w, s->h);
                } break;
                case GUI_COMMAND_LINE: {
                    const struct gui_command_line *l = gui_command(line, cmd);
                    draw_line(l->begin.x, l->begin.y, l->end.x, l->end.y, l->color);
                } break;
                case GUI_COMMAND_RECT: {
                    const struct gui_command_rect *r = gui_command(rect, cmd);
                    draw_rect(r->x, r->y, r->w, r->h, r->rounding, r->color);
                } break;
                case GUI_COMMAND_CIRCLE: {
                    const struct gui_command_circle *c = gui_command(circle, cmd);
                    draw_circle(c->x, c->y, (float)c->w / 2.0f, c->color);
                } break;
                case GUI_COMMAND_TRIANGLE: {
                    const struct gui_command_triangle *t = gui_command(triangle, cmd);
                    draw_triangle(t->a.x, t->a.y, t->b.x, t->b.y, t->c.x,
                                  t->c.y, t->color);
                } break;
                case GUI_COMMAND_TEXT: {
                    const struct gui_command_text *t = gui_command(text, cmd);
                    draw_text(t->x, t->y, t->w, t->h, t->foreground, t->background, t->string, t->length);
                } break;
                case GUI_COMMAND_IMAGE: {
                    const struct gui_command_image *i = gui_command(image, cmd);
                    draw_image(i->img.handle, i->x, i->y, i->w, i->h, 1);
                } break;
                case GUI_COMMAND_MAX:
                default: break;
            }
        }       gui_command_queue_clear(&queue);
    }
    free(memory);
}

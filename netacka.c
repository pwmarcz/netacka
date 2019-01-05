#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <allegro.h>
#include <libnet.h>

#include "netacka.h"
#include "bots.inc"
#include "ui.h"

int gray_bg = 0;
int windowed = 0;

int game_mode = gNORMAL;
int torus = 0;


char server_name[16];
char game_path[256];

char server_passwd[7] = "";

#define MAX_FPS 255

int FPS = 20;

void crash(const char *msg);

#define TO_NEXT_HOLE (rand()%20+50)
#define HOLE_SIZE (rand()%2+rand()%2)

volatile int ticks = 0;
void _tick()
{
    ticks++;
}

volatile int escape = 0;
void _escape()
{
    escape = 1;
}

PALETTE pal = {
    {0, 0, 0},                  //  0 black
    {20, 20, 20},               //  1 dark gray
    {50, 50, 50},               //  2 very light gray
    {42, 42, 42},               //  3 light gray
    {63, 0, 0},                 //  4 red
    {0, 0, 0},                  //  5
    {0, 0, 0},                  //  6
    {28, 28, 28},               //  7 gray
    {0, 0, 0},                  //  8
    {0, 32, 63},                //  9 blue
    {0, 63, 0},                 // 10 green
    {0, 63, 63},                // 11 cyan
    {63, 32, 0},                // 12 orange
    {63, 0, 63},                // 13 pink
    {63, 63, 0},                // 14 yellow
    {63, 63, 63},               // 15 white
    {63, 63, 63},               // 15 white_wall
};

const int player_colors[MAX_PLAYERS][2] = {
    {cWHITE, cWHITE},
    {cRED, cRED},
    {cYELLOW, cYELLOW},
    {cGREEN, cGREEN},
    {cORANGE, cORANGE},
    {cBLUE, cBLUE},
    {cPINK, cPINK},
    {cCYAN, cCYAN},
    {cGRAY, cGRAY},
    {cWHITE, cGREEN},
    {cWHITE, cBLUE},
    {cWHITE, cRED},
    {cWHITE, cGRAY},
    {cRED, cYELLOW},
    {cYELLOW, cBLUE},
    {cRED, cBLUE},
    {cRED, cGRAY},
    {cGREEN, cGRAY},
    {cBLUE, cGRAY},
    {cYELLOW, cGRAY},
    {cPINK, cWHITE},
    {cPINK, cGRAY},
    {cRED, cGREEN}
};

inline int get_player_color(int i, int j) {
    return palette_color[player_colors[i][j]];
}

#define MOUSE_KEYS 5
const struct {
    int left, right;
    char str[14];
} client_keys[CLIENT_PLAYERS] = {
    {
    KEY_1, KEY_Q, "1      Q"}, {
    KEY_LCONTROL, KEY_ALT, "L.Ctrl L.Alt"}, {
    KEY_C, KEY_V, "C      V"}, {
    KEY_M, KEY_COMMA, "M      ,"}, {
    KEY_LEFT, KEY_DOWN, "Left   Down"}, {
    0, 0, "MOUSE"}
};

inline int check_keys(int k)
{
    int a = 0;
    if (k == MOUSE_KEYS) {
        int mb = mouse_b;
        if (mb & 2)
            a--;
        if (mb & 1)
            a++;
    } else {
        if (key[client_keys[k].right])
            a--;
        if (key[client_keys[k].left])
            a++;
    }
    return a;
}

struct player {
   char name[11];
   int x,y,old_x,old_y;
   int hole,old_hole,to_change;
   int a,last_da,da,da_change_time;
   int playing,alive;
   int client,client_num;
   int score;
} players[MAX_PLAYERS];
int n_players;
int n_alive;
int score_limit = 0;
int one_game = 0;
int save_log = 0;
int wait_for_key = 0;
int instant_start = 0;

struct client_player {
    char name[11];
    int num;
    int da;
    int keys;
    int bot;
    void *bot_data;
} client_players[CLIENT_PLAYERS];
int n_client_players;

int screen_w, screen_h;

NET_CHANNEL *chan;

int hide_bot_numbers = 0;

void send_name(int whose, NET_CHANNEL * chan);
void send_names_to_client(int who);

struct {
    //NET_CHANNEL *chan;
    int playing;
    char addr[50];
    int ready, gotit;
} clients[MAX_CLIENTS];
int n_clients = 0;

inline void set_to_client(int client)
{
    net_assigntarget(chan, clients[client].addr);
}

const char *welcome[] = {
    " NETACKA ver. " VER_STRING " with BOTS [a.k.a. Botacka]",
    "",
    "by    humpolec (humpolec@gmail.com)",
    "                   + bots by",
    "      jamiwron (jamiwron@gmail.com)",
    "    & derezin  (md262929@students.mimuw.edu.pl)",
    "",
    "Edit 'netacka.ini' to change configuration.",
    "",
    "NETACKA is using libnet (libnet.sf.net)",
    "             and Allegro (alleg.sf.net)",
    "",
    "The game is based on ZATACKA a.k.a. \"Achtung, die Kurve!\"",
    " (author unknown)",
    0
};

/* Returns - 0 = escape'd, 1 = start */
int get_client_players()
{
    int i;
    int playing[CLIENT_PLAYERS];
    char names[CLIENT_PLAYERS][11];

    int confirmed = 0;
    int done = 0;
    int success = 0;

    BITMAP *buf = create_bitmap(screen_w, screen_h);
    //show_mouse(buf);
    show_os_cursor(1);

    escape = 0;
    for (i = 0; i < CLIENT_PLAYERS; i++) {
        playing[i] = 0;
        names[i][0] = '\0';
    }
    while (!(done || key[KEY_ESC] || escape)) {
        if (confirmed) {
            ui_handle_input();
        }

        nk_begin(&ui, "welcome", nk_rect(60, 220, 500, 220), 0);
        {
            nk_layout_row_dynamic(&ui, 10, 1);
            for (i = 0; welcome[i]; i++) {
                nk_label_colored(&ui, welcome[i], NK_TEXT_LEFT, UI_LIGHT);
            }
        }
        nk_end(&ui);

        // Players
        nk_begin(&ui, "players", nk_rect(5, 10, 320, 200), 0);
        {
            for (i = 0; i < CLIENT_PLAYERS; i++) {
                nk_layout_row_begin(&ui, NK_LAYOUT_STATIC, 25, 3);
                {
                    nk_layout_row_push(&ui, 70);
                    if (confirmed) {
                        nk_checkbox_label(&ui, playing[i] ? " READY" : "", &playing[i]);
                    } else {
                        nk_label(&ui, playing[i] ? " READY" : "", NK_TEXT_LEFT);
                    }

                    nk_layout_row_push(&ui, 120);
                    nk_label(&ui, client_keys[i].str, NK_TEXT_LEFT);

                    if (playing[i] && confirmed) {
                        nk_layout_row_push(&ui, 100);
                        nk_edit_string_zero_terminated(&ui, NK_EDIT_SIMPLE, names[i], 10, nk_filter_default);
                    }
                }
                nk_layout_row_end(&ui);
            }
        }
        nk_end(&ui);

        if (confirmed) {
            nk_begin(&ui, "help", nk_rect(340, 10, 300, 200), 0);
            {
                // TODO colors
                nk_layout_row_dynamic(&ui, 10, 1);
                nk_label_colored(&ui, "Type bot number before player name", NK_TEXT_LEFT, UI_LIGHT);
                nk_label_colored(&ui, "to start a bot", NK_TEXT_LEFT, UI_LIGHT);
                nk_layout_row_dynamic(&ui, 10, 1);

                for (i = 0; i < N_BOTS; i++) {
                    nk_layout_row_begin(&ui, NK_LAYOUT_STATIC, 10, 2);

                    nk_layout_row_push(&ui, 120);
                    nk_labelf_colored(&ui, NK_TEXT_LEFT, UI_VERY_LIGHT, "%d %s", i, bots[i].name);

                    nk_layout_row_push(&ui, 120);
                    nk_label_colored(&ui, bots[i].descr, NK_TEXT_LEFT, UI_MEDIUM);
                    nk_layout_row_end(&ui);
                }
                nk_layout_row_dynamic(&ui, 20, 1);
                nk_layout_row_dynamic(&ui, 30, 1);
                if (nk_button_label_styled(&ui, &ui_style_button_important, "Proceed")) {
                    success = 1;
                    done = 1;
                }
            }
            nk_end(&ui);
        } else {
            for (i = 0; i < CLIENT_PLAYERS; i++) {
                if (check_keys(i) == 1 && !playing[i]) {
                    playing[i] = 1;
                }
                if (check_keys(i) == -1) {
                    playing[i] = 0;
                }
            }

            if (key[KEY_SPACE]) {
                while (key[KEY_SPACE])
                    rest(1);
                clear_keybuf();
                confirmed = 1;
            }
        }

        rest(1);
        clear_bitmap(buf);
        ui_draw(buf);
        blit(buf, screen, 0, 0, 0, 0, screen_w, screen_h);
    }
    show_mouse(NULL);
    destroy_bitmap(buf);

    if (success) {
        int j = 0;
        for (i = 0; i < CLIENT_PLAYERS; i++) {
            if (playing[i]) {
                client_players[j].keys = i;
                strcpy(client_players[j].name, names[i]);
                char c = names[i][0];
                if (!isdigit(c) || c - '0' >= N_BOTS)
                    client_players[j].bot = -1;
                else
                    client_players[j].bot = c - '0';
                j++;
            }
        }
        n_client_players = j;
    }

    return success;
}

void draw_score_list(BITMAP * score_list)
{
    int i;
    int y;
    int best_score = 0;

    for (i = 0; i < MAX_PLAYERS; i++)
        if (players[i].playing && best_score < players[i].score)
            best_score = players[i].score;
    clear_bitmap(score_list);
    clear_to_color(score_list, gray_bg ? palette_color[cGRAY] : palette_color[cBLACK]);
//   line(score_list,0,0,0,screen_h-1,palette_color[cWHITE]);
    y = 0;
    for (i = 0; i < MAX_PLAYERS; i++, y += 25) {
        if (!players[i].playing)
            continue;
        //line(score_list,2,9+y,109,9+y,palette_color[cGRAY]);
        if (!gray_bg) {
            if (!players[i].alive)
                rectfill(score_list, 2, 9 + y, 109, 9 + y + 24, palette_color[cGRAY]);
        } else {
            if (players[i].alive)
                rectfill(score_list, 2, 9 + y, 109, 9 + y + 24, palette_color[cBLACK]);
        }
        if (players[i].name[0]) {
            char *name = (hide_bot_numbers
                          && isdigit(players[i].name[0])) ? &players[i].
                name[1] : players[i].name;
            textout_ex(score_list, font, name, 2, 11 + y,
                       (players[i].score ==
                        best_score) ? palette_color[cWHITE] : palette_color[cVLIGHTGRAY], -1);
        }
        rectfill(score_list, 82, 10 + y, 95, 19 + y, get_player_color(i, 0));
        rectfill(score_list, 96, 10 + y, 108, 19 + y, get_player_color(i, 1));
        textprintf_ex(score_list, font, 83, 11 + y,
                      palette_color[cBLACK], -1, "%3d", players[i].score);
        if (players[i].client == -1) {
            struct client_player *c =
                &client_players[players[i].client_num];
            textout_ex(score_list, font,
                       (c->bot ==
                        -1) ? client_keys[c->keys].str : bots[c->bot].name,
                       2, 22 + y, palette_color[cGREEN], -1);
        }
        if (players[i].score == best_score)
            rect(score_list, 1, 9 + y, 108, 9 + y + 24, palette_color[cWHITE]);
    }
}

void draw_konec(BITMAP * bmp)
{
    int i, j;
    int place = 0, last_score = -1;
    int order[MAX_PLAYERS];
    int x = (screen_w - 110) / 2;
    FILE *fp = 0;

    if (save_log) {
        long t = time(0);
        char fname[256];

        sprintf(fname, "%s%ld.txt", game_path, t);
        fp = fopen(fname, "w");
    }
    clear_bitmap(bmp);
    for (i = 0; i < MAX_PLAYERS; i++)
        order[i] = i;
    for (i = 0; i < n_players; i++) {
        char *name;
        int max_j = -1;
        int tmp;

        for (j = i; j < MAX_PLAYERS; j++)
            if (players[order[j]].playing) {
                if (max_j == -1)
                    max_j = j;
                else if (players[order[j]].score >
                         players[order[max_j]].score)
                    max_j = j;
            }
        tmp = order[max_j];
        order[max_j] = order[i];
        order[i] = tmp;
        name = (hide_bot_numbers && isdigit(players[tmp].name[0])) ?
            &players[tmp].name[1] : players[tmp].name;
        if (players[tmp].score != last_score) {
            textprintf_ex(bmp, font, x - 30, 10 + i * 25, palette_color[cGRAY], -1,
                          "%2d.", ++place);
            if (fp)
                fprintf(fp, "%2d. ", place);
        } else if (fp)
            fprintf(fp, "    ");
        textprintf_ex(bmp, font, x + 25, 10 + i * 25, palette_color[cWHITE],
                      -1, "%3d", players[tmp].score);
        if (save_log) {
            if (fp)
                fprintf(fp, "#%2d %10s (%3d)\n", tmp, name,
                        players[tmp].score);
        }
        rectfill(bmp, x, 10 + i * 25, x + 9, 19 + i * 25,
                 get_player_color(tmp, 0));
        rectfill(bmp, x + 10, 10 + i * 25, x + 19, 19 + i * 25,
                 get_player_color(tmp, 1));
        textprintf_ex(bmp, font, x + 55, 10 + i * 25, palette_color[cVLIGHTGRAY], -1,
                      "%10s", name);
        last_score = players[tmp].score;
    }
    if (fp)
        fclose(fp);
}

void send_keys()
{
    char data[n_client_players * 2 + 1];
    int i;

    data[0] = clMYINPUT;
    for (i = 0; i < n_client_players; i++) {
        data[i * 2 + 1] = client_players[i].num;
        data[i * 2 + 2] = client_players[i].da + 1;
    }
    net_send(chan, data, n_client_players * 2 + 1);
}

int add_client(int n_pl, char *addr, char *passwd)
{
    int i, j, k;
    unsigned char data[14];

    if (n_clients == MAX_CLIENTS || n_players + n_pl > MAX_PLAYERS)
        return 0;
    if (n_pl)
        if (strcmp(server_passwd, passwd))
            return 0;

    for (i = 0; clients[i].playing; i++);
    //clients[i].chan=net_openchannel(net_driver,0);
    clients[i].playing = 1;
    strcpy(clients[i].addr, addr);
    //net_assigntarget(clients[i].chan,addr);

    data[0] = seHEREARESETTNGS;
    data[1] = FPS;
    data[2] = screen_w >> 8;
    data[3] = screen_w & 0xFF;
    data[4] = screen_h >> 8;
    data[5] = screen_h & 0xFF;
    j = 0;
    for (k = 0; k < n_pl; k++) {
        for (; players[j].playing; j++);
        players[j].playing = 1;
        players[j].x = players[j].y = -256;
        players[j].alive = 0;
        players[j].client = i;
        data[6 + k] = j;
    }
    data[6 + n_pl] = torus + (game_mode << 1);
    set_to_client(i);
    net_send(chan, data, 7 + k);
    clients[i].ready = 0;

    n_clients++;

    send_names_to_client(i);

    return n_pl;
}

void remove_client(int n)
{
    int i;

    clients[n].playing = 0;
    n_clients--;
    for (i = 0; i < MAX_PLAYERS; i++)
        if (players[i].client == n && players[i].playing) {
            players[i].playing = 0;
            n_players--;
            if (players[i].alive) {
                players[i].alive = 0;
                n_alive--;
            }
            players[i].score = 0;
        }
}

#define DISTANCE 75

void server_start_new_round()
{
    int i;
    for (i = 0; i < MAX_PLAYERS; i++)
        if (players[i].playing) {
            players[i].alive = 1;
            players[i].x =
                (rand() % (screen_w - 110 - 2 * DISTANCE) +
                 DISTANCE) * 256;
            players[i].y =
                (rand() % (screen_h - 2 * DISTANCE) + DISTANCE) * 256;
            if (game_mode != gTRON)
                players[i].a = rand() % 256;
            else
                players[i].a = rand() % 4;
            players[i].da = players[i].last_da = 0;
            players[i].da_change_time = ticks;
            players[i].hole = players[i].old_hole = 0;
            players[i].to_change = TO_NEXT_HOLE;
        }
    n_alive = n_players;
}

void send_players(int who)
{
    int i, pos = 1;
    unsigned char data[1 + MAX_PLAYERS * 8];

    data[0] = seHEREAREPLAYERS;
    for (i = 0; i < MAX_PLAYERS; i++) {
        if (players[i].playing) {
            data[pos] = i;
            data[pos + 1] = players[i].x >> 16;
            data[pos + 2] = players[i].x >> 8;
            data[pos + 3] = players[i].y >> 16;
            data[pos + 4] = players[i].y >> 8;
            data[pos + 5] = players[i].a;
            data[pos + 6] = players[i].da + 1;
            data[pos + 7] = players[i].score >> 8;
            data[pos + 8] = players[i].score;
            if (players[i].alive)
                data[pos + 6] |= 128;
            if (players[i].hole)
                data[pos + 6] |= 64;
            pos += 9;
        }
    }
    if (who == -1) {
        for (i = 0; i < MAX_CLIENTS; i++)
            if (clients[i].playing) {
                set_to_client(i);
                net_send(chan, data, pos);
            }
    } else {
        set_to_client(who);
        net_send(chan, data, pos);
    }
}

void send_name(int whose, NET_CHANNEL * chan)
{
    unsigned char data[13];

    data[0] = csANAME;
    data[1] = whose;
    strcpy((char *) &data[2], players[whose].name);
    net_send(chan, data, 3 + strlen(players[whose].name));
}

void send_names_to_client(int who)
{
    int i;

    if (who == -1) {
        for (i = 0; i < MAX_CLIENTS; i++)
            if (clients[i].playing)
                send_names_to_client(i);
    } else {
        set_to_client(who);
        for (i = 0; i < MAX_PLAYERS; i++)
            if (players[i].client != who)
                send_name(i, chan);
    }
}

void send_names_to_server()
{
    int i;

    for (i = 0; i < n_client_players; i++)
        send_name(client_players[i].num, chan);
}

inline int __test(BITMAP * arena, int old_x, int old_y, int x, int y)
{
    int dox = x - old_x, doy = y - old_y;
    if ((dox == 0 || dox == 1) && (doy == 0 || doy == 1))
        return 0;
    if (torus)
        return getpixel(arena, (x + screen_w - 112) % (screen_w - 111) + 1,
                        (y + screen_h - 3) % (screen_h - 2) + 1) != palette_color[cBLACK];
    return getpixel(arena, x, y) != palette_color[cBLACK];
}

int _test(BITMAP * arena, int old_x, int old_y, int x, int y,
          int hole, int old_hole)
{
    int half_x = (x + old_x) / 512,
        half_y = (y + old_y) / 512, p = 0, q = 0, r = 0, s = 0;
    x /= 256;
    y /= 256;
    old_x /= 256;
    old_y /= 256;

    if (!torus)
        if (x < 1 || x > screen_w - 111 || y < 1 || y > screen_h - 2)
            return palette_color[cWHITE_WALL];
    if (hole)
        return 0;
    if (!old_hole) {
        if ((p = __test(arena, old_x, old_y, half_x, half_y)) ||
            (q = __test(arena, old_x, old_y, half_x + 1, half_y)) ||
            (r = __test(arena, old_x, old_y, half_x, half_y + 1)) ||
            (s = __test(arena, old_x, old_y, half_x + 1, half_y + 1)))
            return p ? p : (q ? q : (r ? r : s));
    }
    if ((p = __test(arena, old_x, old_y, x, y)) ||
        (q = __test(arena, old_x, old_y, x + 1, y)) ||
        (r = __test(arena, old_x, old_y, x, y + 1)) ||
        (s = __test(arena, old_x, old_y, x + 1, y + 1)))
        return p ? p : (q ? q : (r ? r : s));
    else
        return 0;

}

void _update(int x, int y, int a, int *x1, int *y1)
{
    *x1 = x + (768) * fixtof(fixcos(itofix(a)));
    *y1 = y - (768) * fixtof(fixsin(itofix(a)));
}

void _update_tron(int x, int y, int a, int *x1, int *y1)
{
    int dx = 0, dy = 0;
    switch (a) {
    case 0:
        dx = 0;
        dy = -1;
        break;
    case 1:
        dx = 1;
        dy = 0;
        break;
    case 2:
        dx = 0;
        dy = 1;
        break;
    case 3:
        dx = -1;
        dy = 0;
        break;
    }
    *x1 = x + 768 * dx;
    *y1 = y - 768 * dy;
}

void _put(BITMAP * arena, int x, int y, int c)
{
    if (torus) {

        putpixel(arena, (x + screen_w - 112) % (screen_w - 111) + 1,
                 (y + screen_h - 3) % (screen_h - 2) + 1, c);
        putpixel(arena, (x + screen_w - 111) % (screen_w - 111) + 1,
                 (y + screen_h - 3) % (screen_h - 2) + 1, c);
        putpixel(arena, (x + screen_w - 112) % (screen_w - 111) + 1,
                 (y + screen_h - 2) % (screen_h - 2) + 1, c);
        putpixel(arena, (x + screen_w - 111) % (screen_w - 111) + 1,
                 (y + screen_h - 2) % (screen_h - 2) + 1, c);
    } else
        rectfill(arena, x, y, x + 1, y + 1, c);
}

void draw_players(BITMAP * arena, int i_know, int t)
{
    int i;
    for (i = 0; i < MAX_PLAYERS; i++) {
        if (!players[i].playing)
            continue;
        if (i_know && !players[i].hole && !players[i].old_hole)
            _put(arena, (players[i].x + players[i].old_x) / 512,
                 (players[i].y + players[i].old_y) / 512,
                 get_player_color(i, (t / 4) % 2));
        if (!players[i].hole)
            _put(arena, players[i].x / 256, players[i].y / 256,
                 get_player_color(i, (t / 4) % 2));
    }
}

void revise_pos()
{
    int i = 0;
    for (; i < MAX_PLAYERS; i++) {
        if (players[i].alive && players[i].playing) {
            if (players[i].x < 5 << 8)
                players[i].x += (screen_w - 111) << 8;
            if (players[i].x > (screen_w - 105) << 8)
                players[i].x -= (screen_w - 111) << 8;
            if (players[i].y < 5 << 8)
                players[i].y += (screen_h - 2) << 8;
            if (players[i].y > (screen_h + 6) << 8)
                players[i].y -= (screen_h - 2) << 8;
        }
        int dx = (players[i].x - players[i].old_x) >> 8,
            dy = (players[i].y - players[i].old_y) >> 8;
        if (dx > screen_w / 2)
            players[i].old_x += (screen_w - 111) << 8;
        else if (dx < -screen_w / 2)
            players[i].old_x -= (screen_w - 111) << 8;
        if (dy > screen_h / 2)
            players[i].old_y += (screen_h - 2) << 8;
        else if (dy < -screen_h / 2)
            players[i].old_y -= (screen_h - 2) << 8;

    }
}

void screenshot(BITMAP * buf)
{
    char fname[256];
    long t = time(0);
    sprintf(fname, "%s%ld.pcx", game_path, t);
    save_bitmap(fname, buf, pal);
}

void close_bots()
{
    int i;
    for (i = 0; i < n_client_players; i++) {
        if (client_players[i].bot != -1) {
            if (client_players[i].bot_data)
                bots[client_players[i].bot].close(client_players[i].
                                                  bot_data);
            client_players[i].bot_data = NULL;
        }
    }
}

void start_bots()
{
    int i;
    for (i = 0; i < n_client_players; i++) {
        if (client_players[i].bot != -1)
            if (!(client_players[i].bot_data =
                  bots[client_players[i].bot].start(client_players[i].num,
                                                    screen_w, screen_h)))
                client_players[i].bot = -1;
    }
}

#define STARTING_TIME (1.5+0.2*n_players)
int play_round(int is_server)
{
    BITMAP *buf = create_bitmap(screen_w, screen_h);
    BITMAP *arena = create_sub_bitmap(buf, 0, 0, screen_w - 110, screen_h);
    BITMAP *score_list =
        create_sub_bitmap(buf, screen_w - 109, 0, 110, screen_h);

    int t, new_round = 0, new_round_announce = 0;
    int i;
    int playing = 1, announcing = 0, done = 0;
    int i_know = 0, first = 0;
    int konec = 0;
    int change_time = FPS / 3;

    if (!is_server)
        save_log = 0;
    clear_bitmap(buf);
    rect(buf, 0, 0, screen_w - 110, screen_h - 1, palette_color[cWHITE_WALL]);
    ticks = t = 0;
    escape = 0;
    //for(i=0;i<N_BOTS;i++) bots[i].start();
    start_bots();
    while (!key[KEY_ESC] && !escape && !done) {
        rest(1);
        if (is_server && wait_for_key)
            if (key[KEY_SPACE])
                wait_for_key = 0;
        if (((n_alive < 2 && n_players > 1)
             || (n_players == 1 && n_alive == 0))
            && playing && !wait_for_key)        //new round
        {

            playing = 0;
            ticks = t = 0;
            draw_score_list(score_list);
            if (is_server) {
                send_players(-1);
                new_round_announce = FPS * 1.5;
                if (save_log)
                    screenshot(buf);
            }
        }

        if (!playing && !announcing && ticks > new_round_announce &&
            is_server && !wait_for_key) {
            int i;
            if (!konec) {
                if (score_limit) {
                    for (i = 0; i < MAX_PLAYERS; i++)
                        if (players[i].playing
                            && players[i].score >= score_limit)
                            konec = 1;
                }
            } else {
                konec = 0;
                for (i = 0; i < MAX_PLAYERS; i++)
                    players[i].score = 0;
                if (one_game)
                    done = 1;
            }
            if (konec) {
                unsigned char data[n_players * 3 + 1];
                int j = 1;

                data[0] = seKONEC_HRY;
                for (i = 0; i < MAX_PLAYERS; i++)
                    if (players[i].playing) {
                        data[j] = i;
                        data[j + 1] = players[i].score >> 8;
                        data[j + 2] = players[i].score;
                        j += 3;
                    }
                for (i = 0; i < MAX_CLIENTS; i++)
                    if (clients[i].playing) {
                        set_to_client(i);
                        net_send(chan, data, j);
                    }
                ticks = t = 0;
                new_round_announce = FPS * (0.5 + STARTING_TIME);
                draw_konec(screen);
            } else {
                server_start_new_round();
                i_know = 0;
                clear_bitmap(buf);
                close_bots();
                start_bots();
                rect(buf, 0, 0, screen_w - 110, screen_h - 1, palette_color[cWHITE_WALL]);
                draw_players(arena, i_know, t);
                ticks = t = 0;
                new_round =
                    instant_start ? FPS * 0.3 : FPS * STARTING_TIME;
                announcing = 1;
                //broadcast NEWROUND
                for (i = 0; i < MAX_CLIENTS; i++)
                    if (clients[i].playing) {
                        clients[i].ready = 0;
                        set_to_client(i);
                        send_byte(chan, seNEWROUND);
                    }
            }
        }

        if (announcing && ticks > new_round && is_server) {
            for (i = 0; i < MAX_CLIENTS; i++)
                if (clients[i].playing && !clients[i].ready) {
                    remove_client(i);
                }
            announcing = 0;
            playing = 1;
            i_know = 0;
        }
        if (!konec) {
            draw_score_list(score_list);
            blit(buf, screen, 0, 0, 0, 0, screen_w, screen_h);
        }

        if (t <= ticks && playing) {
            if (!is_server) {

                for (i = 0; i < n_client_players; i++) {
                    int k = client_players[i].keys, j =
                        client_players[i].num;
                    if (players[j].alive) {
                        if (client_players[i].bot == -1)
                            client_players[i].da = check_keys(k);
                        else
                            client_players[i].da =
                                bots[client_players[i].bot].check(arena, j,
                                                                  client_players
                                                                  [i].
                                                                  bot_data);
                    }
                }
                send_keys();
            } else
                for (i = 0; i < n_client_players; i++) {
                    int j = client_players[i].num;
                    if (players[j].alive && client_players[i].bot != -1)
                        players[j].da =
                            bots[client_players[i].bot].check(arena, j,
                                                              client_players
                                                              [i].
                                                              bot_data);
                }
        }

        while (t <= ticks) {
            t++;
            draw_players(arena, i_know, t);
            if (is_server && playing) {
                send_players(-1);
                if (torus)
                    revise_pos();
                for (i = 0; i < n_client_players; i++) {
                    int k = client_players[i].keys, j =
                        client_players[i].num;
                    if (players[j].alive) {
                        if (client_players[i].bot == -1)
                            players[j].da = check_keys(k);
                    }
                }
                for (i = 0; i < MAX_PLAYERS; i++) {

                    int x, y;
                    int collide = 0;
                    if (!(players[i].alive && players[i].playing))
                        continue;


                    switch (game_mode) {
                    case gNORMAL:
                        _update_angle(&players[i].a, players[i].da);
                        break;
                    case gONEFINGER:
                        _update_angle(&players[i].a,
                                      players[i].da ? 1 : -1);
                        break;
                    case gTRON:
                        if (players[i].da != players[i].last_da) {
                            players[i].last_da = players[i].da;
                            players[i].da_change_time = ticks - 1;
                        }

                        if (players[i].da) {

                            if (ticks >= players[i].da_change_time) {
                                players[i].a =
                                    (players[i].a + 4 + players[i].da) % 4;
                                players[i].da_change_time =
                                    ticks + change_time;
                            }
                        }
                        break;
                    }

                    players[i].old_hole = players[i].hole;

                    if (game_mode != gTRON)
                        if (players[i].to_change-- <= 0) {
                            if (players[i].hole) {
                                players[i].hole = 0;
                                players[i].to_change = TO_NEXT_HOLE;
                            } else {
                                players[i].hole = 1;
                                players[i].to_change = HOLE_SIZE;
                            }
                        }

                    if (game_mode != gTRON)
                        _update(players[i].x, players[i].y, players[i].a,
                                &x, &y);
                    else
                        _update_tron(players[i].x, players[i].y,
                                     players[i].a, &x, &y);

                    /*             if(torus)
                       {
                       players[i].torus_jump=0;
                       if(x<4<<8)
                       { x+=(screen_w-110)<<8; players[i].torus_jump=1;}
                       if(x>(screen_w-112)<<8)
                       { x-=(screen_w-110)<<8; players[i].torus_jump=1;}
                       if(y<4<<8)
                       { y+=(screen_h)<<8; players[i].torus_jump=1; }
                       if(y>(screen_h-3)<<8)
                       { y-=(screen_h)<<8; players[i].torus_jump=1; }
                       if(players[i].torus_jump)
                       {
                       players[i].x=x; players[i].y=y;
                       }
                       } */


                    if (_test(arena, players[i].x, players[i].y,
                              x, y, players[i].hole, players[i].old_hole))
                        collide = 1;

                    if (collide) {
                        int j;

                        players[i].alive = 0;
                        n_alive--;
                        for (j = 0; j < MAX_PLAYERS; j++)
                            if (players[j].alive && players[j].playing)
                                players[j].score++;
                    }

                    /*players[i].old_x=(players[i].x+x)/2;
                       players[i].old_y=(players[i].y+y)/2; */
                    players[i].old_x = players[i].x;
                    players[i].old_y = players[i].y;

                    players[i].x = x;
                    players[i].y = y;
                }

                i_know = 1;
            }
        }

        while (net_query(chan)) {
            if (is_server) {
                unsigned char data[80];
                char from[50];
                int n;
                int j;

                n = net_receive(chan, data, 80, from);
                for (i = 0; i < MAX_CLIENTS; i++) {
                    if (!strcmp(from, clients[i].addr))
                        break;
                }
                switch (*data) {
                case clIWANNAPLAY:
                    if (i == MAX_CLIENTS)
                        n_players +=
                            add_client(data[1], from, (char *) &data[2]);
                    break;
                case clMYINPUT:
                    for (j = 1; j < n; j += 2) {
                        int k = data[j], da = 0;
                        if (players[k].client == i) {
                            switch (data[j + 1]) {
                            case 0:
                                da = -1;
                                break;
                            case 1:
                                da = 0;
                                break;
                            case 2:
                                da = 1;
                                break;
                            }
                            players[k].da = da;
                        }
                    }
                    break;
                case clGOTIT:
                    clients[i].gotit = 1;
                    break;
                case clREADY:
                    clients[i].ready = 1;
                    send_players(i);
                    break;
                case clGOODBYE:
                    remove_client(i);
                    break;
                case clKNOCKKNOCK:
                    {
                        char data2[20];
                        data2[0] = seMYINFO;
                        data2[1] = n_players;
                        data2[2] = server_passwd[0] ? '*' : ' ';
                        strcpy(&data2[3], server_name);
                        net_assigntarget(chan, from);
                        net_send(chan, data2, 20);
                        break;
                    }
                case PING:
                    net_assigntarget(chan, from);
                    send_byte(chan, PONG);
                    break;
                case csANAME:
                    if (players[data[1]].client == i) {
                        int k;

                        strcpy(players[data[1]].name, (char *) &data[2]);
                        for (k = 0; k < MAX_CLIENTS; k++)
                            if (k != i && clients[k].playing) {
                                set_to_client(k);
                                send_name(data[1], chan);
                            }
                    }
                default:
                    break;
                }
            } else {
                unsigned char data[300];
                int n;

                n = net_receive(chan, data, 300, 0);
                switch (data[0]) {
                case seHEREAREPLAYERS:
                    {
                        int pos = 1;
                        int i;
                        //if (torus)revise_pos();
                        for (i = 0; i < MAX_PLAYERS; i++)
                            players[i].playing = 0;
                        n_players = 0;
                        n_alive = 0;
                        while (pos < n) {
                            i = data[pos];
                            players[i].playing = 1;
                            n_players++;
                            players[i].old_hole = players[i].hole;
                            players[i].old_x = players[i].x;
                            players[i].old_y = players[i].y;
                            players[i].x =
                                (data[pos + 1] << 16) +
                                (data[pos + 2] << 8);
                            players[i].y =
                                (data[pos + 3] << 16) +
                                (data[pos + 4] << 8);
                            players[i].a = data[pos + 5];
                            players[i].da = (data[pos + 6] & 3) - 1;
                            if ((players[i].alive =
                                 (data[pos + 6] & 128) ? 1 : 0))
                                n_alive++;
                            players[i].hole = (data[pos + 6] & 64) ? 1 : 0;
                            players[i].score =
                                (data[pos + 7] << 8) + data[pos + 8];
                            pos += 9;
                        }
                        if (torus)
                            revise_pos();
                        playing = 1;
                        draw_players(arena, i_know, t);
                        if (first)
                            first = 0;
                        else
                            i_know = 1;
                        break;
                    }
                case seKONEC_HRY:
                    {
                        int i;
                        konec = 1;
                        for (i = 1; i < n; i += 3) {
                            int j = data[i];
                            players[j].score =
                                (data[i + 1] << 8) + data[i + 2];
                        }
                        draw_konec(screen);
                        break;
                    }
                case seNEWROUND:
                    konec = 0;
                    playing = 1;
                    i_know = 0;
                    first = 1;
                    clear_bitmap(buf);
                    close_bots();
                    start_bots();
                    rect(buf, 0, 0, screen_w - 110, screen_h - 1,
                         palette_color[cWHITE_WALL]);
                    ticks = t = 0;
                    for (i = 0; i < MAX_PLAYERS; i++)
                        players[i].x = players[i].y = -256;
                    send_byte(chan, clREADY);
                    send_names_to_server();
                    break;
                case seIKICKYOU:
                    done = 1;
                    break;
                case csANAME:
                    strcpy(players[data[1]].name, (char *) &data[2]);
                default:
                    break;
                }
            }
        }
    }

    if (is_server) {
        for (i = 0; i < MAX_CLIENTS; i++)
            if (clients[i].playing) {
                set_to_client(i);
                send_byte(chan, seIKICKYOU);
                remove_client(i);
            }
    }
    close_bots();

    destroy_bitmap(score_list);
    destroy_bitmap(arena);
    destroy_bitmap(buf);
    return 0;
}

void send_client_players(NET_CHANNEL * chan)
{
    unsigned char data[10];

    data[0] = clIWANNAPLAY;
    data[1] = n_client_players;
    strcpy((char *) &data[2], server_passwd);
    net_send(chan, data, 2 + strlen(server_passwd) + 1);
}


void set_the_damn_config()
{
    char str[256];
    get_executable_name(str, 240);
    replace_filename(game_path, str, "", 240);
    strcpy(str, game_path);
    strcat(str, "netacka.ini");
    set_config_file(str);
}

// 1 = client, -1 = server, 0 = Escape'd

int set_mode(int w, int h)
{
    int depth = desktop_color_depth();
    if (depth == 0)
        crash("couldn't determine color depth");
    if (depth != 24 && depth != 32)
        crash("expecting 24-bit or 32-bit color depth");

    set_color_depth(depth);

    if (set_gfx_mode(windowed ? GFX_AUTODETECT_WINDOWED : GFX_AUTODETECT,
                     w, h, 0, 0) != 0)
        if (set_gfx_mode
            (windowed ? GFX_AUTODETECT : GFX_AUTODETECT_WINDOWED, w, h, 0,
             0) != 0)
            return -1;

    set_palette(pal);

    if (set_display_switch_mode(SWITCH_BACKGROUND)) {
        set_display_switch_mode(SWITCH_BACKAMNESIA);
    }
    disable_hardware_cursor();
    show_mouse(NULL);
    clear_bitmap(screen);
    return 0;
}

int start();

int main()
{
    int is_server = 0;
    int success = 0;

    srand(time(0));

    allegro_init();

    set_the_damn_config();
    windowed = get_config_int(0, "windowed", 1);
    if (getenv("NETACKA_WINDOWED"))
        windowed = 1;

    install_keyboard();
    install_mouse();
    install_timer();
    install_int_ex(_tick, BPS_TO_TIMER(FPS));
    set_close_button_callback(_escape);

    screen_w = 640;
    screen_h = 480;
    if (set_mode(640, 480))
        crash("couldn't set video mode");
    gui_fg_color = palette_color[cWHITE];
    gui_bg_color = palette_color[cBLACK];
    gui_mg_color = palette_color[cGRAY];

    if (start_net())
        return 1;

    ui_init();
    // ui_demo();

    if (get_client_players()) {
        switch (start()) {
        case 1:
            is_server = 0;
            success = 1;
            break;
        case -1:
            is_server = 1;
            success = 1;
            break;
        default:
            break;
        }
    }
    if (success) {
        remove_int(_tick);
        install_int_ex(_tick, BPS_TO_TIMER(FPS));

        if (!(screen_w == 640 && screen_h == 480)) {
            if (set_mode(screen_w, screen_h))
                crash("couldn't set video mode");
        }
        if (!is_server)
            send_byte(chan, clREADY);
        play_round(is_server);
        if (!is_server)
            send_byte(chan, clGOODBYE);
    }
    ui_shutdown();
    set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
    net_closechannel(chan);
    return 0;
}

END_OF_MAIN()

void crash(const char *msg)
{
    set_gfx_mode(GFX_TEXT, 0, 0, 0, 0);
    allegro_message("Error: %s", msg);
    exit(1);
}

#define SERVER_LIST_SIZE 16

struct {
    char addr[50], name[17];
    int active;
    int n_players;
} servers[SERVER_LIST_SIZE];
int n_servers = 0;

char *_getter(int index, int *list_size)
{
    static char str[100];
    if (index == -1) {
        if (n_servers)
            *list_size = n_servers;
        else
            *list_size = 1;
        return 0;
    }
    if (n_servers == 0 && index == 0)
        return "  ...";
    if (!servers[index].active)
        sprintf(str, "...");
    else if (servers[index].n_players == -1)
        sprintf(str, "%20s - [not responding]", servers[index].addr);
    else
        sprintf(str, "%20s - %15s - %2d/%2d players", servers[index].addr,
                servers[index].name, servers[index].n_players,
                MAX_PLAYERS);
    return str;
}

#define N_RES 5
const int res_list[N_RES][2] = {
    {640, 480},
    {800, 600},
    {1024, 768},
    {1280, 1024},
    {1600, 1200}
};

int def_res_x, def_res_y;

char *_res_getter(int index, int *list_size)
{
    static char str[40];
    if (index == -1) {
        *list_size = N_RES + 1;
        return 0;
    }
    if (index == 0)
        sprintf(str, "%4d x %4d (from config file)", def_res_x, def_res_y);
    else
        sprintf(str, "%4d x %4d", res_list[index - 1][0],
                res_list[index - 1][1]);
    return str;
}

void get_res_label(void *data, int index, const char **label) {
    static char str[40];
    if (index == 0)
        sprintf(str, "%4d x %4d (from config file)", def_res_x, def_res_y);
    else
        sprintf(str, "%4d x %4d", res_list[index - 1][0],
                res_list[index - 1][1]);
    *label = str;
}

const char *game_mode_names[] = {
    "Normal game",
    "One-finger game",
    "TRON-style game",
};

const struct {
    int modenum;
    char *name;
} game_modes[] = { {
gNORMAL, "Normal game"}, {
gONEFINGER, "One-finger game"}, {
gTRON, "TRON-style game"}};

char *_mode_getter(int index, int *list_size)
{
    if (index == -1) {
        *list_size = 3;
        return 0;
    } else
        return game_modes[index].name;
}

char server_addr[31], str_score_limit[4], str_fps[3], port[6];
int res_num;

#define POS_SERVER_LIST     0
#define POS_TRY_SERVER      1
#define POS_RELOAD_LIST     2
#define POS_CONNECT         4
#define POS_START_SERVER    5
#define POS_RES_LIST        10
#define POS_MODE_LIST       15
#define POS_ONEGAME         16
#define POS_SAVELOG         17
#define POS_WAITFORKEY      22
#define POS_TORUS           23

#define POS_SCORELIMIT      7
#define POS_FPS             9

int to_disable[] = { POS_SCORELIMIT, POS_FPS, POS_WAITFORKEY,
    POS_SAVELOG, POS_ONEGAME, POS_RES_LIST,
    -1
};

DIALOG the_list[] = {
    /* (dialog proc)     (x)   (y)   (w)   (h) (fg)(bg) (key) (flags)     (d1) (d2)    (dp)                   (dp2) (dp3) */
    {d_list_proc, 10, 50, 620, 180, 0, 0, 0, 0, 0, 0, _getter, 0, NULL},
    {d_button_proc, 10, 260, 305, 30, 0, 0, 0, 0, 0, 0,
     "Try selected server", 0, NULL},
    {d_button_proc, 325, 10, 305, 30, 0, 0, 0, 0, 0, 0,
     "Reload server list", 0, NULL},

    {d_edit_proc, 325, 260, 200, 10, 0, 0, 0, 0, 30, 0, server_addr, 0,
     NULL},
    {d_button_proc, 325, 275, 200, 30, 0, 0, 0, 0, 0, 0, "Try this server",
     0, NULL},

    {d_button_proc, 325, 430, 305, 30, 0, 0, 0, 0, 0, 0, "Start Server", 0,
     NULL},
    {d_text_proc, 10, 370, 0, 0, 0, 0, 0, 0, 0, 0, "Score limit:", 0,
     NULL},
    {d_edit_proc, 200, 370, 100, 10, 0, 0, 0, 0, 3, 0, str_score_limit, 0,
     NULL},
    {d_text_proc, 10, 390, 0, 0, 0, 0, 0, 0, 0, 0, "FPS (game speed):", 0,
     NULL},
    {d_edit_proc, 200, 390, 100, 10, 0, 0, 0, 0, 3, 0, str_fps, 0, NULL},
    {d_list_proc, 10, 410, 260, 55, 0, 0, 0, 0, 0, 0, _res_getter, 0,
     NULL},
    {d_text_proc, 325, 380, 0, 0, 0, 0, 0, 0, 0, 0, "Server name:", 0,
     NULL},
    {d_edit_proc, 455, 380, 170, 10, 0, 0, 0, 0, 15, 0, server_name, 0,
     NULL},
    {d_text_proc, 325, 395, 0, 0, 0, 0, 0, 0, 0, 0, "Server port:", 0,
     NULL},
    {d_edit_proc, 455, 395, 170, 10, 0, 0, 0, 0, 5, 0, port, 0, NULL},

    {d_list_proc, 10, 315, 260, 40, 0, 0, 0, 0, 0, 0, _mode_getter, 0,
     NULL},

    {d_check_proc, 325, 315, 200, 10, 0, 0, 0, 0, 1, 0, "Only one game", 0,
     NULL},
    {d_check_proc, 325, 330, 200, 10, 0, 0, 0, 0, 1, 0,
     "Save log && screenshots", 0, NULL},

    {d_text_proc, 325, 345, 0, 0, 0, 0, 0, 0, 0, 0, "Password:", 0, NULL},
    {d_edit_proc, 455, 345, 170, 10, 0, 0, 0, 0, 6, 0, server_passwd, 0,
     NULL},

    {d_text_proc, 10, 240, 0, 0, 0, 0, 0, 0, 0, 0, "Password:", 0, NULL},
    {d_edit_proc, 140, 240, 170, 10, 0, 0, 0, 0, 6, 0, server_passwd, 0,
     NULL},

    {d_check_proc, 325, 360, 200, 10, 0, 0, 0, 0, 1, 0,
     "Wait for key (SPACE)", 0, NULL},

    {d_check_proc, 10, 355, 200, 10, 0, 0, 0, 0, 1, 0, "Torus Mode", 0,
     NULL},

    {0}
};

char *try_to_connect(const char *server_addr, int wait_time);

int start_server(const char *port);

inline int check_button(DIALOG * d, int n)
{
    return d[n].flags & D_SELECTED;
}

inline void unselect_button(DIALOG * d, int n)
{
    d[n].flags &= ~D_SELECTED;
    d[n].flags |= D_DIRTY;
}

void load_settings()
{
    int fps;

    def_res_x = get_config_int("server", "screen_w", 800);
    def_res_y = get_config_int("server", "screen_h", 600);
    fps = get_config_int("server", "fps", 30);
    if (fps < 1 || fps > MAX_FPS)
        fps = 30;
    sprintf(str_fps, "%d", fps);
    score_limit = get_config_int("server", "score_limit", 0);
    if (score_limit < 0 || score_limit > 999)
        score_limit = 0;
    sprintf(str_score_limit, "%d", score_limit);
    strncpy(server_addr,
            get_config_string("client", "server", "127.0.0.1:6789"), 30);
    strncpy(port, get_config_string("server", "port", "6789"), 5);
    strncpy(server_name,
            get_config_string("server", "name", "Netacka v" VER_STRING),
            15);

    one_game = get_config_int("server", "one_game", 0);
    save_log = get_config_int("server", "save_log", 0);
    wait_for_key = get_config_int("server", "wait_for_key", 0);
    hide_bot_numbers = get_config_int(0, "hide_bot_numbers", 0);
    gray_bg = get_config_int(0, "gray_bg", 0);
    instant_start = get_config_int("server", "instant_start", 0);
}


int start_old()
{
    DIALOG_PLAYER *dialog_player;
    NET_CHANNEL *chan = net_openchannel(net_driver, 0),
        *chan2 = net_openchannel(net_driver, 0);
    int done = 0, i = 0;
    int fps;
    int restart_server_list = 1;

    clear_keybuf();
    load_settings();

    if (one_game)
        the_list[POS_ONEGAME].flags |= D_SELECTED;
    if (save_log)
        the_list[POS_SAVELOG].flags |= D_SELECTED;
    if (wait_for_key)
        the_list[POS_WAITFORKEY].flags |= D_SELECTED;

    clear_bitmap(screen);
    hline(screen, 0, 310, 639, gui_fg_color);
    set_dialog_color(the_list, gui_fg_color, gui_bg_color);
    for (i = 0; the_list[i].proc; i++)
        if (the_list[i].proc == d_edit_proc)
            the_list[i].bg = palette_color[cDARKGRAY];
    the_list[POS_TRY_SERVER].bg = palette_color[cDARKGRAY];
    the_list[POS_START_SERVER].bg = palette_color[cDARKGRAY];

    if (get_config_int("server", "disable_changes", 0)) {
        int i;
        for (i = 0; to_disable[i] != -1; i++) {
            the_list[to_disable[i]].flags |= D_DISABLED;
        }
    }

    dialog_player = init_dialog(the_list, 0);

    escape = 0;

    show_mouse(screen);
    for (;;) {
        //show_mouse(NULL);
        update_dialog(dialog_player);
        rest(1);
        if (restart_server_list) {
            n_servers = 0;

            for (i = 0; i < SERVER_LIST_SIZE; i++) {
                servers[i].active = 0;
                servers[i].n_players = -1;
            }
            the_list[POS_SERVER_LIST].flags |= D_DIRTY;

            net_assigntarget(chan2, "255.255.255.255:6789");
            send_byte(chan2, clKNOCKKNOCK);

            // Also send to localhost, for testing.
            net_assigntarget(chan2, "127.0.0.1:6789");
            send_byte(chan2, clKNOCKKNOCK);

            restart_server_list = 0;
        }
        if (key[KEY_ESC] || escape) {
            done = 0;
            break;
        }
        if (check_button(the_list, POS_RELOAD_LIST)) {
            unselect_button(the_list, POS_RELOAD_LIST);
            restart_server_list = 1;
        }
        if (check_button(the_list, POS_TRY_SERVER)) {
            unselect_button(the_list, POS_TRY_SERVER);
            i = the_list[0].d1;
            if (servers[i].active) {
                char *err;

                if (!(err = try_to_connect(servers[i].addr, FPS * 2))) {
                    done = 1;
                    break;
                } else
                    alert(err, 0, 0, "OK", 0, 0, 0);
            }
        }

        if (check_button(the_list, POS_CONNECT)) {
            char *err;

            unselect_button(the_list, POS_CONNECT);
            if (!(err = try_to_connect(server_addr, FPS * 2))) {
                done = 1;
                break;
            } else
                alert(err, 0, 0, "OK", 0, 0, 0);
        }
        if (check_button(the_list, POS_START_SERVER)) {
            int res_num = the_list[POS_RES_LIST].d1;

            if (res_num == 0) {
                screen_w = def_res_x;
                screen_h = def_res_y;
            } else {
                screen_w = res_list[res_num - 1][0];
                screen_h = res_list[res_num - 1][1];
            }
            unselect_button(the_list, POS_START_SERVER);
            score_limit = atoi(str_score_limit);
            if (score_limit < 0 || score_limit > 999)
                score_limit = 0;
            fps = atoi(str_fps);
            if (fps < 1 || fps > MAX_FPS)
                fps = 30;
            FPS = fps;
            game_mode = game_modes[the_list[POS_MODE_LIST].d1].modenum;
            if (start_server(port)) {
                done = -1;
                break;
            } else {
                alert("Couldn't start server", 0, 0, "OK", 0, 0, 0);
                FPS = 20;
            }
        }
        while (net_query(chan2)) {
            char data[20];
            char from[50];

            net_receive(chan2, data, 20, from);
            for (i = 0; i < n_servers; i++) {
                if (servers[i].active)
                    if (!strcmp(from, servers[i].addr))
                        break;
            }
            if (*data == seMYINFO) {
                if (i == n_servers && n_servers < SERVER_LIST_SIZE) {
                    n_servers++;
                    strcpy(servers[i].addr, from);
                    servers[i].active = 1;
                }
                if (i != n_servers) {
                    servers[i].n_players = data[1];
                    strcpy(servers[i].name, &data[2]);
                    the_list[POS_SERVER_LIST].flags |= D_DIRTY;
                }
            }
        }
    }
    one_game = (the_list[POS_ONEGAME].flags & D_SELECTED) ? 1 : 0;
    save_log = (the_list[POS_SAVELOG].flags & D_SELECTED) ? 1 : 0;
    wait_for_key = (the_list[POS_WAITFORKEY].flags & D_SELECTED) ? 1 : 0;
    if (done == -1)
        torus = (the_list[POS_TORUS].flags & D_SELECTED) ? 1 : 0;
    show_mouse(NULL);
    shutdown_dialog(dialog_player);
    clear_bitmap(screen);
    net_closechannel(chan);
    net_closechannel(chan2);

    return done;
}

char *try_to_connect(const char *server_addr, int wait_time)
{
    char *err = 0;

    chan = net_openchannel(net_driver, 0);
    net_assigntarget(chan, server_addr);
    send_client_players(chan);
    ticks = 0;
    while (ticks <= wait_time) {
        rest(1);
        if (net_query(chan)) {
            unsigned char data[40];
            net_receive(chan, data, 40, 0);
            if (data[0] == seHEREARESETTNGS) {
                int k;
                FPS = data[1];
                screen_w = (data[2] << 8) + data[3];
                screen_h = (data[4] << 8) + data[5];
                for (k = 0; k < n_client_players; k++) {
                    client_players[k].num = data[6 + k];
                    players[data[6 + k]].client = -1;
                    players[data[6 + k]].client_num = k;
                    strcpy(players[data[6 + k]].name,
                           client_players[k].name);
                }
                torus = data[6 + n_client_players] & 1;
                game_mode = data[6 + n_client_players] >> 1;
                send_names_to_server();
                return 0;
            } else if (data[0] == seIMFULL) {
                err = "Server full";
                break;
            } else if (data[0] == seBADPASSWD) {
                err = "Bad password";
                break;
            }
        }
    }
    net_closechannel(chan);
    if (err)
        return err;
    else
        return "Couldn't connect";
}

int start_server(const char *port)
{
    char binding[30] = "0.0.0.0:";

    strcat(binding, port);
    if (!(chan = net_openchannel(net_driver, binding)))
        return 0;
    for (n_players = 0; n_players < n_client_players; n_players++) {
        players[n_players].client = -1;
        players[n_players].client_num = n_players;
        players[n_players].playing = 1;
        players[n_players].alive = 0;
        client_players[n_players].num = n_players;
        strcpy(players[n_players].name, client_players[n_players].name);
    }

    return 1;
}

int get_game_mode() {
    return game_mode;
}

int get_torus() {
    return torus;
}

void get_player_data(int i, int *x, int *y, int *a, int *last_da)
{
    if (x)
        *x = players[i].x;
    if (y)
        *y = players[i].y;
    if (a)
        *a = players[i].a;
    if (last_da)
        *last_da = players[i].last_da;
}

int is_player_active(int i) {
    return players[i].alive && players[i].playing;
}

int start()
{
    BITMAP *buf = create_bitmap(screen_w, screen_h);
    int reload_server_list = 1;
    NET_CHANNEL *chan = net_openchannel(net_driver, 0),
        *chan2 = net_openchannel(net_driver, 0);

    const char *error = NULL;

    show_os_cursor(1);

    load_settings();

    int done = 0;
    while (!(done || key[KEY_ESC] || escape)) {
        ui_handle_input();

        nk_begin(&ui, "settings", nk_rect(0, 0, 640, 480), 0);
        nk_layout_row_dynamic(&ui, 460, 2);

        nk_group_begin(&ui, "Connect to server", NK_WINDOW_TITLE|NK_WINDOW_BORDER);
        {
            nk_layout_row_dynamic(&ui, 30, 1);
            if (nk_button_label(&ui, "Reload server list")) {
                reload_server_list = 1;
            }
            nk_layout_row_dynamic(&ui, 10, 1);
            nk_layout_row_dynamic(&ui, 250, 1);
            nk_group_begin_titled(&ui, "servers", "Servers", NK_WINDOW_TITLE|NK_WINDOW_BORDER);
            {
                if (n_servers == 0) {
                    nk_layout_row_dynamic(&ui, 15, 1);
                    nk_label(&ui, "...", NK_TEXT_LEFT);
                } else {
                    for (int i = 0; i < n_servers; i++) {
                        if (!servers[i].active) {
                            continue;
                        }

                        if (servers[i].n_players == -1) {
                            nk_layout_row_dynamic(&ui, 15, 1);
                            nk_labelf_colored(&ui, NK_TEXT_LEFT, UI_LIGHT,
                                              "%s [not responding]", servers[i].addr);
                        } else {
                            nk_layout_row_dynamic(&ui, 15, 1);
                            nk_labelf_colored(&ui, NK_TEXT_LEFT, UI_VERY_LIGHT,
                                              "%s %s", servers[i].addr, servers[i].name);
                            nk_layout_row_dynamic(&ui, 15, 2);
                            nk_labelf_colored(&ui, NK_TEXT_LEFT, UI_LIGHT,
                                              "%d/%d players", servers[i].n_players, MAX_PLAYERS);
                            if (nk_button_label_styled(&ui, &ui_style_button_important, "Connect")) {
                                // ...
                            }
                        }
                        nk_layout_row_dynamic(&ui, 10, 1);
                    }
                }
            }
            nk_group_end(&ui);
            nk_layout_row_dynamic(&ui, 10, 1);
            nk_layout_row_dynamic(&ui, 20, 2);
            nk_label(&ui, "Server:", NK_TEXT_LEFT);
            nk_edit_string_zero_terminated(&ui, NK_EDIT_SIMPLE, server_addr, 30, nk_filter_default);
            nk_label(&ui, "Password:", NK_TEXT_LEFT);
            nk_edit_string_zero_terminated(&ui, NK_EDIT_SIMPLE, server_passwd, 6, nk_filter_default);
            nk_layout_row_dynamic(&ui, 10, 1);
            nk_layout_row_dynamic(&ui, 30, 1);
            if (nk_button_label(&ui, "Connect")) {
                // ...
                error = "Not implemented";
            }
        }
        nk_group_end(&ui);
        nk_group_begin(&ui, "Start server", NK_WINDOW_TITLE|NK_WINDOW_BORDER);
        {
            nk_layout_row_dynamic(&ui, 20, 2);
            nk_label(&ui, "Server name:", NK_TEXT_LEFT);
            nk_edit_string_zero_terminated(&ui, NK_EDIT_SIMPLE, server_name, 15, nk_filter_default);
            nk_label(&ui, "Server port:", NK_TEXT_LEFT);
            nk_edit_string_zero_terminated(&ui, NK_EDIT_SIMPLE, port, 5, nk_filter_decimal);
            nk_label(&ui, "Password:", NK_TEXT_LEFT);
            nk_edit_string_zero_terminated(&ui, NK_EDIT_SIMPLE, server_passwd, 6, nk_filter_decimal);

            nk_layout_row_dynamic(&ui, 10, 1);
            nk_layout_row_dynamic(&ui, 20, 1);
            nk_checkbox_label(&ui, "Only one game", &one_game);
            nk_checkbox_label(&ui, "Save log & screenshots", &save_log);
            nk_checkbox_label(&ui, "Wait for key (Space)", &wait_for_key);

            nk_layout_row_dynamic(&ui, 10, 1);
            nk_layout_row_dynamic(&ui, 20, 2);
            nk_label(&ui, "Score limit:", NK_TEXT_LEFT);
            nk_edit_string_zero_terminated(&ui, NK_EDIT_SIMPLE, str_score_limit, 3, nk_filter_decimal);
            nk_label(&ui, "FPS (game speed):", NK_TEXT_LEFT);
            nk_edit_string_zero_terminated(&ui, NK_EDIT_SIMPLE, str_fps, 2, nk_filter_decimal);

            nk_layout_row_dynamic(&ui, 10, 1);
            nk_layout_row_dynamic(&ui, 20, 1);
            nk_label(&ui, "Resolution:", NK_TEXT_LEFT);
            nk_layout_row_dynamic(&ui, 20, 1);
            nk_combobox_callback(&ui, get_res_label, NULL, &res_num, N_RES+1, 15, nk_vec2(300, 120));

            nk_layout_row_dynamic(&ui, 10, 1);
            nk_layout_row_dynamic(&ui, 20, 1);
            nk_combobox(&ui, game_mode_names, 3, &game_mode, 15, nk_vec2(250, 70));
            nk_layout_row_dynamic(&ui, 20, 1);
            nk_checkbox_label(&ui, "Torus Mode", &torus);

            nk_layout_row_dynamic(&ui, 10, 1);
            nk_layout_row_dynamic(&ui, 30, 1);
            if (nk_button_label_styled(&ui, &ui_style_button_important, "Start Server")) {
                // ...
                error = "Not implemented";
            }
        }
        nk_group_end(&ui);

        if (error) {
            if (nk_popup_begin(&ui, NK_POPUP_DYNAMIC, "Error", NK_WINDOW_TITLE|NK_WINDOW_BORDER,
                               nk_rect(200, 180, 240, 100))) {
                nk_layout_row_dynamic(&ui, 20, 1);
                nk_label(&ui, error, NK_TEXT_LEFT);
                if (nk_button_label(&ui, "OK")) {
                    error = NULL;
                    nk_popup_close(&ui);
                }
                nk_popup_end(&ui);
            } else {
                error = NULL;
            }
        }

        nk_end(&ui);



        rest(1);
        clear_bitmap(buf);
        ui_draw(buf);
        blit(buf, screen, 0, 0, 0, 0, screen_w, screen_h);

        if (reload_server_list) {
            n_servers = 0;

            for (int i = 0; i < SERVER_LIST_SIZE; i++) {
                servers[i].active = 0;
                servers[i].n_players = -1;
            }

            net_assigntarget(chan2, "255.255.255.255:6789");
            send_byte(chan2, clKNOCKKNOCK);

            // Also send to localhost, for testing.
            net_assigntarget(chan2, "127.0.0.1:6789");
            send_byte(chan2, clKNOCKKNOCK);

            reload_server_list = 0;
        }
        while (net_query(chan2)) {
            char data[20];
            char from[50];

            net_receive(chan2, data, 20, from);
            int i;
            for (i = 0; i < n_servers; i++) {
                if (servers[i].active)
                    if (!strcmp(from, servers[i].addr))
                        break;
            }
            if (*data == seMYINFO) {
                if (i == n_servers && n_servers < SERVER_LIST_SIZE) {
                    n_servers++;
                    strcpy(servers[i].addr, from);
                    servers[i].active = 1;
                }
                if (i != n_servers) {
                    servers[i].n_players = data[1];
                    strcpy(servers[i].name, &data[2]);
                }
            }
        }
    }

    show_mouse(NULL);
    destroy_bitmap(buf);
    clear_bitmap(screen);
    net_closechannel(chan);
    net_closechannel(chan2);
    return 0;
}

#include "libs/eadk.h"
#include <stdio.h>
#include <stdbool.h>
#include "fps.h"
#include "jpg.h"
#include "game.h"

// Variables du menu de triche
bool cheat_menu_open = false;
int cheat_selected_option = 0;
bool god_mode = false;
int player_speed = 8; 

const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "HollowKnight";
const uint32_t eadk_api_level  __attribute__((section(".rodata.eadk_api_level"))) = 0;

map_point_t camera = {5000, 5000};
map_point_t player = {5000, 5000};

#define TILE_WIDTH 320
#define TILE_HEIGHT 240
#define TILE_COLS 56
#define TILE_ROWS 49

static inline bool tile_in_bounds(int16_t col, int16_t row) {
    return col >= 0 && col < TILE_COLS && row >= 0 && row < TILE_ROWS;
}

static inline bool tile_visible_on_screen(int16_t screen_x, int16_t screen_y) {
    return (screen_x < EADK_SCREEN_WIDTH && (screen_x + TILE_WIDTH) > 0 &&
            screen_y < EADK_SCREEN_HEIGHT && (screen_y + TILE_HEIGHT) > 0);
}

static void draw_visible_tiles(int16_t tile_col, int16_t tile_row, int16_t pixel_offset_x, int16_t pixel_offset_y) {
    for (int dy = 0; dy < 2; dy++) {
        for (int dx = 0; dx < 2; dx++) {
            int16_t current_col = tile_col + dx;
            int16_t current_row = tile_row + dy;

            int16_t screen_x = dx * TILE_WIDTH - pixel_offset_x;
            int16_t screen_y = dy * TILE_HEIGHT - pixel_offset_y;

            if (tile_visible_on_screen(screen_x, screen_y) && tile_in_bounds(current_col, current_row)) {
                jpg_draw_tile(current_row, current_col, (map_point_t){screen_x, screen_y});
            }
        }
    }
}

void display_player(){
    int16_t screen_x = player.x - camera.x;
    int16_t screen_y = player.y - camera.y;
    eadk_display_push_rect_uniform((eadk_rect_t){screen_x, screen_y, 12, 24}, eadk_color_white);
}

// Affichage dynamique du menu avec l'état [ON] / [OFF]
void draw_cheat_menu() {
    if (!cheat_menu_open) return;

    eadk_display_push_rect_uniform((eadk_rect_t){40, 40, 240, 160}, eadk_color_black);
    eadk_display_draw_string("--- SOH CHEAT MENU ---", (eadk_point_t){50, 50}, true, eadk_color_yellow, eadk_color_black);

    char buf_god[32];
    sprintf(buf_god, "God Mode : %s", god_mode ? "ON" : "OFF");
    if (cheat_selected_option == 0) {
        eadk_display_draw_string(buf_god, (eadk_point_t){60, 90}, true, eadk_color_green, eadk_color_black);
    } else {
        eadk_display_draw_string(buf_god, (eadk_point_t){60, 90}, true, eadk_color_white, eadk_color_black);
    }

    char buf_spd[32];
    sprintf(buf_spd, "Vitesse x2 : %s", (player_speed > 8) ? "ON" : "OFF");
    if (cheat_selected_option == 1) {
        eadk_display_draw_string(buf_spd, (eadk_point_t){60, 120}, true, eadk_color_green, eadk_color_black);
    } else {
        eadk_display_draw_string(buf_spd, (eadk_point_t){60, 120}, true, eadk_color_white, eadk_color_black);
    }
}

int main(void) {
    eadk_display_push_rect_uniform(eadk_screen_rect, eadk_color_black);

    fps_manager_t* fps = fps_manager_create(TARGET_FPS);
    
    while (1) {

        fps_stats_t stats;
        int dx = 0;
        int dy = 0;
        fps_manager_start_frame(fps);

        /*------------------------------------------*/

        eadk_keyboard_state_t state = eadk_keyboard_scan();
        if (eadk_keyboard_key_down(state, eadk_key_home)) break;

        // Ouvrir/Fermer le menu avec la touche OK (ou EXE)
        static bool ok_pressed_last = false;
        bool ok_pressed_now = eadk_keyboard_key_down(state, eadk_key_ok);
        if (ok_pressed_now && !ok_pressed_last) {
            cheat_menu_open = !cheat_menu_open;
        }
        ok_pressed_last = ok_pressed_now;

        if (cheat_menu_open) {
            // Navigation Haut / Bas dans le menu
            static bool up_last = false, down_last = false, right_last = false;
            
            bool up_now = eadk_keyboard_key_down(state, eadk_key_up);
            if (up_now && !up_last) {
                cheat_selected_option = (cheat_selected_option - 1 + 2) % 2;
            }
            up_last = up_now;

            bool down_now = eadk_keyboard_key_down(state, eadk_key_down);
            if (down_now && !down_last) {
                cheat_selected_option = (cheat_selected_option + 1) % 2;
            }
            down_last = down_now;

            // Activer / Désactiver avec la touche Droite
            bool right_now = eadk_keyboard_key_down(state, eadk_key_right);
            if (right_now && !right_last) {
                if (cheat_selected_option == 0) {
                    god_mode = !god_mode;
                } else if (cheat_selected_option == 1) {
                    player_speed = (player_speed == 8) ? 16 : 8;
                }
            }
            right_last = right_now;

        } else {
            // Déplacement normal du joueur si le menu est fermé
            if (eadk_keyboard_key_down(state, eadk_key_left)) dx -= player_speed;
            if (eadk_keyboard_key_down(state, eadk_key_right)) dx += player_speed;
            if (eadk_keyboard_key_down(state, eadk_key_up)) dy -= player_speed;
            if (eadk_keyboard_key_down(state, eadk_key_down)) dy += player_speed;
        }

        player.x += dx;
        player.y += dy;

        if (player.x < 0) player.x = 0;
        if (player.y < 0) player.y = 0;
        if (player.x > MAP_WIDTH - 320) player.x = MAP_WIDTH - 320;
        if (player.y > MAP_HEIGHT - 240) player.y = MAP_HEIGHT - 240;

        if (player.x < camera.x + 60) {
            camera.x = player.x - 60;
        } else if (player.x > camera.x + 240) {
            camera.x = player.x - 240;
        }
        if (player.y < camera.y + 45) {
            camera.y = player.y - 45;
        } else if (player.y > camera.y + 180) {
            camera.y = player.y - 180;
        }

        int16_t tile_col = camera.x / 320;
        int16_t tile_row = camera.y / 240;
        int16_t pixel_offset_x = camera.x % 320;
        int16_t pixel_offset_y = camera.y % 240;
        
        draw_visible_tiles(tile_col, tile_row, pixel_offset_x, pixel_offset_y);
        display_player();

        // Dessiner le menu par-dessus
        draw_cheat_menu();

        /*------------------------------------------*/

        fps_manager_end_frame(fps, &stats);
        fps_manager_cap_frame(fps, &stats);
    }
    fps_manager_destroy(fps);

    return 0;
}

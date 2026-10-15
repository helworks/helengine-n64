#pragma once
#include <cstdint>

enum joypad_port_t { JOYPAD_PORT_1, JOYPAD_PORT_2, JOYPAD_PORT_3, JOYPAD_PORT_4 };
enum joypad_style_t { JOYPAD_STYLE_NONE, JOYPAD_STYLE_N64 };
struct joypad_inputs_t {
    struct {
        bool a = false, b = false, c_left = false, c_up = false;
        bool l = false, r = false, z = false, start = false;
        bool d_up = false, d_down = false, d_left = false, d_right = false;
    } btn;
    int8_t stick_x = 0;
    int8_t stick_y = 0;
};
void joypad_init();
void joypad_close();
void joypad_poll();
joypad_style_t joypad_get_style(joypad_port_t port);
joypad_inputs_t joypad_get_inputs(joypad_port_t port);

#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

#define MOD_SHIFT 1
#define MOD_CTRL 2
#define MOD_ALT 4
#define MOD_SUPER 8

typedef enum {
  KEY_Q = 1,
  KEY_W,
  KEY_E,
  KEY_R,
  KEY_T,
  KEY_Y,
  KEY_U,
  KEY_I,
  KEY_O,
  KEY_P,
  KEY_A,
  KEY_S,
  KEY_D,
  KEY_F,
  KEY_G,
  KEY_H,
  KEY_J,
  KEY_K,
  KEY_L,
  KEY_Z,
  KEY_X,
  KEY_C,
  KEY_V,
  KEY_B,
  KEY_N,
  KEY_M,
  KEY_COLON,
  KEY_SEMICOLON,
  KEY_SQUOTE,
  KEY_COMMA,
  KEY_PERIOD,
  KEY_MINUS,
  KEY_SLASH,
  KEY_SPACE,
  KEY_ENTER,
  KEY_BACKSPACE,
  KEY_ESCAPE,
  KEY_CTRL,
  KEY_LSUPER,
  KEY_RSUPER,
  KEY_ALT,
  KEY_LSHIFT,
  KEY_RSHIFT,
  KEY_1,
  KEY_2,
  KEY_3,
  KEY_4,
  KEY_5,
  KEY_6,
  KEY_7,
  KEY_8,
  KEY_9,
  KEY_0,

  KEY_T_MAX,
} key_t;

#define KEY_T_MAX KEY_T_MAX

typedef struct {
  key_t key;
  uint8_t pressed;
  uint8_t modifiers;
} key_event;

void handle_keyboard_interrupt(uint8_t scancode);
key_event get_keyboard_char();

#endif

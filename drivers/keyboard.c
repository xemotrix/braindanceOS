#include "keyboard.h"
#include "../drivers/ports.h"
#include "../drivers/vga_text.h"
#include "../utils/ring_buffer.h"

key_t keycodes[UINT8_MAX] = {
    [0x10] = KEY_Q,      [0x11] = KEY_W,         [0x12] = KEY_E,
    [0x13] = KEY_R,      [0x14] = KEY_T,         [0x15] = KEY_Y,
    [0x16] = KEY_U,      [0x17] = KEY_I,         [0x18] = KEY_O,
    [0x19] = KEY_P,      [0x1e] = KEY_A,         [0x1f] = KEY_S,
    [0x20] = KEY_D,      [0x21] = KEY_F,         [0x22] = KEY_G,
    [0x23] = KEY_H,      [0x24] = KEY_J,         [0x25] = KEY_K,
    [0x26] = KEY_L,      [0x2c] = KEY_Z,         [0x2d] = KEY_X,
    [0x2e] = KEY_C,      [0x2f] = KEY_V,         [0x30] = KEY_B,
    [0x31] = KEY_N,      [0x32] = KEY_M,         [0x27] = KEY_SEMICOLON,
    [0x33] = KEY_COMMA,  [0x34] = KEY_PERIOD,    [0x0c] = KEY_MINUS,
    [0x35] = KEY_SLASH,  [0x28] = KEY_SQUOTE,    [0x39] = KEY_SPACE,
    [0x1c] = KEY_ENTER,  [0x0e] = KEY_BACKSPACE, [0x01] = KEY_ESCAPE,
    [0x1d] = KEY_CTRL,   [0x38] = KEY_ALT,       [0x5b] = KEY_LSUPER,
    [0x5c] = KEY_RSUPER, [0x2a] = KEY_LSHIFT,    [0x36] = KEY_RSHIFT,
    [0x02] = KEY_1,      [0x03] = KEY_2,         [0x04] = KEY_3,
    [0x05] = KEY_4,      [0x06] = KEY_5,         [0x07] = KEY_6,
    [0x08] = KEY_7,      [0x09] = KEY_8,         [0x0a] = KEY_9,
    [0x0b] = KEY_0,
};

#define KEY_EVENT_BUF_SIZE 255
RBDEF(key_event, KEY_EVENT_BUF_SIZE);
key_event_RB key_events = {0};
volatile uint16_t unread = 0;

#define MOD_KEY_LSHIFT 1
#define MOD_KEY_RSHIFT 2
#define MOD_KEY_CTRL 4
#define MOD_KEY_ALT 8
#define MOD_KEY_LSUPER 16
#define MOD_KEY_RSUPER 32
uint8_t mod_mask = 0;

key_event get_keyboard_char() {
  while (unread == 0) {
    __asm__ volatile("hlt");
  }
  key_event ke = rb_read(key_events);
  unread--;
  return ke;
}

void handle_keyboard_interrupt(uint8_t scancode) {
  uint8_t is_press = !((scancode & 0x80) != 0);

  if (!is_press) {
    scancode = scancode & ~0x80;
    key_t kc = keycodes[scancode];
    if (kc == KEY_CTRL)
      mod_mask &= ~MOD_KEY_CTRL;
    if (kc == KEY_ALT)
      mod_mask &= ~MOD_KEY_ALT;
    if (kc == KEY_LSUPER)
      mod_mask &= ~MOD_KEY_LSUPER;
    if (kc == KEY_RSUPER)
      mod_mask &= ~MOD_KEY_RSUPER;
    if (kc == KEY_LSHIFT)
      mod_mask &= ~MOD_KEY_LSHIFT;
    if (kc == KEY_RSHIFT)
      mod_mask &= ~MOD_KEY_RSHIFT;
    outb(0x20, 0x20);
    return;
  }

  key_t kc = keycodes[scancode];

  if (!kc) {
    outb(0x20, 0x20);
    put_char('<');
    put_number(scancode);
    put_char('>');
    return;
  }

  if (kc == KEY_CTRL)
    mod_mask |= MOD_KEY_CTRL;
  if (kc == KEY_ALT)
    mod_mask |= MOD_KEY_ALT;
  if (kc == KEY_LSUPER)
    mod_mask |= MOD_KEY_LSUPER;
  if (kc == KEY_RSUPER)
    mod_mask |= MOD_KEY_RSUPER;
  if (kc == KEY_LSHIFT)
    mod_mask |= MOD_KEY_LSHIFT;
  if (kc == KEY_RSHIFT)
    mod_mask |= MOD_KEY_RSHIFT;

  uint8_t modifiers = ((((MOD_KEY_LSHIFT & mod_mask) != 0) ||
                        ((MOD_KEY_RSHIFT & mod_mask) != 0)) *
                       MOD_SHIFT)                                     //
                      | (((MOD_KEY_CTRL & mod_mask) != 0) * MOD_CTRL) //
                      | (((MOD_KEY_ALT & mod_mask) != 0) * MOD_ALT)   //
                      | ((((MOD_KEY_LSUPER & mod_mask) != 0) ||
                          ((MOD_KEY_RSUPER & mod_mask) != 0)) *
                         MOD_SUPER);

  key_event evt = {
      .key = kc,
      .pressed = 1,
      .modifiers = modifiers,
  };

  rb_write(key_events, evt);
  unread++;
}

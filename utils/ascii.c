#include "ascii.h"

static char lower[KEY_T_MAX] = {
    [KEY_Q] = 'q',     [KEY_W] = 'w',       [KEY_E] = 'e',
    [KEY_R] = 'r',     [KEY_T] = 't',       [KEY_Y] = 'y',
    [KEY_U] = 'u',     [KEY_I] = 'i',       [KEY_O] = 'o',
    [KEY_P] = 'p',     [KEY_A] = 'a',       [KEY_S] = 's',
    [KEY_D] = 'd',     [KEY_F] = 'f',       [KEY_G] = 'g',
    [KEY_H] = 'h',     [KEY_J] = 'j',       [KEY_K] = 'k',
    [KEY_L] = 'l',     [KEY_Z] = 'z',       [KEY_X] = 'x',
    [KEY_C] = 'c',     [KEY_V] = 'v',       [KEY_B] = 'b',
    [KEY_N] = 'n',     [KEY_M] = 'm',       [KEY_SEMICOLON] = ';',
    [KEY_COMMA] = ',', [KEY_PERIOD] = '.',  [KEY_MINUS] = '-',
    [KEY_SLASH] = '/', [KEY_SQUOTE] = '\'', [KEY_SPACE] = ' ',
    [KEY_1] = '1',     [KEY_2] = '2',       [KEY_3] = '3',
    [KEY_4] = '4',     [KEY_5] = '5',       [KEY_6] = '6',
    [KEY_7] = '7',     [KEY_8] = '8',       [KEY_9] = '9',
    [KEY_0] = '0',
};

static char upper[KEY_T_MAX] = {
    [KEY_Q] = 'Q',     [KEY_W] = 'W',      [KEY_E] = 'E',
    [KEY_R] = 'R',     [KEY_T] = 'T',      [KEY_Y] = 'Y',
    [KEY_U] = 'U',     [KEY_I] = 'I',      [KEY_O] = 'O',
    [KEY_P] = 'P',     [KEY_A] = 'A',      [KEY_S] = 'S',
    [KEY_D] = 'D',     [KEY_F] = 'F',      [KEY_G] = 'G',
    [KEY_H] = 'H',     [KEY_J] = 'J',      [KEY_K] = 'K',
    [KEY_L] = 'L',     [KEY_Z] = 'Z',      [KEY_X] = 'X',
    [KEY_C] = 'C',     [KEY_V] = 'V',      [KEY_B] = 'B',
    [KEY_N] = 'N',     [KEY_M] = 'M',      [KEY_SEMICOLON] = ';',
    [KEY_COMMA] = '<', [KEY_PERIOD] = '>', [KEY_MINUS] = '_',
    [KEY_SLASH] = '?', [KEY_SQUOTE] = '"', [KEY_SPACE] = ' ',
    [KEY_1] = '!',     [KEY_2] = '@',      [KEY_3] = '#',
    [KEY_4] = '$',     [KEY_5] = '%',      [KEY_6] = '^',
    [KEY_7] = '&',     [KEY_8] = '*',      [KEY_9] = '(',
    [KEY_0] = ')',

};

char key_event_to_char(key_event evt) {
  if (!evt.pressed)
    return 0;

  if (evt.modifiers == 0) {
    // zero if doesn't correspond
    return lower[evt.key];
  }
  if (evt.modifiers == MOD_SHIFT) {
    return upper[evt.key];
  }
  return 0;
}

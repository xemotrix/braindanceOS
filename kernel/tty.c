#include "tty.h"
#include "../drivers/keyboard.h"
#include "../drivers/vga_text.h"
#include "../utils/ascii.h"
#include "../utils/dynamic_array.h"
#include "../utils/stdio.h"
#include "../utils/string_view.h"
#include "panic.h"
#include "scheduler.h"
#include "vfs.h"

SV_array lines;

static void run_cmd_ls() {
  size_t fd = vfs_open("/hello_disk.txt");

  char buf[256];
  size_t bytes_read = vfs_read(fd, buf, 256);
  kprintf("bytes read: %d\n", bytes_read);
  kprintf("contents: '%s'\n", buf);

  if (!vfs_close(fd))
    PANIC("error closing");
}

static void run_cmd_ps() {
  struct scheduler_info si = scheduler_info();
  kprintf("time since boot: %l\n", si.time_since_boot);
  for (size_t i = 0; i < si.tasks.len; i++) {
    struct scheduler_task_info ti = si.tasks.elts[i];
    kprintf("- %d:%s:", ti.pid, ti.name);
    switch (ti.state) {
    case TASK_STATE_RUNNING:
      kprintf(" state:RUN", ti.pid);
      break;
    case TASK_STATE_READY:
      kprintf(" state:RDY", ti.pid);
      break;
    case TASK_STATE_SLEEPING:
      kprintf(" state:SLE", ti.pid);
      break;
    case TASK_STATE_WAITING:
      kprintf(" state:WAI", ti.pid);
      break;
    case TASK_STATE_UNUSED:
      PANIC("returned unused task");
    }
    kprintf(" time:%l\n", ti.time_used);
  }
}

void parse_cmd(SV cmd) {
  SV_array words = {0};

  size_t word_len_counter = 0;
  size_t i;
  for (i = 0; i < cmd.len; i++) {
    char c = cmd.str[i];
    if (c == ' ') {
      if (word_len_counter) {
        SV w = svsubstring(cmd, i - word_len_counter, word_len_counter);
        DA_APPEND(words, w);
      }
      word_len_counter = 0;
      continue;
    }
    word_len_counter++;
  }

  if (word_len_counter) {
    SV w = svsubstring(cmd, i - word_len_counter, word_len_counter);
    DA_APPEND(words, w);
  }

  SV cmd_name = words.elts[0];
  if (sveq(cmd_name, "ps")) {
    run_cmd_ps();
  } else if (sveq(cmd_name, "ls")) {
    run_cmd_ls();
  } else {
    kprintf("unrecognized command\n");
  }

  DA_FREE(words);
}

void run_tty() {
  clear_screen();
  set_cursor(0, 0);
  kprintf(">");

  struct SV_builder current_line = {0};

  for (;;) {
    key_event ke = get_keyboard_char();

    switch (ke.key) {
    case KEY_ENTER:
      if (!current_line.char_array.len)
        continue;

      kprintf("\n");
      SV line = svbuild(current_line);
      DA_FREE(current_line.char_array);
      current_line = (struct SV_builder){0};

      parse_cmd(line);
      kprintf(">");
      break;
    case KEY_BACKSPACE:
      if (!current_line.char_array.len)
        continue;
      current_line.char_array.len--;
      cursor_back();
      kprintf(" ");
      cursor_back();

      break;
    default:
      char c = key_event_to_char(ke);
      if (!c)
        continue;
      svbuilder_write_char(&current_line, c);
      kprintf("%c", c);

      SV svline = svbuilder_view(&current_line);
    }
  }
}

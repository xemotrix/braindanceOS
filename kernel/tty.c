#include "tty.h"
#include "panic.h"
#include "scheduler.h"
#include "vfs.h"
#include <drivers/keyboard.h>
#include <drivers/vga_text.h>
#include <stdio.h>
#include <string.h>
#include <utils/ascii.h>
#include <utils/dynamic_array.h>
#include <utils/string_view.h>

SV_array lines;

static void run_cmd_ls(SV_array cmd) {
  size_t fd;
  if (cmd.len < 2) {
    fd = vfs_open("/");
  } else {
    char *cstr = svcstring(cmd.elts[1]);
    fd = vfs_open(cstr);
    kfree(cstr);
  }
  vfs_dentry *de = 0;
  while ((de = vfs_readdir(fd))) {
    if (de->is_dir)
      printf("D ");
    else
      printf("F ");
    printf("%x %s\n", de->size, de->name);
  }

  if (!vfs_close(fd))
    PANIC("error closing");
}

static void run_cmd_write(SV_array cmd) {
  if (cmd.len < 3) {
    printf("invalid write command");
    return;
  }

  char *path = svcstring(cmd.elts[1]);
  size_t fd = vfs_open(path);
  if (fd < 0) {
    printf("couldn't open file %s\n", path);
    return;
  }
  kfree(path);

  char *to_write = svcstring(cmd.elts[2]);
  vfs_write(fd, to_write, cmd.elts[2].len);
  kfree(to_write);

  if (!vfs_close(fd))
    PANIC("error closing");
}

static void run_cmd_cat(SV_array cmd) {
  char buf[512] = {0};
  size_t fd;
  if (cmd.len < 2) {
    printf("must provide path to file\n");
    return;
  } else {
    char *cstr = svcstring(cmd.elts[1]);
    fd = vfs_open(cstr);
    kfree(cstr);
  }
  size_t size;
  while ((size = vfs_read(fd, buf, sizeof(buf)))) {
    printf("%s", buf);
  };
  printf("\n");
  if (!vfs_close(fd))
    PANIC("error closing");
}

static void run_cmd_ps() {
  struct scheduler_info si = scheduler_info();
  printf("time since boot: %l\n", si.time_since_boot);
  for (size_t i = 0; i < si.tasks.len; i++) {
    struct scheduler_task_info ti = si.tasks.elts[i];
    printf("- %d:%s:", ti.pid, ti.name);
    switch (ti.state) {
    case TASK_STATE_RUNNING:
      printf(" state:RUN", ti.pid);
      break;
    case TASK_STATE_READY:
      printf(" state:RDY", ti.pid);
      break;
    case TASK_STATE_SLEEPING:
      printf(" state:SLE", ti.pid);
      break;
    case TASK_STATE_WAITING:
      printf(" state:WAI", ti.pid);
      break;
    case TASK_STATE_UNUSED:
      PANIC("returned unused task");
    }
    printf(" time:%l\n", ti.time_used);
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
    run_cmd_ls(words);
  } else if (sveq(cmd_name, "cat")) {
    run_cmd_cat(words);
  } else if (sveq(cmd_name, "write")) {
    run_cmd_write(words);
  } else {
    printf("unrecognized command\n");
  }

  DA_FREE(words);
}

void run_tty() {
  vga_clear_screen();
  vga_set_cursor(0, 0);
  printf(">");

  struct SV_builder current_line = {0};

  for (;;) {
    key_event ke = get_keyboard_char();

    switch (ke.key) {
    case KEY_ENTER:
      if (!current_line.char_array.len)
        continue;

      printf("\n");
      SV line = svbuild(current_line);
      DA_FREE(current_line.char_array);
      current_line = (struct SV_builder){0};

      parse_cmd(line);
      printf(">");
      break;
    case KEY_BACKSPACE:
      if (!current_line.char_array.len)
        continue;
      current_line.char_array.len--;
      vga_cursor_back();
      printf(" ");
      vga_cursor_back();

      break;
    default:
      char c = key_event_to_char(ke);
      if (!c)
        continue;
      svbuilder_write_char(&current_line, c);
      printf("%c", c);

      SV svline = svbuilder_view(&current_line);
    }
  }
}

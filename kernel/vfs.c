#include "vfs.h"
#include "panic.h"
#include <string.h>
#include <utils/string_view.h>

// an instance of an open file
struct vfs_file {
  vfs_node *node;
  size_t cursor;
};

#define MAX_NODES 256
#define MAX_FDS 1024

//====STATE=====================
struct vfs_file fds[MAX_FDS];
vfs_node nodes[MAX_NODES];
vfs_node root;
vfs_ops ops;

//==============================

void vfs_init(vfs_init_data init) {
  ops = init.ops;
  root = (vfs_node){
      .parent = NULL,
      .dentry =
          (vfs_dentry){
              .name = "/",
              .is_dir = true,
              .size = 0,
              .fs_data = init.root_fs_data,
          },
  };
}

static vfs_node *save_node(vfs_node node) {
  for (size_t i = 0; i < MAX_NODES; i++) {
    if (nodes[i].parent == node.parent &&
        strcmp(node.dentry.name, nodes[i].dentry.name) == 0) {
      nodes[i] = node;
      return &nodes[i];
    }
    if (nodes[i].dentry.fs_data == NULL) {
      nodes[i] = node;
      return &nodes[i];
    }
  }
  return 0;
}

static int create_file_descriptor_for_node(vfs_node *node) {
  for (size_t i = 0; i < MAX_FDS; i++) {
    if (fds[i].node == NULL) {
      fds[i] = (struct vfs_file){
          .node = node,
          .cursor = 0,
      };
      return i;
    }
  }
  PANIC("no space for file descriptor!");
  return -1;
}

static vfs_node *lookup(vfs_node *parent, SV path_segment) {
  for (size_t i = 0; i < MAX_NODES; i++) {
    if (nodes[i].parent == parent && sveq(path_segment, nodes[i].dentry.name)) {
      return &nodes[i];
    }
  }

  struct vfs_file file = {
      .cursor = 0,
      .node = parent,
  };
  vfs_dentry out = {0};
  while (ops.readdir(parent->dentry.fs_data, &file.cursor, &out)) {
    if (sveq(path_segment, out.name)) {
      vfs_node *res = save_node((vfs_node){.parent = parent, .dentry = out});
      return res;
    }
  }
  return 0;
}

static SV parse_path_segment(const char **path) {
  SV sv = {.str = *path, .len = 0};
  while (**path) {
    sv.len++;
    (*path)++;
  }
  return sv;
}

int vfs_open(const char *path) {
  vfs_node *node;
  if (*path == '/') {
    node = &root;
    path++;
  } else {
    PANIC("relative paths not implemneted");
  }

  for (;;) {
    SV ps = parse_path_segment(&path);
    if (!ps.len)
      break;
    vfs_node *next = lookup(node, ps);
    if (!next)
      return -1; // not found
    node = next;
  }
  int fd = create_file_descriptor_for_node(node);
  return fd;
}

int vfs_close(size_t fd) {
  fds[fd].node = 0;
  fds[fd].cursor = 0;
  return 1;
}

int vfs_seek(size_t fd, size_t offset, uint8_t whence) {
  struct vfs_file *file = &fds[fd];
  size_t target;
  switch (whence) {
  case SEEK_SET:
    target = offset;
    break;
  case SEEK_CUR:
    target = file->cursor + offset;
    break;
  case SEEK_END:
    target = file->node->dentry.size + offset;
    break;
  }
  file->cursor = target;
  return target;
}

int vfs_write(size_t fd, const void *buf, size_t count) {
  if (!count)
    return 0;
  struct vfs_file *file = &fds[fd];
  vfs_node *node = file->node;
  int res = ops.write(node->dentry.fs_data, &file->cursor, buf, count);
  if (file->cursor > node->dentry.size)
    node->dentry.size = file->cursor;

  return res;
}

int vfs_read(size_t fd, void *buf, size_t count) {
  if (!count)
    return 0;
  struct vfs_file *file = &fds[fd];
  if (file->cursor >= file->node->dentry.size)
    return 0;
  vfs_node *node = file->node;
  size_t bytes_read = ops.read(node->dentry.fs_data, &file->cursor, buf, count);
  return bytes_read;
}

vfs_dentry *vfs_readdir(size_t fd) {
  struct vfs_file *file = &fds[fd];
  vfs_node *parent = file->node;

  if (!parent)
    PANIC("node empty");

  vfs_dentry out = {0};
  if (!ops.readdir(parent->dentry.fs_data, &file->cursor, &out))
    return 0;

  vfs_node *res = save_node((vfs_node){.parent = parent, .dentry = out});
  return &res->dentry;
}

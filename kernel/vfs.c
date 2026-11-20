#include "vfs.h"
#include "panic.h"

enum vfs_info_node_type {
  VFS_INFO_NODE_TYPE_UNUSED,
  VFS_INFO_NODE_TYPE_DIR,
  VFS_INFO_NODE_TYPE_FILE,
};

// info about the underlying file, not dependent on who opens it
struct vfs_info_node {
  enum vfs_info_node_type type;
  size_t size;
  void *fs_data;
  vfs_ops fs_ops;
};

// an instance of an open file
struct vfs_file {
  struct vfs_info_node *inode;
  size_t cursor;
};

#define MAX_INFO_NODES 256
#define MAX_FDS 1024
#define MAX_DENTRIES 1024

struct vfs_info_node inodes[MAX_INFO_NODES];
struct vfs_file fds[MAX_FDS];
vfs_dentry dentries[MAX_DENTRIES];

void vfs_init(vfs_init_data init) {
  inodes[0] = (struct vfs_info_node){
      .size = 0,
      .type = VFS_INFO_NODE_TYPE_DIR,
      .fs_data = init.root_fs_data,
      .fs_ops = init.ops,
  };
  dentries[0] = (vfs_dentry){
      .name = "/",
      .inode = &inodes[0],
      .is_dir = true,
      .size = 0,
  };
}

size_t vfs_open_root() {
  size_t fd = MAX_FDS + 1;
  for (size_t i = 0; i < MAX_FDS; i++) {
    if (fds[i].inode == NULL) {
      fd = i;
      break;
    }
  }
  if (fd > MAX_FDS)
    PANIC("no space for file descriptor");
  fds[fd] = (struct vfs_file){
      .inode = &inodes[0],
      .cursor = 0,
  };
  return fd;
}

int vfs_close(size_t fd) {
  fds[fd].inode = 0;
  fds[fd].cursor = 0;
  return 1;
}

int vfs_readdir(size_t fd, vfs_dentry *out) {
  struct vfs_file *file = &fds[fd];
  struct vfs_info_node *in = file->inode;
  if (!in)
    PANIC("inode empty");
  int res = in->fs_ops.readdir(in->fs_data, &file->cursor, out);
  out->inode = in;
  return res;
}

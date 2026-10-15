#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define MAX_DEPTH 64
#define MAXPATH 512

static char prefix[MAX_DEPTH][5];

// count valid entries in directory
static int count_entries(int fd, char *buf, char *p, int onlyDir)
{
  struct dirent de;
  struct stat st;
  int count = 0;

  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0)
      continue;
    if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
      continue;

    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;

    if (stat(buf, &st) < 0)
      continue;
    if (onlyDir && st.type != T_DIR)
      continue;

    count++;
  }
  return count;
}

// recursive function to display tree
void tree_r(char *path, int level, int maxDepth, int onlyDir)
{
  int fd;
  struct dirent de;
  struct stat st, st2;
  char buf[MAXPATH];
  char *p;
  int count, idx = 0;
  int isLast, i;

  if (level > maxDepth)
    return;

  // open directory
  if ((fd = open(path, O_RDONLY)) < 0) {
    fprintf(2, "tree: cannot open %s\n", path);
    return;
  }
  if (fstat(fd, &st) < 0) {
    fprintf(2, "tree: cannot stat %s\n", path);
    close(fd);
    return;
  }
  if (st.type != T_DIR) {
    close(fd);
    return;
  }

  // check path length
  if (strlen(path) + 1 + DIRSIZ + 1 > MAXPATH) {
    fprintf(2, "tree: path too long\n");
    close(fd);
    return;
  }

  // prepare path buffer
  strcpy(buf, path);
  p = buf + strlen(buf);
  *p++ = '/';

  // count total items first
  count = count_entries(fd, buf, p, onlyDir);
  close(fd);

  // reopen directory to print
  if ((fd = open(path, O_RDONLY)) < 0) {
    fprintf(2, "tree: cannot reopen %s\n", path);
    return;
  }

  p = buf + strlen(path);
  *p++ = '/';

  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0)
      continue;
    if (strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
      continue;

    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;

    if (stat(buf, &st2) < 0)
      continue;
    if (onlyDir && st2.type != T_DIR)
      continue;

    idx++;
    isLast = (idx == count);

    // print indentation
    for (i = 0; i < level - 1; i++) {
      printf("%s", prefix[i]);
    }

    // print connector and name
    if (isLast) {
      printf("`-- %s\n", p);
    } else {
      printf("|-- %s\n", p);
    }

    // recurse into subdirectory
    if (st2.type == T_DIR && level < maxDepth) {
      if (isLast) {
        strcpy(prefix[level - 1], "    ");
      } else {
        strcpy(prefix[level - 1], "|   ");
      }
      tree_r(buf, level + 1, maxDepth, onlyDir);
    }
  }

  close(fd);
}

int main(int argc, char *argv[])
{
  char *path = ".";
  int maxDepth = MAX_DEPTH;
  int onlyDir = 0;
  int i;

  // parse arguments
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-d") == 0) {
      onlyDir = 1;
    } else if (strcmp(argv[i], "-L") == 0) {
      if (i + 1 >= argc) {
        fprintf(2, "Usage: tree [path] [-L depth] [-d]\n");
        exit(1);
      }
      i++;
      maxDepth = atoi(argv[i]);
      if (maxDepth <= 0) {
        fprintf(2, "tree: invalid depth '%s'\n", argv[i]);
        exit(1);
      }
    } else {
      path = argv[i];
    }
  }

  // print root directory
  printf("%s\n", path);

  // init prefixes
  for (i = 0; i < MAX_DEPTH; i++) {
    strcpy(prefix[i], "    ");
  }

  // start traversal
  tree_r(path, 1, maxDepth, onlyDir);

  exit(0);
}

#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "user/user.h"

/*
 * tree – in cây thư mục cho xv6
 * Cú pháp: tree [path] [-L depth] [-d]
 *   path     : thư mục gốc (mặc định ".")
 *   -L depth : giới hạn độ sâu đệ quy
 *   -d       : chỉ in thư mục, bỏ qua file thường
 *
 * Output style (giống lệnh tree trên Linux):
 *   .
 *   |-- dir_a
 *   |   |-- file1
 *   |   `-- file2
 *   `-- dir_b
 *       `-- file3
 */

#define MAX_DEPTH 64
#define MAXPATH   512

/*
 * prefix[i] = chuỗi ký hiệu đứng trước entry ở level i+1.
 * Giá trị "|   " nếu level đó còn entry tiếp theo,
 * hoặc "    " nếu đó là entry cuối của level đó.
 * Mỗi chuỗi dài tối đa 4 ký tự + '\0' = 5 byte.
 */
static char prefix[MAX_DEPTH][5];

/*
 * Đếm số entry hợp lệ trong thư mục (đã mở với fd).
 * Hàm này đọc hết fd – gọi xong cần close và mở lại.
 */
static int
count_entries(int fd, char *buf, char *p, int onlyDir)
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

/*
 * tree_r: hàm đệ quy duyệt thư mục
 *   path     - đường dẫn thư mục cần duyệt
 *   level    - mức hiện tại (con trực tiếp của root = 1)
 *   maxDepth - số mức tối đa
 *   onlyDir  - nếu 1 thì chỉ in thư mục
 */
void
tree_r(char *path, int level, int maxDepth, int onlyDir)
{
  int fd;
  struct dirent de;
  struct stat st, st2;
  char buf[MAXPATH];
  char *p;
  int count, idx, isLast, i;

  if (level > maxDepth)
    return;

  /* Mở thư mục */
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

  /* Kiểm tra độ dài đường dẫn */
  if (strlen(path) + 1 + DIRSIZ + 1 > MAXPATH) {
    fprintf(2, "tree: path too long\n");
    close(fd);
    return;
  }

  /* Xây dựng buf = path + "/" (p trỏ tới phần tên entry) */
  strcpy(buf, path);
  p = buf + strlen(buf);
  *p++ = '/';

  /* Lần 1: đếm entry hợp lệ (xv6 không có lseek, phải đọc 2 lần) */
  count = count_entries(fd, buf, p, onlyDir);
  close(fd);

  /* Lần 2: mở lại và in từng entry */
  if ((fd = open(path, O_RDONLY)) < 0) {
    fprintf(2, "tree: cannot reopen %s\n", path);
    return;
  }

  /* Khôi phục lại p (buf có thể bị count_entries ghi đè) */
  p = buf + strlen(path);
  *p++ = '/';

  idx = 0;
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

    /* In tiền tố các level cha */
    for (i = 0; i < level - 1; i++)
      printf("%s", prefix[i]);

    /* In connector của entry hiện tại */
    if (isLast)
      printf("`-- ");
    else
      printf("|-- ");

    /* In tên entry (p đang trỏ đúng tên sau dấu '/') */
    printf("%s\n", p);

    /* Đệ quy vào thư mục con */
    if (st2.type == T_DIR && level < maxDepth) {
      if (isLast)
        strcpy(prefix[level - 1], "    ");
      else
        strcpy(prefix[level - 1], "|   ");
      tree_r(buf, level + 1, maxDepth, onlyDir);
    }
  }

  close(fd);
}

int
main(int argc, char *argv[])
{
  char *path = ".";
  int   maxDepth = MAX_DEPTH;
  int   onlyDir  = 0;
  int   i;

  /* Parse arguments */
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
      /* Coi là đường dẫn */
      path = argv[i];
    }
  }

  /* In root */
  printf("%s\n", path);

  /* Khởi tạo prefix (mặc định toàn khoảng trắng) */
  for (i = 0; i < MAX_DEPTH; i++)
    strcpy(prefix[i], "    ");

  /* Duyệt đệ quy bắt đầu từ level 1 */
  tree_r(path, 1, maxDepth, onlyDir);

  exit(0);
}

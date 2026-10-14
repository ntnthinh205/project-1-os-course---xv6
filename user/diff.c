#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

#define BUF_SIZE 512
#define MAX_LINE 512

struct line_reader {
  int fd;
  char buf[BUF_SIZE];
  int idx;
  int size;
  int eof;
};

static struct line_reader r1;
static struct line_reader r2;
static char line1[MAX_LINE];
static char line2[MAX_LINE];

static int get_char(struct line_reader *r)
{
  if (r->eof)
    return -1;

  if (r->idx >= r->size) {
    r->size = read(r->fd, r->buf, sizeof(r->buf));
    r->idx = 0;
    if (r->size <= 0) {
      r->eof = 1;
      return -1;
    }
  }

  return (uchar)r->buf[r->idx++];
}

static int readline(struct line_reader *r, char *buf, int max)
{
  int i = 0;
  int c;

  while (1) {
    c = get_char(r);
    if (c < 0) {
      if (i == 0)
        return 0;
      break;
    }

    if (c == '\n')
      break;

    if (c == '\r')
      continue;

    if (i < max - 1)
      buf[i++] = (char)c;
  }

  buf[i] = '\0';
  return 1;
}

int main(int argc, char *argv[])
{
  char *file1 = 0;
  char *file2 = 0;
  int quiet = 0;
  int fd1, fd2;
  struct stat st;
  int i;
  int line_num = 0;

  // parse argument 
  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-q") == 0) {
      quiet = 1;
    } else if (argv[i][0] == '-') {
      fprintf(2, "diff: unknown option %s\n", argv[i]);
      fprintf(2, "Usage: diff file1 file2 [-q]\n");
      exit(1);
    } else if (file1 == 0) {
      file1 = argv[i];
    } else if (file2 == 0) {
      file2 = argv[i];
    } else {
      fprintf(2, "Usage: diff file1 file2 [-q]\n");
      exit(1);
    }
  }

  if (file1 == 0 || file2 == 0) {
    fprintf(2, "Usage: diff file1 file2 [-q]\n");
    exit(1);
  }

  // Open file 1
  if ((fd1 = open(file1, O_RDONLY)) < 0) {
    fprintf(2, "diff: cannot open %s\n", file1);
    exit(1);
  }
  if (fstat(fd1, &st) < 0 || st.type == T_DIR) {
    fprintf(2, "diff: %s is a directory\n", file1);
    close(fd1);
    exit(1);
  }

  // Open file 2
  if ((fd2 = open(file2, O_RDONLY)) < 0) {
    fprintf(2, "diff: cannot open %s\n", file2);
    close(fd1);
    exit(1);
  }
  if (fstat(fd2, &st) < 0 || st.type == T_DIR) {
    fprintf(2, "diff: %s is a directory\n", file2);
    close(fd1);
    close(fd2);
    exit(1);
  }

  // Init readers
  memset(&r1, 0, sizeof(r1));
  memset(&r2, 0, sizeof(r2));
  r1.fd = fd1;
  r2.fd = fd2;

  // Compare files line by line
  while (1) {
    int has1 = readline(&r1, line1, sizeof(line1));
    int has2 = readline(&r2, line2, sizeof(line2));

    // Both files have reached EOF
    if (!has1 && !has2)
      break;

    line_num++;

    int differ = 0;
    if (has1 && has2) {
      if (strcmp(line1, line2) != 0)
        differ = 1;
    } else {
      differ = 1;
    }

    if (differ) {
      if (quiet) {
        printf("diff: files differ\n");
        break;
      }

      printf("%s:%d: < %s\n", file1, line_num, has1 ? line1 : "EOF");
      printf("%s:%d: > %s\n", file2, line_num, has2 ? line2 : "EOF");
    }
  }

  close(fd1);
  close(fd2);
  exit(0);
}

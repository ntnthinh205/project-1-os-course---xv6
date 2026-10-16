#include "kernel/types.h"
#include "user/user.h"

// atoi() cua xv6 khong bao loi (vd "-1" -> 0, "abc" -> 0),
// nen kiem tra chuoi chi gom chu so truoc, sau do moi dung atoi.
static int is_valid_ticks(const char *s) {
  int len = 0;
  if (*s == '\0')
    return 0;
  for (; *s; s++, len++) {
    if (*s < '0' || *s > '9')
      return 0;              // "-1", "abc", "1abc" bi tu choi
  }
  return len <= 9;           // tranh tran so khi atoi
}

int main(int argc, char *argv[]) {
  if (argc != 2) {           // thieu hoac thua doi so deu bao loi
    fprintf(2, "Usage: sleep <ticks>\n");
    exit(1);
  }

  if (!is_valid_ticks(argv[1])) {
    fprintf(2, "sleep: invalid ticks %s\n", argv[1]);
    exit(1);
  }

  int ticks = atoi(argv[1]);

  if (sleep(ticks) < 0) {
    fprintf(2, "sleep: failed\n");
    exit(1);
  }

  exit(0);
}
#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "user/user.h"

static int had_error = 0;   // co loi -> exit(1)

long du(char *path, int showFiles, int summary, int isRoot) {
    struct stat st;
    if (stat(path, &st) < 0) {
        fprintf(2, "du: cannot stat %s\n", path);
        had_error = 1;
        return 0;
    }

    if (st.type == T_FILE) {
        if ((showFiles && !summary) || isRoot) {
            printf("%d\t%s\n", (int)st.size, path);
        }
        return st.size;
    }

    if (st.type == T_DIR) {
        int fd = open(path, O_RDONLY);
        if (fd < 0) {
            fprintf(2, "du: cannot open %s\n", path);
            had_error = 1;
            return 0;
        }

        long total = 0;
        struct dirent de;
        char buf[512], *p;

        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
            fprintf(2, "du: path too long: %s\n", path);
            had_error = 1;
            close(fd);
            return 0;
        }

        strcpy(buf, path);
        p = buf + strlen(buf);
        if (p > buf && p[-1] != '/') {
            *p++ = '/';
        }
        *p = 0;

        while (read(fd, &de, sizeof(de)) == sizeof(de)) {
            if (de.inum == 0)
                continue;

            char name[DIRSIZ + 1];
            memset(name, 0, sizeof(name));
            memmove(name, de.name, DIRSIZ);

            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
                continue;

            strcpy(p, name);

            struct stat st2;
            if (stat(buf, &st2) < 0) {
                fprintf(2, "du: cannot stat %s\n", buf);
                had_error = 1;
                continue;
            }

            if (st2.type == T_DIR) {
                total += du(buf, showFiles, summary, 0);
            } else {
                total += st2.size;
                if (showFiles && !summary) {
                    printf("%d\t%s\n", (int)st2.size, buf);
                }
            }
        }
        close(fd);

        if (!summary || isRoot) {
            printf("%d\t%s\n", (int)total, path);
        }
        return total;
    }

    return 0;
}

int main(int argc, char *argv[]) {
    char *path = 0;
    int showFiles = 0;
    int summary = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-a") == 0) {
            showFiles = 1;
        } else if (strcmp(argv[i], "-s") == 0) {
            summary = 1;
        } else if (argv[i][0] == '-') {
            // tuy chon la, vd: -x
            fprintf(2, "du: unknown option %s\n", argv[i]);
            fprintf(2, "Usage: du [-a] [-s] [path]\n");
            exit(1);
        } else {
            if (path != 0) {
                // nhieu duong dan -> khong am tham dung duong dan cuoi
                fprintf(2, "du: too many paths\n");
                fprintf(2, "Usage: du [-a] [-s] [path]\n");
                exit(1);
            }
            path = argv[i];
        }
    }

    if (path == 0)
        path = ".";

    du(path, showFiles, summary, 1);

    exit(had_error ? 1 : 0);
}
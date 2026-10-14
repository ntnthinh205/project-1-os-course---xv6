#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "kernel/fs.h"
#include "user/user.h"

long du(char *path, int showFiles, int summary, int isRoot) {
    struct stat st;
    if (stat(path, &st) < 0) {
        fprintf(2, "du: cannot stat %s\n", path);
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
            return 0;
        }

        long total = 0;
        struct dirent de;
        char buf[512], *p;

        strcpy(buf, path);
        p = buf + strlen(buf);
        if (p > buf && p[-1] != '/') {
            *p++ = '/';
        }
        *p = 0; // Dam bao buf la chuoi hop le sau khi them dau gach cheo

        while (read(fd, &de, sizeof(de)) == sizeof(de)) {
            if (de.inum == 0)
                continue;
            
            char name[DIRSIZ + 1];
            memset(name, 0, sizeof(name));
            memmove(name, de.name, DIRSIZ);

            if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
                continue;

            // Build child path
            strcpy(p, name);

            struct stat st2;
            if (stat(buf, &st2) < 0) {
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
    char *path = ".";
    int showFiles = 0;
    int summary = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-a") == 0) {
            showFiles = 1;
        } else if (strcmp(argv[i], "-s") == 0) {
            summary = 1;
        } else {
            path = argv[i];
        }
    }

    du(path, showFiles, summary, 1);

    exit(0);
}

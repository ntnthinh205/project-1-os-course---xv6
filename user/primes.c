#include "kernel/types.h"
#include "user/user.h"

/*
 * Primes - thuật toán sàng số nguyên tố chạy song song sử dụng pipe và fork
 * In các số nguyên tố từ 2 đến 280.
 */

// Mỗi tiến trình sàng kết thúc bằng exit, không trả về hàm gọi.
void sieve(int rd) __attribute__((noreturn));

int main(int argc, char *argv[]) {
    int p[2];
    int i;

    if (pipe(p) < 0) {
        printf("primes: pipe failed\n");
        exit(1);
    }

    // fork tiến trình sàng trước khi ghi để luôn có tiến trình đọc pipe
    if (fork() == 0) {
        close(p[1]);
        sieve(p[0]);
    }

    close(p[0]);

    // Ghi các số từ 2 đến 280 vào pipe
    for (i = 2; i <= 280; i++) {
        write(p[1], &i, 4);
    }
    close(p[1]);            // đóng đầu ghi để báo hết dữ liệu

    wait((int*)0);          // chờ toàn bộ pipeline kết thúc
    exit(0);
}

void sieve(int rd) {
    int prime, num;
    int pw[2];
    int has_child = 0;

    // Số đầu tiên đọc được luôn là số nguyên tố
    if (read(rd, &prime, 4) != 4) {
        close(rd);
        exit(0);
    }
    printf("prime %d\n", prime);

    // Lọc bỏ bội số của prime, chuyển phần còn lại sang tiến trình kế tiếp
    while (read(rd, &num, 4) == 4) {
        if (num % prime != 0) {
            if (!has_child) {
                if (pipe(pw) < 0) {
                    printf("primes: pipe failed\n");
                    exit(1);
                }
                if (fork() == 0) {
                    close(rd);
                    close(pw[1]);
                    sieve(pw[0]);
                }
                close(pw[0]);
                has_child = 1;
            }
            write(pw[1], &num, 4);
        }
    }
    close(rd);

    if (has_child) {
        close(pw[1]);
        wait((int*)0);              // chờ tiến trình con kết thúc
    }
    exit(0);
}
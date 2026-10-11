#include "kernel/types.h"
#include "user/user.h"

/*
 * Primes - concurrent prime sieve using pipes and fork
 */

void sieve(int rd);

int main(int argc, char *argv[]) {
    int p[2];
    int i;
    
    if (pipe(p) < 0) {
        printf("primes: pipe failed\n");
        exit(1);
    }
    
    // Write numbers 2-35 to pipe
    for (i = 2; i < 35; i++) {
        write(p[1], &i, 4);
    }
    close(p[1]);  // close write end in parent
    
    if (fork() == 0) {
        // child: sieve
        close(p[1]);
        sieve(p[0]);
    }
    
    close(p[0]);
    wait((int*)0);
    exit(0);
}

void sieve(int rd) {
    int prime;
    int pw[2];
    int num;
    
    if (read(rd, &prime, 4) != 4) {
        // no more data
        close(rd);
        exit(0);
    }
    
    printf("prime %d\n", prime);
    
    if (pipe(pw) < 0) {
        printf("primes: pipe failed\n");
        exit(1);
    }
    
    // fork to handle next prime
    if (fork() == 0) {
        // child: continue sieving with the new pipe
        close(rd);
        close(pw[1]);
        sieve(pw[0]);
        exit(0);
    }
    
    // parent: read from rd, filter multiples of prime, write to pw[1]
    close(pw[0]);
    while (read(rd, &num, 4) == 4) {
        if (num % prime != 0) {
            write(pw[1], &num, 4);
        }
    }
    close(rd);
    close(pw[1]);
    
    // wait for child to finish
    wait((int*)0);
    
    exit(0);
}

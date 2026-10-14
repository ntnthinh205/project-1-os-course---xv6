#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int fd_src, fd_dst, n;
  char buf[512];

  //Kiem tra so luong tham so :
  if(argc != 3){
    printf("usage: cp src dst\n");
    exit(1);
  }

  //Mo file nguoin chuong trinh. Neu khong mo dc -> cp: cannot open <ten chuong trinh> và dung chuong trinh
  if((fd_src = open(argv[1], O_RDONLY)) < 0){
    printf("cp: cannot open %s\n", argv[1]);
    exit(1);
  }

  //Tao file dst ( Neu co roi thi mo file nay len), 
  if((fd_dst = open(argv[2], O_WRONLY | O_CREATE | O_TRUNC)) < 0){
    printf("cp: cannot create %s\n", argv[2]);
    close(fd_src);
    exit(1);
  }

  //oc tu src va ghi sang dst theo block 512 bytes
  while((n = read(fd_src, buf, sizeof(buf))) > 0){
    if(write(fd_dst, buf, n) != n){
      printf("cp: write error\n");
      close(fd_src);
      close(fd_dst);
      exit(1);
    }
  }

  if(n < 0){
    printf("cp: read error\n");
  }

  //Dong file va ket thuc
  close(fd_src);
  close(fd_dst);
  exit(0);
}
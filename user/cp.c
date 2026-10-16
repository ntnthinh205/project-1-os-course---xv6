#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/fcntl.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int fd_src, fd_dst, n;
  char buf[512];
  struct stat st_src, st_dst;

  // Kiểm tra số lượng tham số
  if(argc != 3){
    printf("usage: cp src dst\n");
    exit(1);
  }

  // Mở file nguồn
  if((fd_src = open(argv[1], O_RDONLY)) < 0){
    printf("cp: cannot open %s\n", argv[1]);
    exit(1);
  }

  // Lấy thông tin stat của file nguồn
  if(fstat(fd_src, &st_src) < 0){
    printf("cp: cannot stat %s\n", argv[1]);
    close(fd_src);
    exit(1);
  }

  // Tạo/Mở file đích
  if((fd_dst = open(argv[2], O_WRONLY | O_CREATE | O_TRUNC)) < 0){
    printf("cp: cannot create %s\n", argv[2]);
    close(fd_src);
    exit(1);
  }

  // Lấy thông tin stat của file đích để kiểm tra có trùng file/inode với nguồn không
  if(fstat(fd_dst, &st_dst) < 0){
    printf("cp: cannot stat %s\n", argv[2]);
    close(fd_src);
    close(fd_dst);
    exit(1);
  }

  // Nếu src và dst là cùng một file (cùng device và cùng inode)
  if(st_src.dev == st_dst.dev && st_src.ino == st_dst.ino){
    printf("cp: cannot copy file to itself\n");
    close(fd_src);
    close(fd_dst);
    exit(1);
  }

  // Đọc từ src và ghi sang dst theo block 512 bytes
  while((n = read(fd_src, buf, sizeof(buf))) > 0){
    if(write(fd_dst, buf, n) != n){
      printf("cp: write error\n");
      close(fd_src);
      close(fd_dst);
      exit(1);
    }
  }

  // Sửa lỗi: Nếu read trả về lỗi (n < 0) -> exit(1)
  if(n < 0){
    printf("cp: read error\n");
    close(fd_src);
    close(fd_dst);
    exit(1);
  }

  // Đóng file và kết thúc thành công
  close(fd_src);
  close(fd_dst);
  exit(0);
}
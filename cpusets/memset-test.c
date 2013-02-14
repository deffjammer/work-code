#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>



#define PATH_MAX 200

int main(int argc, char  **argv)
{

  char buf[PATH_MAX];
  char buf1[16];
  char *bp;
  int fd = -1;
  int cpu = -1;
  char flagp[16];
  int pid = 0;


  if ((fd = open("./dev/cpuset/sched_load", O_RDONLY)) < 0)
      goto err;
  if (read(fd, buf1, sizeof(buf)) < 1)
      goto err;
  if (atoi(buf1))
      *flagp = 1;
  else
      *flagp = 0;
  close(fd);

  memset(buf, '\0', sizeof(buf));

  if (pid == 0)
    snprintf(buf, sizeof(buf), "/proc/self/stat");
  else
    snprintf(buf, sizeof(buf), "/proc/%d/stat", pid);

  if ((fd = open(buf, O_RDONLY)) < 0)
    goto err;
  if (read(fd, buf, sizeof(buf)) < 1)
    goto err;
  close(fd);

  bp = strrchr(buf, ')');
  if (bp)
    sscanf(bp + 1, "%*s %*u %*u %*u %*u %*u %*u %*u "
                    "%*u %*u %*u %*u %*u %*u %*u %*u %*u %*u "
                    "%*u %*u %*u %*u %*u %*u %*u %*u %*u %*u "
	   "%*u %*u %*u %*u %*u %*u %*u %*u %u", /* 37th field past ')' */
	   &cpu);
  if (cpu < 0)
    errno = EINVAL;
  return cpu;
 err:
  if (fd >= 0)
    close(fd);

 return -1;


}

#include <stdio.h>
#include <react.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
             #include <dirent.h>
              #include <pwd.h>
              #include <grp.h>
              #include <time.h>
              #include <locale.h>
              #include <langinfo.h>
              #include <stdio.h>
              #include <stdint.h>
              #include <errno.h>

void bin_prnt_byte(int x)
{
  int n;
  for(n=0; n<8; n++)
    {
      if((x & 0x80) !=0)
	{
	  printf("1");
	}
      else
	{
	  printf("0");
	}
      if (n==3)
	{
	  printf(" "); /* insert a space between nybbles */
	}
      x = x<<1;
    }
}

void bin_prnt_int(int x)
{
  int hi, lo;
  hi=(x>>8) & 0xff;
  lo=x&0xff;
  bin_prnt_byte(hi);
  printf(" ");
  bin_prnt_byte(lo);
}

gid_t get_group(const char *dev)
{
   struct stat statbuf;
   char dev_path[100];

   sprintf(dev_path, "/dev/%s",dev);

   if (stat(dev_path, &statbuf) < 0){
     printf("stat %s not loaded\n",dev_path);
     perror("stat");
     return -1;
   }

   printf("get group %s %d\n",dev_path,statbuf.st_gid);

   return statbuf.st_gid;

}
mode_t get_mode(const char *dev)
{
   struct stat statbuf;
   char dev_path[100];
   int rc;

   sprintf(dev_path, "/dev/%s",dev);

   if (stat(dev_path, &statbuf) <0){
     printf(" stat %s not loaded ",dev_path);
     //perror("stat");
     return -1; //NEED original mode
   }
   //printf("get mode %s 0%o\n",dev_path,statbuf.st_mode);

   return statbuf.st_mode;
}

mode_t pick_mode(mode_t mode, char *dev, mode_t file_mode)
{
  mode_t tmp;

  tmp = (mode == -1) ? get_mode(dev) : mode;
  
  //printf("- >pick_mode tmp %d , file_mode %o\n",tmp,file_mode);

  return (tmp == -1) ? file_mode : tmp;

}
gid_t pick_group(gid_t group, char *dev, gid_t file_group)
{
  gid_t tmp;

  tmp = (group == -1) ? get_group(dev) : group;

  printf("- >pick_group tmp %d , file_group %o\n",tmp,file_group);
  return (tmp == -1) ? file_group :  tmp;
   
}


#define WF 1
#define NO 2
int main(int argc, char **argv) {
  int verbose =1;
  gid_t mg = -1;
  mode_t mm = -1;
  int dd=100644;
  int width =2;
  char str[10],str2[10];
  char *p_str;
  int length=0;
  char mms[10];
  unsigned long x,mask=0, new_mask=0;
  mode_t new_mode;
  struct stat statbuf;
  int         status;
  struct passwd  *pwd;
  struct group   *grp;
  struct tm      *tm;
  int i_mode;

  printf(" mode octal %o\n",mm);
  printf(" mode dec  %d\n",mm);
  printf(" group dec  %d\n",mg);
  
#if 0
  //Set bit to 1 and leave all other unset
  //x |= mask;
  if(stat("/dev/cpuset/boot/tasks", &statbuf)<0)
    return -1;

  printf("read mode %o\n", statbuf.st_mode);

  mm = statbuf.st_mode;
  printf("orig mode %o\n",mm);

  sprintf(str, "%o", mm);
  p_str = &str;
  printf("string %s p %s\n",str,p_str);
  length = strlen(str) - 4;
  p_str+=length;
  printf("p adjusted %s \n",p_str);
  strcpy(mms,p_str);
  printf("mms %s\n",mms);
  return 1;
#endif
#if 0
  mask |= RT_NO_WAIT;
  mask |= WF;

  printf("RT_NO_WAIT & mask %d\n",RT_NO_WAIT & mask);
  printf("RT_WAIT & mask %d\n",RT_WAIT & mask);
  printf("WF & mask %d\n",WF & mask);

  if (cpu_sysrt_perm(mg /*group*/, mm/*mode*/,  mask) < 0)
  //  if (cpu_sysrt_perm(0 /*group*/, 0 /*mode*/,  RT_WAIT) < 0)
    printf("Perm error\n");

#endif

   FILE *udev_fd;
   char udevbuf[1200]={'\0'};

   if ((udev_fd = fopen("./sgi-udev-rule", "w")) < 0){
     perror("open /etc/sysconfig/sgi-react.conf failed");
   }
   //get all group info individually if if -1 passed.
   printf("kbar group %d\n",pick_group(mg, "kbar",111));
   printf("uli group %d\n",pick_group(mg, "uli",111));
   return;
   printf("kbar mode  %o group %d\n",pick_mode(mm, "kbar",0777),   pick_group(mg, "kbar",111));
   printf("uli  mode  %o group %d\n",pick_mode(mm, "uli",0777),   pick_group(mg, "uli",111));
   printf("frs  mode  %o group %d\n",pick_mode(mm, "frs",0777),    pick_group(mg, "frs",111));
   printf("sgi-shield %o group %d\n",pick_mode(mm, "sgi-shield",0777),   pick_group(mg, "sgi-shield",111));


   //fprintf(udev_fd,"%o\n",mm);
   fprintf(udev_fd,
	  "KERNEL==\"frs\", MODE=\"0%o\", GROUP=\"%d\"\n"
	  "KERNEL==\"uli\", MODE=\"0%o\", GROUP=\"%d\"\n"
	  "KERNEL==\"kbar\", MODE=\"0%o\", GROUP=\"%d\"\n"
	  "KERNEL==\"sgi-shield\", MODE=\"0%o\", GROUP=\"%d\"\n"
	  "KERNEL==\"extint*\", SUBSYSTEM==\"extint\", MODE=\"0%o\", GROUP=\"%d\", RUN+=\"/bin/chown :%d /sys/class/extint/%k/mode\"\n"
	  "KERNEL==\"extint*\", SUBSYSTEM==\"extint\", RUN+=\"/bin/chmod 0%o /sys/class/extint/%k/mode\"\n"
	  "KERNEL==\"extint*\", SUBSYSTEM==\"extint\", RUN+=\"/bin/chown :%d /sys/class/extint/%k/source\"\n"
	  "KERNEL==\"extint*\", SUBSYSTEM==\"extint\", RUN+=\"/bin/chmod 0%o /sys/class/extint/%k/source\"\n"
	  "KERNEL==\"extint*\", SUBSYSTEM==\"extint\", RUN+=\"/bin/chown :%d /sys/class/extint/%k/period\"\n"
	  "KERNEL==\"extint*\", SUBSYSTEM==\"extint\", RUN+=\"/bin/chmod 0%o /sys/class/extint/%k/period\"\n"
	  "KERNEL==\"intout*\", SUBSYSTEM==\"ioc4_intout\", MODE=\"0%o\", GROUP=\"%i\"\n"
	   ,pick_mode(mm, "frs",0777), mg,
	  pick_mode(mm, "uli",0777), mg,  
	  pick_mode(mm, "kbar",0777), mg,
	  pick_mode(mm, "sgi-shield",0777), mg,
	  pick_mode(mm, "extint",0777), mg, mg,
	  pick_mode(mm, "extint",0777),
	  mg,
	  pick_mode(mm, "extint",0777),
	  mg,
	  pick_mode(mm, "extint",0777),
	  pick_mode(mm, "ioc4_intout",0777), mg);

   //printf("%s",udevbuf);   
  //  write(udev_fd,udevbuf,strlen(udevbuf));

  fclose(udev_fd); 

}

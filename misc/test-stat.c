#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#define is_mode_get(s, is_group) ((is_group) ? s.st_gid : s.st_mode)

#define t_g(group, f_group, force_file) ((force_file) ? f_group : group)

/*                                                                                                                  
 * Try to find extension number of extint/intout                                                                    
 * if nothing found, defaults to 0.                                                                                 
 */
int find_dev_num(char *dev)
{
  int i=0;
  struct stat statbuf;
  char dev_path[50];

  while (i <= 255) {
    sprintf(dev_path, "%s%d", dev,i);
    if (stat(dev_path, &statbuf) == 0){
      //printf("Found %s\n",dev_path);                                                                              
      return i;
    }
    i++;
  }
  return 0;
}

typedef struct g_and_m_ret {
  mode_t m;
  gid_t  g;
}g_and_m_ret_t;

struct stat get_m_and_g(char *dev)
//g_and_m_ret_t get_m_and_g(char *dev)                                                                              
{
  struct stat statbuf_g;
  g_and_m_ret_t u_gm;

  printf("Now stat %s\n",dev);
  if (stat(dev, &statbuf_g) <0)
    printf("NOT FOUND\n");

  printf("Mode 0%o\n",statbuf_g.st_mode);
  printf("Group %d\n",statbuf_g.st_gid);

  u_gm.m = statbuf_g.st_mode;
  u_gm.g = statbuf_g.st_gid;

  return statbuf_g;
  //return u_gm;                                                                                                    
}

int main(int argc, char **argv)
{


  char dev_path[50];
  char *pdev = "extint";
  mode_t mode = 0666;
  mode_t f_mode = 0777;
  //g_and_m_ret_t g_and_m;                                                                                          
  struct stat g_and_m;

  sprintf(dev_path, "/dev/%s",pdev);
  printf("%s\n",dev_path);

  if (((strncmp(dev_path, "/dev/extint", strlen("/dev/extint"))) == 0 ) ||
      ((strncmp(dev_path, "/dev/intout", strlen("/dev/intout"))) == 0)) {
    char tmp_dev[50];
    int dev_num = find_dev_num(dev_path);
    sprintf(tmp_dev, "%s%i",dev_path, dev_num);
    strncpy(dev_path, tmp_dev, strlen(tmp_dev));

  }

  //  sprintf(dev_path, "/dev/%s",dev_path);                                                                        
  g_and_m = get_m_and_g(dev_path);
  printf("Mode 0%o Group %d\n",g_and_m.st_mode, g_and_m.st_gid);
  printf("Mode 0%o Group %d\n", is_mode_get(g_and_m,1), is_mode_get(g_and_m,0));

  return 0;
}




#include <regex.h>
#include <sys/file.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{

  FILE*           infoFile;       /* Int file pointer */
  int             status;
  regex_t         dreg,*pdreg=&dreg;
  regmatch_t      drmatch[16];
  char            *pattern  ="^cpu[[:space:]]*cores[[:space:]]*:[[:space:]]*([[:digit:]]*)",
		   c_count_str[16]="\0";
  char            line[1024] = {'\0'};
  int core_count;

  if ((infoFile = fopen("/proc/cpuinfo", "r")) == NULL){
    fprintf(stderr,"open /proc/cpuinfo failed\n");
    return -1;
  }


  if (regcomp(pdreg, pattern, REG_EXTENDED|REG_ICASE) != 0) {
    fprintf(stderr,"regcomp failed\n");
    return(0);      /* Report error. */
  }


  while( fgets(line,sizeof(line),infoFile) ) {

	if( regexec(pdreg, line, 16-1 , drmatch, 0))
	  continue;

	printf("length %d, %s\n",strlen(&line[drmatch[1].rm_so]),&line[drmatch[1].rm_so]);
	/* Might need to increase to 2, if we get above 8 cores per cpu */
	//strncpy(core_count,&line[drmatch[1].rm_so] + (drmatch[0].rm_eo - drmatch[0].rm_so), 1);
	strncpy(c_count_str,&line[drmatch[1].rm_so], drmatch[1].rm_eo - drmatch[1].rm_so + 1);
	printf("integer val cores %d\n",atoi(c_count_str));
	break;
  }

  regfree(pdreg);
  printf("core_count %s\n", c_count_str);

  if (status != 0) {
    return(0);      /* Report error. */
  }

  return(1);
}


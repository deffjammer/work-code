/* ######################################################
 * #
 * # elilo-adjust.c 
 * #
 * # Program to adjust command line in a elilo.conf file.
 * # Goal is to store command line so we can tweak values.
 * #
 * # Auther: Derek Fults
 * #
 * ######################################################
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/file.h>
#include <regex.h>

#define COMMAND_LINE_SIZE 256
char *const react_cmdline  = "idle=halt nohz=off highres=on rcutree.blimit=3 nmi_watchdog=0 noirqdebug cgroups_disable=memory init=/sbin/react-init.sh isolcpus";


char *escape_it(char *str) {
        static  char    res[BUFSIZ]="\0";
        char            *pres=NULL,
                        *pstr=NULL,
                        wildcard[]=".*";
        int             i=0;

        if( !str )
                return NULL;

        for(pstr=str, pres=&res[0] ; *pstr ;i++,  pstr++, pres++) {
                if( index(wildcard,*pstr) )
                        *pres++ = '\\';
                *pres=*pstr;
        }
        *pres='\0';
        return &res[0];
}

int react_elilo_adjust(const char *conf_file, int add_to_file)
{
        FILE            *pfile;       /* Int file pointer */
        FILE            *tmp = tmpfile();
        char            line[COMMAND_LINE_SIZE] = {'\0'};
        char            default_label[256] = {'\0'};

        regex_t         dreg_default,*pdreg_default=&dreg_default,
                        dreg_image,*pdreg_image=&dreg_image,
                        dreg_label,*pdreg_label=&dreg_label,
                        dreg_append,*pdreg_append=&dreg_append;
        regmatch_t      drmatch[16];

        char            *rdefault="^[[:space:]]*default[[:space:]]*=[[:space:]]*([-_\\.[:alnum:]]*)[[:space:]]*",
                        *flabel  ="^[[:space:]]*label[[:space:]]*=[[:space:]]*%s[[:space:]]*$",
                        *fappend ="^[[:space:]]*append[[:space:]]*=",
                        *fimage  ="^[[:space:]]*image[[:space:]]*=",
                        label[BUFSIZ]="\0",
                        rlabel[BUFSIZ]="\0";
        int             label_found=0, append_found=0, added=0, done=0;
        long            pfile_append_pos=0, pfile_pos;


        //char *const elilocmd[] = {
        //  "/sbin/elilo", "/bin/elilo", "/usr/sbin/elilo", NULL };

        //char *const run_elilo[] = {
        //  "elilo", NULL};


        /* Open the config file */
        if((pfile = fopen(conf_file, "r")) == NULL){
                fprintf(stderr,"open elilo.conf failed\n");
                return -1;
        }

        /* Find default label and reset the file position indicator. */
        if( regcomp(pdreg_default, rdefault, REG_EXTENDED | REG_ICASE ) != 0 )
                return -1;

        while( fgets(line,sizeof(line),pfile) ) {
                if( regexec(pdreg_default, line, 16-1 , drmatch, 0) == REG_NOMATCH
                    || drmatch[1].rm_so == drmatch[1].rm_eo )
                        continue;

                strncpy(default_label,&line[drmatch[1].rm_so], drmatch[1].rm_eo - drmatch[1].rm_so);
                snprintf(rlabel,sizeof(label),flabel,escape_it(default_label));
                break;
        }

        rewind(pfile);
        if( !*default_label ) {
                fprintf(stderr,"Default label not found. Exiting\n");
                return -1;
        }
        printf("Default label = '%s'\n" ,default_label);

        /*  Search label=<label */
        printf("Searching for : %s\n",rlabel);
        regfree(pdreg_default);
        if( regcomp(pdreg_label, rlabel, REG_EXTENDED ) != 0 )
                return -1;

        /* Put file contents into a tmp file */
        while ((fgets(line, COMMAND_LINE_SIZE, pfile))!=NULL) {
                fputs(line, tmp);
        }

        fclose(pfile);
        rewind(tmp);

        /* Reopen file now with write permissions */
        if((pfile = fopen(conf_file, "w")) == NULL){
                fprintf(stderr,"open elilo.conf failed\n");
                return -1;
        }

        if( regcomp(pdreg_append, fappend, REG_EXTENDED | REG_ICASE) != 0 )
                return -1;

        if( regcomp(pdreg_image, fimage, REG_EXTENDED | REG_ICASE) != 0 )
                return -1;

        /* Add react stuff to the default label */
        while(1){
          pfile_pos = ftell(tmp);
          if ((fgets(line, COMMAND_LINE_SIZE, tmp))== NULL)
                break;

          if (!done && !added) {
                /* Find image= which denotes a new section */
                if( regexec(pdreg_image, line, 0 , NULL, 0) != REG_NOMATCH ) {
                        printf("Section image : %s",line);
                        append_found = 0;
                        label_found  = 0;
                }
                /* Find section that matches the default label */
                if( regexec(pdreg_label, line, 0 , NULL, 0) != REG_NOMATCH ) {
                        label_found = 1;
                        if(append_found){
                                /* Reset tmp file back to append line */
                                fseek(tmp, pfile_append_pos, SEEK_SET);
                                /*
                                 * We already put the append line back to file,
                                 * reset pfile pos as well.
                                 */
                                fseek(pfile, pfile_append_pos, SEEK_SET);
                                if ((fgets(line, COMMAND_LINE_SIZE, tmp)) != NULL) {
                                        printf("Found matching label : %s",line);
                                        //added = add_react_to_append(line, add_to_file);
                                }
                                done = 1;
                                goto added;
                        }
                }

                /* Find append= and keep its position to edit if we match default label */
                if( regexec(pdreg_append, line, 0 , NULL, 0) != REG_NOMATCH ) {
                        pfile_append_pos = pfile_pos;
                        append_found = 1;
                        if(label_found){
                                printf("Found append : %s",line);
                                added = //add_react_to_append(line, add_to_file);
                                done = 1;
                                goto added;
                        }
                }
          }/* if !added */
        added:
          /* Put line back to file */
          fputs(line, pfile);
        } /* while 1 */
        fclose(pfile);
        fclose(tmp);

        regfree(pdreg_append);
        regfree(pdreg_image);
        regfree(pdreg_label);

        //if (added == 1)
        //        fork_cmd(run_elilo, elilocmd);
        /*else*/ 
	if (added == 0) {
                fprintf(stderr,"REACT not added.\n");
                return -1;
        } /* (added < 0)  Not error, but didn't add, dont' run elilo */

        return 0;
}
int new_func(const char *conf_file)
{
	//struct t1 {
	//	char * keyword;
	//	char * value;
	//struct t t1[100];
	//t1[0].keyword = "isolcpus"}
	//t1[0].keyword = "isolpus="
	//t1[0].value   = "1-16"

	//Check optarg for command line options
	   //Read in arg string
	//Check react config file for options (get string.)
	  //Parse Args - Should take string from optargs
	  //if '=' is in the string split into keyword/value?

	//Open File
	//Find default label
	  //Find append line
	  //Copy in append line as we go. (keep append and label count)

	return 0;
}

int main (int argc, char *argv[])
{
	//"idle=halt nohz=off highres=on rcutree.blimit=3 nmi_watchdog=0 noirqdebug cgroups_disable=memory init=/sbin/react-init.sh isolcpus"
	int c;

	while ((c = getopt(argc, argv, "hedvsr:m:i:t:p:w:c:I:")) != -1) {
                switch (c) {
                case 'h':
                        printf("help\n");
                        exit(EXIT_SUCCESS);
                        break;
                case 'w':
			printf("optargs %s\n",optarg);
			break;
         	default:
                        printf("default %c %s\n",c, optarg);
                        exit(EXIT_FAILURE);
                        break;
                }
        }
	//react_elilo_adjust("./elilo.conf", 0 /*int add_to_file*/ );
	new_func("./elilo.conf");

	return 0;
}

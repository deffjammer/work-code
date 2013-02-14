#include <sys/types.h>
#include <regex.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

char *
escape_it(char *str) {
	static	char	res[BUFSIZ]="\0";
	char		*pres=NULL,
			*pstr=NULL,
			wildcard[]=".*";
	int		i=0;

	if( !str ) return NULL;
	for(pstr=str, pres=&res[0] ; *pstr ;i++,  pstr++, pres++)  {
		if( index(wildcard,*pstr) ) 
			*pres++ = '\\';
		*pres=*pstr;
	}
	*pres='\0';
	return &res[0];
}
int
main(int argc, char *argv[]) {
	FILE	*fp=fopen("r.conf","r");
	char	line[BUFSIZ];
	int	lineno=0,
		dlineno=0;

	regex_t		dreg,*pdreg=&dreg;
	regmatch_t	drmatch[16];	// Should be enough

	char		*rdefault="[[:space:]]*default[[:space:]]*=[[:space:]]*([-_\\.[:alnum:]]*)[[:space:]]*",
			*flabel  ="[[:space:]]*label[[:space:]]*=[[:space:]]*%s[[:space:]]*$",
			label[BUFSIZ]="\0",
			rlabel[BUFSIZ]="\0";

	// Search default label
	fprintf(stderr,"Searching for : %s\n",rdefault);
	if( regcomp(pdreg, rdefault, REG_EXTENDED | REG_ICASE) != 0 ) 
		exit(1);
	while( fgets(line,sizeof(line),fp) ) {
		char	*cr=strrchr(line,'\n');

		lineno++;
		if( cr ) *cr='\0';
		if( regexec(pdreg, line, 16-1 , drmatch, 0) == REG_NOMATCH 
		    || drmatch[1].rm_so == drmatch[1].rm_eo )
			continue;
		dlineno=lineno;
		strncpy(label,&line[drmatch[1].rm_so], drmatch[1].rm_eo - drmatch[1].rm_so);
		snprintf(rlabel,sizeof(label),flabel,escape_it(label));
		break;
	}
	rewind(fp);
	if( !*label ) { 
		fprintf(stderr,"Default label not found. Exiting\n");
		exit(1);
	}
	fprintf(stderr,"%d Default label = '%s'\n" ,dlineno,label);
	fprintf(stderr,"%d Default rlabel = '%s'\n",dlineno,rlabel);

	// Search label=<label>
	fprintf(stderr,"Searching for : %s\n",rlabel);
	regfree(pdreg); lineno=0;
	if( regcomp(pdreg, rlabel, REG_EXTENDED | REG_ICASE) != 0 ) 
		exit(1);
	while( fgets(line,sizeof(line),fp) ) {
		char	*cr=strrchr(line,'\n');

		lineno++;
		if( cr ) *cr='\0';
		if( regexec(pdreg, line, 0, NULL, 0) == REG_NOMATCH ) continue;
		fprintf(stderr,"%d found matching image here: %s\n",lineno,line);
		break;
	}

}

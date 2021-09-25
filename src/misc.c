#include "globals.h"


/*** See if the string contains a valid number ***/
int numType(char *str)
{
	char *s;
	char *s2;
	char *e;
	char *e2;
	int dot;
	int digit;

	s = str;
	e=str+strlen(str)-1;

	if (*s == '-' && ++s > e) return NOT_NUM;

	/* See if its a bin, oct or hex value */
	if ((int)(e-s) > 0 && *s == '0')
	{
		s2 = s+1;
		if (*s2 == 'b' || *s2 == 'B')
		{
			strtoll(s2+1,&e2,2);
			return (e2 != e+1 || errno == EINVAL) ? NOT_NUM : NUM_BIN;
		}
		if (*s2 == 'x' || *s2 == 'X')
		{
			strtoll(s,&e2,16);
			return (e2 != e+1 || errno == EINVAL) ? NOT_NUM : NUM_HEX;
		}
		if (*s2 != '.')
		{
			strtoll(s,&e2,8);

			/* Don't return if not oct. Could still be a decimal.
			   eg. 088 */
			if (e2 == e+1 && errno != EINVAL) return NUM_OCT;
		}
	}
	
	for(dot=digit=0;*s && s<= e;++s)
	{
		if (*s == '.')
		{
			/* Can only have 1 decimal point */
			if (dot) return NOT_NUM;
			dot = 1;
		}
		else if (isdigit(*s)) digit = 1;
		else return NOT_NUM;
	}
	return digit ? (dot ? NUM_FLOAT : NUM_INT) : NOT_NUM;
}




void toUpperStr(char *str)
{
	char *s;
	for(s=str;*s;++s) *s = toupper(*s);
}




void toLowerStr(char *str)
{
	char *s;
	for(s=str;*s;++s) *s = tolower(*s);
}




void doError(int err, st_progline *progline)
{
	if (progline && progline->linenum)
	{
		printf("ERROR %d: %s on line %d",
			err,error_str[err],progline->linenum);
	}
	else printf("ERROR %d: %s",err,error_str[err]);

	if (errno) printf(": %s",strerror(errno));
	putchar('\n');
}




/*** Returns TRUE if the string matches the pattern, else FALSE. Supports 
     wildcard patterns containing '*' and '?' ***/
bool wildMatch(char *str, char *pat, bool case_sensitive)
{
	char *s,*p,*s2;

	for(s=str,p=pat;*s && *p;++s,++p)
	{
		switch(*p)
		{
		case '?':
			continue;

		case '*':
			if (!*(p+1)) return TRUE;

			for(s2=s;*s2;++s2)
			{
				if (wildMatch(s2,p+1,case_sensitive))
					return TRUE;
			}
			return FALSE;
		}
		if (case_sensitive)
		{
			if (*s != *p) return FALSE;
		}
		else if (toupper(*s) != toupper(*p)) return FALSE;
	}

	/* Could have '*' leftover in the pattern which can match nothing.
	   eg: "abc*" should match "abc" and "*" should match "" */
	if (!*s)
	{
		/* Could have *'s on the end which should all match "" */
		for(;*p && *p == '*';++p);
		if (!*p) return TRUE;
	}

	return FALSE;
}




char *addFileExtension(char *filename)
{
	char *tmp = NULL;
	int len = strlen(filename);

	if (len < FILE_EXT_LEN || strcmp(filename+len-FILE_EXT_LEN,FILE_EXT))
	{
		assert((tmp = (char *)malloc(len + FILE_EXT_LEN+1)));
		strcpy(tmp,filename);
		strcat(tmp,FILE_EXT);
	}
	return tmp;
}




double getCurrentTime()
{
	struct timeval tv;

	gettimeofday(&tv,NULL);
	return tv.tv_sec + (double)tv.tv_usec / 1000000;
}




void printTrace(int linenum, char *type, char *name)
{
	if (tracing_mode == TRACING_OFF) return;

	if (child_process)
		printf("[%u,%d,%s,%s]",getpid(),linenum,type,name);
	else
		printf("[%d,%s,%s]",linenum,type,name);
	if (tracing_mode == TRACING_STEP)
	{
		printf(": ");
		fflush(stdout);
		pressAnyKey(NULL);
	}
	putchar('\n');	
}




void ready()
{
	puts("READY");
}




void prompt()
{
	/* Turned off by EDIT command */
	if (draw_prompt)
		PRINT("] ",2);
	else
		draw_prompt = TRUE;
}




/*** Print a message then wait for a key to be pressed ***/
char pressAnyKey(char *msg)
{
	fd_set mask;
	int len;
	int i;
	char c;

	/* Print message */
	if (msg)
	{
		len = strlen(msg);
		fflush(stdout);
		PRINT(msg,len);
	}

	/* Use select() so we get control-C interrupts */
	FD_ZERO(&mask);
	FD_SET(STDIN,&mask);
	if (select(FD_SETSIZE,&mask,0,0,0) > 0)
		read(STDIN,&c,1);  /* Remove from STDIN */
	else
		c = ' ';

	/* Erase message */
	if (msg)
	{
		putchar('\r');
		for(i=0;i < len;++i) putchar(' ');
		putchar('\r');
	}

	return c;
}




void doExit(int code)
{
	if (!child_process)
	{
		saneMode();
		printf("*** EXIT with code %d ***\n",code);
	}
	exit(code);
}

#include "globals.h"

int qsortCompare(const void *p1, const void *p2)
{                       
	/* p1 and p2 are actually point to (char **), not (char *) */
	return strcmp(*(char **)p1, *(char **)p2);
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




double getCurrentTime(void)
{
	struct timeval tv;

	gettimeofday(&tv,NULL);
	return tv.tv_sec + (double)tv.tv_usec / 1000000;
}




void printTrace(int linenum, char *type, char *name)
{
	if (tracing_mode == TRACING_OFF) return;

	if (flags.child_process)
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




void ready(void)
{
	puts("READY");
}




void prompt(void)
{
	/* Turned off by EDIT command */
	if (flags.draw_prompt)
	{
		if (autoline_curr)
			PRINT(autoline_str,strlen(autoline_str));
		else
			PRINT("] ",2);
	}
	else flags.draw_prompt = TRUE;
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
	else len = 0; /* Avoids gcc warning */

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




void setAutoLineStr(void)
{
	snprintf(autoline_str,sizeof(autoline_str),"%5u ",autoline_curr);
}




void doExit(int code)
{
	if (!flags.child_process)
	{
		saneMode();
		printf("*** EXIT with code %d ***\n",code);
	}
	exit(code);
}

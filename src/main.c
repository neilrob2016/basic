/*****************************************************************************
 NRJ-BASIC

 An old fashioned line BASIC interpreter for Unix.

 Copyright (C) Neil Robertson 2016-2022
 *****************************************************************************/

#define MAINFILE
#include "globals.h"

#define NUM_KEYB_LINES 100

char *filename;

static void setBuildOptions();
static void parseCmdLine(int argc, char **argv);
static void init();
static void mainloop();
static void sigHandler(int sig);
static void getTermSize(int sig);


/*********************************** START *********************************/

int main(int argc, char **argv, char **env)
{
	int err;

	setBuildOptions();
	parseCmdLine(argc,argv);
	if (!autorun)
	{
		printf("\n%s %s (%s, %s), pid: %u\n%s\n\n",
			INTERPRETER,VERSION,
			build_options[0] ? build_options : "<none>",
			BUILD_DATE,getpid(),COPYRIGHT);
	}
	init();
	createSystemVariables(argc,argv,env);
	rawMode();
	if (filename)
	{
		if ((err = loadProgram(filename,0,FALSE)) != OK)
			doError(err,NULL);
	}
	mainloop();
	saneMode();
	return 0;
}



/********************************* STATICS *********************************/

static void setBuildOptions()
{
	build_options[0] = 0;
#ifdef LINE_FLOAT_ALGO
	strcpy(build_options,"LINE_FLOAT_ALGO ");
#endif
#ifdef NO_CRYPT
	strcat(build_options,"NO_CRYPT ");
#endif
#ifdef DES_ONLY
	strcat(build_options,"DES_ONLY ");
#endif
#ifdef NO_LOG2
	strcat(build_options,"NO_LOG2 ");
#endif
	/* Get rid of trailing space */
	if (build_options[0]) build_options[strlen(build_options)-1] = 0;
}




static void parseCmdLine(int argc, char **argv)
{
	int i;

	filename = NULL;
	autorun = FALSE;
	basic_argc_start = argc;
	num_keyb_lines = NUM_KEYB_LINES;
	cmdline_run_arg = NULL;
	strict_mode = 0;
	kilobyte = 1000;

	for(i=1;i < argc;++i)
	{
		if (argv[i][0] != '-' || strlen(argv[i]) != 2) goto USAGE;

		switch(argv[i][1])
		{
		case 'h':
			if (++i == argc || 
			   (num_keyb_lines = atoi(argv[i])) < 1)
				goto USAGE;

			/* Add 1 because current line is always cleared */
			++num_keyb_lines;
			break;		
			
		case 'l':
			if (++i == argc) goto USAGE;
			filename = argv[i];
			break;

		case 'r':
			if (++i == argc) goto USAGE;
			cmdline_run_arg = argv[i];
			break;

		case 'a':
			autorun = TRUE;
			break;

		case 'k':
			kilobyte = 1024;
			break;

		case 's':
			strict_mode = 1;
			break;

		case 'v':
			printf("\n%s, %s\n\n",INTERPRETER,COPYRIGHT);
			printf("Version      : %s\n",VERSION);
			printf("Build options: %s\n",
				build_options[0] ? build_options : "<none>");
			printf("Build date   : %s\n\n",BUILD_DATE);
			exit(0);

		case '-':
			basic_argc_start = i+1;
			return;


		default:
			goto USAGE;
		}
	}
	return;

	USAGE:
	printf("Usage: %s\n"
	       "       -l <.bas file> : BASIC program file to load at startup.\n"
	       "       -h <lines>     : Number of history lines. Default = %d\n"
	       "       -r <run arg>   : Sets _run_arg system variable.\n"
	       "       -a             : Autorun program.\n"
	       "       -s             : Enforce strict mode which means normal variables must\n"
	       "                        be declared with DIM before use.\n"
	       "       -k             : Use 1024 bytes to calculate kilobytes in DIR* output\n"
	       "                        instead of the default 1000.\n"
	       "       -v             : Print version and build information then exit.\n"
	       "       --             : Everything following this gets passed to BASIC as\n"
	       "                        _argv with _argc system variables set.\n"
	       "Note: All parameters are optional.\n",
		argv[0],NUM_KEYB_LINES);
	exit(1);
}




static int qsortCompare(const void *p1, const void *p2)
{
	/* p1 and p2 are actually point to (char **), not (char *) */
	return strcmp(*(char **)p1, *(char **)p2);
}




static void init()
{
	int size;
	int i;

	size = sizeof(st_keybline) * num_keyb_lines;
	assert((keyb_line = (st_keybline *)malloc(size)));
	bzero(keyb_line,size);

	bzero(first_var,sizeof(first_var));
	bzero(last_var,sizeof(last_var));
	first_defexp = NULL;
	last_defexp = NULL;
	next_keyb_line = 0;
	keyb_lines_free = num_keyb_lines;
	tracing_mode = TRACING_OFF;
	listing_line_wrap = FALSE;
	indent_spaces = 4;
	term_cols_var = NULL;
	term_rows_var = NULL;
	last_signal = 0;
	draw_prompt = TRUE;
	child_process = FALSE;

	bzero(stream,sizeof(stream));
	bzero(popen_fp,sizeof(popen_fp));
	bzero(dir_stream,sizeof(dir_stream));

	/* Sort the commands for HELP */
	for(i=0;i < NUM_COMMANDS;++i) sorted_commands[i] = command[i].name;
	for(i=0;i < NUM_FUNCTIONS;++i) sorted_functions[i] = function[i].name;
	qsort(sorted_commands,NUM_COMMANDS,sizeof(char *),qsortCompare);
	qsort(sorted_functions,NUM_FUNCTIONS,sizeof(char *),qsortCompare);

	initWatchVars();
	initProcessList();
	initDefMods();
	initProgram();

	signal(SIGINT,sigHandler);
	signal(SIGWINCH,getTermSize);
	signal(SIGPIPE,SIG_IGN);
	getTermSize(0);
}




static void mainloop()
{
	char *line;

	/* If -r given then run the loaded program immediately then exit */
	if (autorun)
	{
		processProgLine(prog_first_line);
		return;
	}
	ready();

	/* Enter main user input loop */
	prompt();
	while(getLine(&line))
	{
		if (line) processProgLine(tokenise(line));
		prompt();

		/* Clear up any dead child processes that the program didn't
		   reap itself */
		removeProcessFromList(waitpid(-1,NULL,WNOHANG));
		errno = 0;
	}
	puts("*** EOF on STDIN ***");
}




static void sigHandler(int sig)
{
	last_signal = sig;

	if (sig == SIGINT && !executing)
	{
		puts("*** BREAK ***");
		prompt();
	}
}




static void getTermSize(int sig)
{
#ifdef TIOCGWINSZ
	struct winsize ws;

	if (ioctl(1,TIOCGWINSZ,&ws) != -1 && (ws.ws_col || ws.ws_row))
	{
		term_cols = ws.ws_col;
		term_rows = ws.ws_row;
	}
	else
	{
#endif
		/* Just default to standard terminal screen size if we
		   can't get it */
		term_cols = 80;
		term_rows = 25;
#ifdef TIOCGWINSZ
	}
#endif
	if (term_cols_var)
	{
		setValue(term_cols_var->value,VAL_NUM,NULL,term_cols);
		setValue(term_rows_var->value,VAL_NUM,NULL,term_rows);
	}
	last_signal = sig;
}

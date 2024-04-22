/*****************************************************************************
 NRJ-BASIC

 An old fashioned line BASIC interpreter for Unix.

 Copyright (C) Neil Robertson 2016-2024
 *****************************************************************************/

#define MAINFILE
#include "globals.h"

#define NUM_KEYB_LINES 100

char *prog_filename;

static void setBuildOptions(void);
static void parseCmdLine(int argc, char **argv);
static void init(void);
static void mainloop(void);
static void sigHandler(int sig);
static void getTermSize(int sig);


/*********************************** START *********************************/

int main(int argc, char **argv, char **env)
{
	int err;

	setBuildOptions();
	parseCmdLine(argc,argv);
	if (!flags.autorun)
	{
		printf("\n%s %s (%s, %s), pid: %u\n%s\n\n",
			INTERPRETER,VERSION,
			build_options[0] ? build_options : "<none>",
			BUILD_DATE,getpid(),COPYRIGHT);
	}
	init();
	createSystemVariables(argc,argv,env);
	if (prog_filename)
	{
		if ((err = loadProgram(prog_filename,0,COM_LOAD,!flags.autorun)) != OK)
			doError(err,NULL);
	}
	mainloop();
	saneMode();
	return 0;
}



/********************************* STATICS *********************************/

void setBuildOptions(void)
{
	build_options[0] = 0;
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




void parseCmdLine(int argc, char **argv)
{
	int i;

	prog_filename = NULL;
	flags.autorun = FALSE;
	basic_argc_start = argc;
	num_keyb_lines = NUM_KEYB_LINES;
	cmdline_run_arg = NULL;
	strict_mode = 0;
	kilobyte = 1000;
	bzero(&flags,sizeof(flags));

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
			prog_filename = argv[i];
			break;

		case 'r':
			if (++i == argc) goto USAGE;
			cmdline_run_arg = argv[i];
			break;

		case 'a':
			flags.autorun = TRUE;
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
	if (flags.autorun && !prog_filename)
	{
		fprintf(stderr,"ERROR: The -a argument requires -l.\n");
		exit(1);
	}
	return;

	USAGE:
	printf("Usage: %s\n"
	       "       -l <.bas file> : BASIC program file to load at startup.\n"
	       "       -h <lines>     : Number of history lines. Default = %d\n"
	       "       -r <run arg>   : Sets _run_arg system variable.\n"
	       "       -a             : Auto run program loaded with -l.\n"
	       "       -s             : Enforce strict mode which means normal variables must\n"
	       "                        be declared with DIM before use.\n"
	       "       -k             : Use 1024 bytes to calculate kilobytes in DIR* output\n"
	       "                        instead of the default 1000.\n"
	       "       -v             : Print version and build information then exit.\n"
	       "       --             : Everything following this gets passed to BASIC as\n"
	       "                        _argv with _argc system variables set.\n"
	       "Note: All these arguments are optional.\n",
		argv[0],NUM_KEYB_LINES);
	exit(1);
}




void init(void)
{
	int i;

	srandom(time(0));

	flags.draw_prompt = TRUE;
	tracing_mode = TRACING_OFF;
	indent_spaces = 4;
	term_cols_var = NULL;
	term_rows_var = NULL;
	term_rows = TERM_ROWS;
	term_cols = TERM_COLS;
	last_signal = 0;
	autoline_curr = 0;
	autoline_step = 0;

	bzero(stream,sizeof(stream));
	bzero(stream_flags,sizeof(stream_flags));
	bzero(popen_fp,sizeof(popen_fp));
	bzero(dir_stream,sizeof(dir_stream));

	/* Sort the commands for HELP */
	for(i=0;i < NUM_COMMANDS;++i) sorted_commands[i] = command[i].name;
	for(i=0;i < NUM_FUNCTIONS;++i) sorted_functions[i] = function[i].name;
	qsort(sorted_commands,NUM_COMMANDS,sizeof(char *),qsortCompare);
	qsort(sorted_functions,NUM_FUNCTIONS,sizeof(char *),qsortCompare);

	initKeyboard();
	initVariables();
	initWatchVars();
	initProcessList();
	initDefMods();
	initDefExps();
	initProgram();
	initLabels();

	signal(SIGINT,sigHandler);
	signal(SIGWINCH,getTermSize);
	signal(SIGPIPE,SIG_IGN);
	getTermSize(0);
}




void mainloop(void)
{
	char *line;

	/* If -a given then run the loaded program immediately then exit */
	if (flags.autorun)
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




void sigHandler(int sig)
{
	last_signal = sig;

	if (sig == SIGINT && !flags.executing)
	{
		puts("*** BREAK ***");
		autoline_curr = 0;
		prompt();
	}
}




void getTermSize(int sig)
{
#ifdef TIOCGWINSZ
	struct winsize ws;

	/* Very small chance of a race condition with ioctl() but too much
	   hassle to rejig everything to do it later */
	if (ioctl(1,TIOCGWINSZ,&ws) != -1 && (ws.ws_col || ws.ws_row))
	{
		term_cols = ws.ws_col;
		term_rows = ws.ws_row;
		if (!flags.executing) setTermVariables();
	}
#endif
	last_signal = sig;
}

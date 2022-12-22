#include "globals.h"

static bool pushGosub(st_progline *progline, st_runline *runline);


/*** Execute a program line which might be a direct line or the first line of
     the program. If its "RUN" then it'll set the new program line which we'll
     continue executing in this loop ***/
bool execProgLine(st_progline *progline)
{
	st_runline *runline;
	st_runline *next;
	int linenum;
	bool ok;

	runline = progline->first_runline;
	prog_new_runline = NULL;
	flags.prog_new_runline_set = FALSE;
	flags.executing = TRUE;
	linenum = -1;

	/* Loop until no more runlines. This could be a single program line
	   or an entire program */
	for(ok=TRUE;runline;)
	{
		if (runline->parent->linenum != linenum)
		{
			linenum = runline->parent->linenum;
			setValue(prog_line_var->value,VAL_NUM,NULL,linenum);
		}

		/* Store it in case DELETE or MOVE has been called */
		next = runline->next;
		if (!execRunLine(runline))
		{
			ok = FALSE;
			break;
		}

		/* If something has set it , jump to it */
		if (flags.prog_new_runline_set)
		{
			runline = prog_new_runline;
			flags.prog_new_runline_set = FALSE;
		}
		else runline = next;
	}

	/* If we're a child process then exit - don't go back to prompt */
	if (flags.child_process) exit(0);

	/* Don't want to jump back into the program if we have an error on
	   the command line */
	initOnSettings();

	flags.executing = FALSE;
	return ok;
}




bool execRunLine(st_runline *runline)
{
	st_token *token;
	int err;

	assert(runline->num_tokens);

	/* First token is either a variable or command */
	token = &runline->tokens[0];
	errno = 0;

	switch(token->type)
	{
	case TOK_VAR:
		printTrace(runline->parent->linenum,"COM","LET");
		err = comDimLet(COM_LET,runline);
		break;

	case TOK_COM:
		printTrace(runline->parent->linenum,"COM",token->str);
		err = (*command[token->subtype].funcptr)(token->subtype,runline);
		break;

	default:
		doError(ERR_SYNTAX,runline->parent);
		return FALSE;
	}

	/* Interrupts take precedence over errors because they can cause them */
	switch(last_signal)
	{
	case SIGINT:
		last_signal = 0;
		interrupted_runline = runline;
		setValue(interrupted_var->value,VAL_NUM,NULL,1);

		setValue(
			break_line_var->value,
			VAL_NUM,NULL,runline->parent->linenum);

		/* Delete user variables. This can be used in conjunction with 
		   other ON BREAK settings hence it comes first */
		if (flags.on_break_clear) deleteVariables(NULL);
		if (flags.on_break_cont) return TRUE;

		/* If break handlers set use them */
		if (on_jump[BRK_GOTO])
		{
			setNewRunLine(on_jump[BRK_GOTO]->first_runline);
			return TRUE;
		}
		if (on_jump[BRK_GOSUB])
		{
			if (pushGosub(on_jump[BRK_GOSUB],runline)) return TRUE;
			doError(ERR_MAX_RECURSION,runline->parent);
		}
		else if (runline->parent->linenum) goto BREAK_AT_LINE;
		else puts("*** BREAK ***");

		return FALSE;

	case SIGWINCH:
		last_signal = 0;
		interrupted_runline = runline;
		setValue(interrupted_var->value,VAL_NUM,NULL,1);
		setTermVariables();

		if (flags.on_termsize_cont)
		{
			if (on_jump[TERM_GOTO])
			{
				setNewRunLine(on_jump[TERM_GOTO]->first_runline);
				return TRUE;
			}
			if (on_jump[TERM_GOSUB])
			{
				if (pushGosub(on_jump[TERM_GOSUB],runline)) return TRUE;
				doError(ERR_MAX_RECURSION,runline->parent);
			}
		}
		else goto BREAK_AT_LINE;
		break;
	}

	if (err != OK)
	{
		/* Store last error, system error and line number */
		setValue(error_var->value,VAL_NUM,NULL,err);
		setValue(syserror_var->value,VAL_NUM,NULL,errno);
		setValue(error_line_var->value,VAL_NUM,NULL,runline->parent->linenum);

		if (flags.on_error_cont) return TRUE;

		/* See if error handlers set */
		if (on_jump[ERR_GOTO])
		{
			setNewRunLine(on_jump[ERR_GOTO]->first_runline);
			return TRUE;
		}
		else if (on_jump[ERR_GOSUB])
		{
			if (pushGosub(on_jump[ERR_GOSUB],runline)) return TRUE;
			err = ERR_MAX_RECURSION;
		}
		doError(err,runline->parent);
		return FALSE;
	}

	return TRUE;

	BREAK_AT_LINE:
	if (flags.child_process)
	{
		printf("*** BREAK in line %d, pid %u ***\n",
			runline->parent->linenum,getpid());
	}
	else printf("*** BREAK in line %d ***\n",runline->parent->linenum);

	return FALSE;
}




static bool pushGosub(st_progline *progline, st_runline *runline)
{
	if (return_stack_cnt == MAX_RETURN_STACK) return FALSE;
	setNewRunLine(progline->first_runline);
	return_stack[return_stack_cnt++] = runline->next;
	return TRUE;
}

#include "globals.h"

#define PROGRESS_DOT_LINES 10

int  addProgLine(st_progline *progline);
void removeProgLine(st_progline *progline);
void resetProgPointers();
void spacePad(FILE *fp, int len);


/*** Called at startup, when a program is deleted, new'd or re-run ***/
void subInitProgram()
{
	int i;

	return_stack_cnt = 0;
	bzero(return_stack,sizeof(return_stack));
	interrupted_runline = NULL;
	data_runline = NULL;
	data_autorestore_runline = NULL;
	data_pc = 0;
	last_signal = 0;
	prog_new_runline_set = FALSE;

	initOnSettings();
	deleteDefExps();

	for(i=0;i < MAX_STREAMS;++i)
	{
		if (stream[i])
		{
			close(stream[i]);
			stream[i] = 0;
		}
	}
	for(i=0;i < MAX_DIR_STREAMS;++i)
	{
		if (dir_stream[i])
		{
			closedir(dir_stream[i]);
			dir_stream[i] = NULL;
		}
	}
}




void initOnSettings()
{
	on_error_goto = NULL;
	on_error_gosub = NULL;
	on_break_goto = NULL;
	on_break_gosub = NULL;
	on_break_cont = FALSE;
	on_error_cont = FALSE;
	on_break_clear = FALSE;
}




/*** Called at startup and when a program is deleted ***/
void initProgram()
{
	prog_first_line = NULL;
	prog_new_runline = NULL;
	prog_new_runline_set = FALSE;
	angle_in_degrees = TRUE;

	subInitProgram();
}




/*** Reset everything for a new run ***/
void resetProgram()
{
	subInitProgram();
	if (prog_first_line)
		setNewRunLine(prog_first_line->first_runline);
	else
		prog_new_runline_set = FALSE;

	resetProgPointers();
}




/*** Deal with a single program line. If line is executed the ok value is
     returned ***/
bool processProgLine(st_progline *progline)
{
	st_token *token;
	int err;
	bool ok;

	if (!progline) return TRUE;

	assert(progline->first_runline);

	ok = TRUE;
	token = &progline->first_runline->tokens[0];
	if (IS_NUM(token) && token->subtype == NUM_INT)
	{
		if (token->negative || token->dval > MAX_LINENUM)
		{
			doError(ERR_INVALID_LINENUM,NULL);

			/* Delete temporary progline */
			deleteProgLine(progline,TRUE,TRUE);
		}
		/* If we have more than one token in the first runline then
		   get the line number, delete the line number token and add
		   the whole program line to the program */
		else if (progline->first_runline->num_tokens > 1)
		{
			progline->linenum = (u_int)token->dval;
			deleteTokenFromRunLine(progline->first_runline,0);
			if ((err = addProgLine(progline)) != OK)
			{
				doError(ERR_INVALID_LINENUM,NULL);
				ok = FALSE;
			}
		}
		else
		{
			/* Just have a line number, nothing else which means
			   delete the program line in the program */
			if (!deleteProgLineByNum(token->dval))
				doError(ERR_NO_SUCH_LINE,NULL);

			deleteProgLine(progline,TRUE,TRUE);
		}
	}
	else
	{
		/* Is a direct command - execute then delete */
		ok = execProgLine(progline);
		deleteProgLine(progline,FALSE,FALSE);
	}
	return ok;
}




/*** Put the program line into the program linked list but also attach the
     runlines into their linked list (the entire program) too */
int addProgLine(st_progline *progline)
{
	st_progline *pl;
	st_progline *pl_prev;

	if (progline->linenum < 1) return ERR_INVALID_LINENUM;

	/* Find nearest line number */
	for(pl=prog_first_line,pl_prev=NULL;
	    pl && pl->linenum < progline->linenum;pl_prev=pl,pl=pl->next);

	/* If we have a nearest line ... */
	if (pl)
	{
		if (pl->prev) pl->prev->next = progline;
		progline->prev = pl->prev;

		if (pl->linenum == progline->linenum)
		{
			/* Replace line */
			if (pl->next) pl->next->prev = progline;
			progline->next = pl->next;

			if (prog_first_line == pl)
				prog_first_line = progline;

			deleteProgLine(pl,FALSE,TRUE);
		}
		else
		{
			/* Insert line */
			progline->next = pl;
			pl->prev = progline;
		}
		if (pl == prog_first_line) prog_first_line = progline;
	}
	else if (pl_prev)
	{
		/* Just add to end */
		pl_prev->next = progline;
		progline->prev = pl_prev;
	}
	else prog_first_line = progline;

	/* Link up the runlines to those in the program lines either side */
	if (progline->prev)
	{
		progline->first_runline->prev = progline->prev->last_runline;
		progline->prev->last_runline->next = progline->first_runline;
	}

	if (progline->next)
	{
		progline->last_runline->next = progline->next->first_runline;
		progline->next->first_runline->prev = progline->last_runline;
	}

	/* Could have added a new NEXT, WEND etc so reset */
	resetProgPointers();

	return OK;
}




st_progline *getProgLine(u_int linenum)
{
	st_progline *pl;

	for(pl=prog_first_line;pl;pl=pl->next)
		if (pl->linenum == linenum) return pl;
	return NULL;
}




/*** Delete everything associated with the program ***/
void deleteProgram()
{
	st_progline *progline;
	st_progline *next;

	deleteDefExps();
	deleteVariables(NULL);

	for(progline = prog_first_line;progline;progline=next)
	{
		next = progline->next;
		deleteProgLine(progline,TRUE,TRUE);
	}
	initProgram();
}




void deleteProgLine(st_progline *progline, bool update_ptrs, bool force)
{
	st_runline *runline;
	st_runline *next;

	assert(progline);
	if (!progline->first_runline) return;

	if (interrupted_runline && interrupted_runline->parent == progline)
		interrupted_runline = NULL;

	if (update_ptrs)
	{
		removeProgLine(progline);
		resetProgPointers();
	}

	/* Delete the runlines */
	for(runline=progline->first_runline;;runline=next)
	{
		next = runline->next;

		/* If force is not set the the runline might not actually be
		   deleted if its owned by a DEFEXP */
		deleteDefExp(runline,force);
		deleteRunLine(runline,force);

		/* Can't do this test in the for() in case first_runline == 
		   last_runline **/
		if (runline == progline->last_runline) break;
	}

	free(progline);
}




bool deleteProgLineByNum(u_int linenum)
{
	st_progline *pl;

	for(pl=prog_first_line;pl;pl=pl->next)
	{
		if (pl->linenum == linenum)
		{
			deleteProgLine(pl,TRUE,TRUE);
			return TRUE;
		}
	}
	return FALSE;
}




/*** Delete from -> to inclusive ***/
bool deleteProgLines(u_int from, u_int to)
{
	st_progline *progline;
	st_progline *next;
	bool ret;

	for(progline=prog_first_line,ret=FALSE;progline;progline=next)
	{
		next = progline->next;
		if (progline->linenum >= from)
		{
			if (to && progline->linenum > to) break;
			deleteProgLine(progline,TRUE,TRUE);
			ret = TRUE;
		}
	}

	/* Just to be safe */
	subInitProgram();

	return ret;
}




void setNewRunLine(st_runline *runline)
{
	prog_new_runline_set = TRUE;
	prog_new_runline = runline;
}




/*** Update linked list pointers either side of the line ***/
void removeProgLine(st_progline *progline)
{
	if (progline->prev)
	{
		progline->prev->next = progline->next;
		progline->first_runline->prev->next = progline->last_runline->next;
	}
	if (progline->next)
	{
		progline->next->prev = progline->prev;
		progline->next->first_runline->prev = progline->first_runline->prev;
	}
	if (progline == prog_first_line)
		prog_first_line = prog_first_line->next;

	progline->prev = NULL;
	progline->next = NULL;
	progline->first_runline->prev = NULL;
	progline->last_runline->next = NULL;
}




/*** Reset jump pointers and delete any for loop structs ***/
void resetProgPointers()
{
	st_runline *runline;
	int i;

	if (!prog_first_line) return;

	for(runline=prog_first_line->first_runline;
	    runline;runline=runline->next)
	{
		FREE(runline->for_loop);
		FREE(runline->foreach_loop);
		runline->jump = NULL;
		runline->else_jump = NULL;
		runline->next_case = NULL;
		for(i=0;i < runline->num_tokens;++i)
			runline->tokens[i].lazy_jump = 0;
	}
}




/*** Load a program from disk. Will also load and run direct commands in the
     file too ***/
int loadProgram(char *filename, u_int merge_linenum, bool delprog)
{
	st_progline *progline;
	st_token *token;
	FILE *fp;
	char matchpath[PATH_MAX+1];
	char *line;
	char *ret;
	char *tmp;
	int alloced;
	int len;
	int err;
	int linecnt;

	line = NULL;
	fp = NULL;

	/* Add .bas on end if not there */
	if ((tmp = addFileExtension(filename))) filename = tmp;

	if ((err = getFirstFileMatch(S_IFREG,filename,matchpath,0)) != OK)
		goto ERROR;
	
	if (!autorun)
	{
		printf("LOADING: %s",matchpath);
		fflush(stdout);
	}
	if (!(fp = fopen(matchpath,"r")))
	{
		err = ERR_CANT_OPEN_FILE;
		goto ERROR;
	}

	if (delprog) deleteProgram();
	
	linecnt = 0;
	alloced = 0;
	errno = 0;

	while(1)
	{
		/* +1 for \0 */
		assert((line = (char *)realloc(line,alloced+CHAR_ALLOC+1)));

		/* +1 because fgets() reads in size-1 bytes */
		line[alloced] = 0;
		if (!(ret = fgets(line+alloced,CHAR_ALLOC+1,fp)) && errno)
		{
			/* Read error */
			err = ERR_READ;
			goto ERROR;
		}

		len = strlen(line)-1;

		/* EOL or EOF */
		if (!ret || line[len] == '\n')
		{
			if (!autorun && !(++linecnt % PROGRESS_DOT_LINES))
				PRINT(".",1);

			if (ret) line[len] = 0;
			if (line[0])
			{
				if (!(progline = tokenise(line))) break;

				/* If we're mergeing make sure we don't 
				   overwrite merge line else we'll crash */
				if (merge_linenum && progline->first_runline)
				{
					token = &progline->first_runline->tokens[0];
					if (IS_NUM(token) && 
					    token->subtype == NUM_INT && 
					    token->dval <= merge_linenum)
					{
						err = ERR_CANT_MERGE;
						goto ERROR;
					}
				}

				if (!processProgLine(progline)) break;
			}

			if (!ret) break;

			free(line);
			line = NULL;
			alloced = 0;
		}
		else alloced += CHAR_ALLOC;
	}
	fclose(fp);
	FREE(line);
	if (!autorun) putchar('\n');
	return OK;

	ERROR: 
	if (fp) fclose(fp);
	FREE(line);
	FREE(tmp);
	return err;
}



/* Wrap lines nicely with indentation. Don't just leave it up to the term */
#define AUTOWRAP(INC) \
	if (fp == stdout && listing_line_wrap) \
	{ \
		line_width += (INC); \
		if (line_width + loop_indent + if_indent >= term_cols) \
		{ \
			fputc('\n',fp); \
			spacePad(fp,9+loop_indent+if_indent); \
			line_width = 9; \
			++pause_linecnt; \
		} \
	}


/*** This can output to the screen for a listing or a file for saving. Check
     the fprintf() return values for file write errors. Don't need to check
     fputc() too since if they fail so will the next fprintf() anyway. ***/
int listProgram(FILE *fp, u_int from, u_int to, bool pause)
{
	st_progline *progline;
	st_runline *runline;
	st_token *token;
	st_token *next_token;
	bool start;
	bool in_rem;
	int if_indent;
	int loop_indent;
	int linecnt;
	int pause_linecnt;
	int line_width;
	int print_colon;
	int prev_tok_was_com;
	int ret;
	int i;

	if_indent = 0;
	loop_indent = 0;
	linecnt = 0;
	pause_linecnt = 0;

	for(progline=prog_first_line;
	    progline && last_signal != SIGINT;progline=progline->next)
	{
		if (progline->linenum < from) continue;
		if (to && progline->linenum > to) break;

		if (pause)
		{
 			if (pause_linecnt == term_rows - 3)
			{
				if (pressAnyKey("Space to advance 1 line, any other key to page: ") != ' ')
					pause_linecnt = 0;
			}
			else ++pause_linecnt;
		}

		/* Print line number */
		if (fprintf(fp,"%5u ",progline->linenum) == -1)
			return ERR_WRITE;
		line_width = 6;

		/* Do indentation. Look ahead to next token first */
		token = &progline->first_runline->tokens[0];
		if (IS_COM(token))
		{
			switch(token->subtype)
			{
			case COM_WEND:
			case COM_UNTIL:
			case COM_NEXT:
			case COM_NEXTEACH:
			case COM_LEND:
			case COM_CHOSEN:
				if (loop_indent) loop_indent -= indent_spaces;
				break;

			case COM_FI:
			case COM_ELSE:
				if (if_indent) if_indent -= indent_spaces;
				break;

			case COM_FIALL:
				if_indent = 0;
				break;
			}
		}
		spacePad(fp,loop_indent + if_indent);

		ret = 0;
		print_colon = 0;

		for(runline=progline->first_runline;;runline=runline->next)
		{
			token = &runline->tokens[0];

			if (print_colon)
			{
				/* If the 1st token is FI, FIALL or ELSE don't
				   print the colon */
				if (!IS_COM(token) ||
				    (token->subtype != COM_FI &&
				     token->subtype != COM_FIALL &&
				     token->subtype != COM_ELSE)) fputc(':',fp);

				/* Don't add a space if just printed command */
				if (!prev_tok_was_com) fputc(' ',fp);
			}
			in_rem = IS_COM_TYPE(token,COM_REM);
			prev_tok_was_com = 0;
			start = TRUE;

			for(i=0;i < runline->num_tokens;++i)
			{
				token = &runline->tokens[i];
				next_token = (i+1 < runline->num_tokens) ? 
				              &runline->tokens[i+1] : NULL;

				if (in_rem && i)
				{
					AUTOWRAP(token->len+1);
					if (fputs(token->str,fp) == -1)
						return ERR_WRITE;
					/* REM only has 1 token for text */
					break;
				}
				if (token->negative)
				{
					AUTOWRAP(1);
					fputc('-',fp);
				}

				prev_tok_was_com = 0;

				switch(token->type)
				{
				case TOK_STR:
					/* Add back quotes */
					AUTOWRAP(token->len + 2);
					ret = fprintf(fp,"\"%s\"",token->str);
					break;

				case TOK_COM:
					prev_tok_was_com = 1;

					/* If not start put leading space */
					if (!start)
					{
						AUTOWRAP(1);
						fputc(' ',fp);
					}
					AUTOWRAP(token->len+1);

					/* Don't put a trailing space if its a
					   command */
					if (next_token &&
					    next_token->type == TOK_COM)
						ret = fputs(token->str,fp);
					else
						ret = fprintf(fp,"%s ",token->str);

					/* See if we need to add or remove 
					   indentation */
					switch(token->subtype)
					{
					case COM_WHILE:
					case COM_REPEAT:
					case COM_FOR:
					case COM_FOREACH:
					case COM_LOOP:
					case COM_CHOOSE:
						loop_indent += indent_spaces;
						break;

					case COM_IF:
						if_indent += indent_spaces;
						break;

					case COM_ELSE:
						if (runline == progline->first_runline)
							if_indent += indent_spaces;
						break;

					case COM_WEND:
					case COM_UNTIL:
					case COM_NEXT:
					case COM_NEXTEACH:
					case COM_LEND:
					case COM_CHOSEN:
						/* Don't subtract if already 
						   done above */
						if (loop_indent && (
						    i || runline != progline->first_runline))
						{
							loop_indent -= indent_spaces;
						}
						break;

					case COM_FI:
						if (if_indent && 
						    (i || runline != progline->first_runline))
							if_indent -= indent_spaces;
						break;

					case COM_FIALL:
						if_indent = 0;
					}
					break;

				case TOK_OP:
					/* Put spaces around most operators 
					   apart from the exceptions below */
					switch(token->subtype)
					{
					case OP_COMMA:
					case OP_COLON:
					case OP_HASH:
					case OP_AT:
					case OP_SEMI_COLON:
					case OP_L_BRACKET:
					case OP_R_BRACKET:
					case OP_BIT_COMPL:
						AUTOWRAP(token->len);
						ret = fprintf(fp,"%s",token->str);
						break;

					case OP_NOT:
						AUTOWRAP(token->len+1);
						ret = fprintf(fp,"%s ",token->str);
						break;

					default:
						AUTOWRAP(token->len+2);
						ret = fprintf(fp," %s ",token->str);
					}
					break;

				default:
					AUTOWRAP(token->len);
					ret = fprintf(fp,"%s",token->str);
				}
				start = FALSE;
			}
			if (ret == -1 || fflush(fp) == -1) return ERR_WRITE;
			if (runline == progline->last_runline) break;

			AUTOWRAP(2);

			/* Don't print a colon following THEN or ELSE if the
			   line continues, it looks neater without them. */
			print_colon = !token || 
			              (!IS_COM_TYPE(token,COM_THEN) && !IS_COM_TYPE(token,COM_ELSE));
		}	
		if (fp != stdout && !(++linecnt % PROGRESS_DOT_LINES))
			PRINT(".",1);
		fputc('\n',fp);
	}
	return OK;
}




/*** This doesn't just renumber the line, it moves it elsewhere ***/
int moveProgLine(u_int from, u_int to)
{
	st_progline *progline;
	int err;

	if (from == to) return OK;
	if (from < 1 || to < 1) return ERR_INVALID_LINENUM;

	/* Make sure from line exists */
	if (!(progline = getProgLine(from))) return ERR_NO_SUCH_LINE;

	/* Make sure to line doesn't exist */
	if (getProgLine(to)) return ERR_LINE_EXISTS;

	/* Pull line out of the program, change line number then re-insert */
	removeProgLine(progline);
	progline->linenum = to;
	err = addProgLine(progline);

	resetProgPointers();

	return err;
}




void spacePad(FILE *fp, int len)
{
	int i;
	for(i=0;i < len;++i) putc(' ',fp);
}

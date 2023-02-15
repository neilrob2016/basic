/*** The command functions are not in the same order as the commands are
     defined in globals.h ***/
#include "globals.h"

#define CHECK_DIR_STREAM(S) \
	if ((S) < 0 || (S) >= MAX_STREAMS) return ERR_INVALID_STREAM; \
	if (!dir_stream[S]) return ERR_STREAM_NOT_OPEN;

#define STREAM_STDIO  -1 
#define STREAM_PRINTER -2

static int  doComRun(int comnum, st_runline *runline, int pc);
static bool setBlockEnd(st_runline *runline, int start_com, int end_com);
static int  getRunLineValue(
	st_runline *runline, int *pc, int type, int eol, st_value *result);


/****************************** DEFAULT FUNCS ********************************/

/*** For commands that do nothing and take no arguments. Eg FI ***/
int comDefault(int comnum, st_runline *runline)
{
	if (runline->num_tokens > 1) return ERR_SYNTAX;
	return OK;
}




/*** For commands that should never appear on their own. eg: TO, STEP ***/
int comUnexpected(int comnum, st_runline *runline)
{
	return ERR_SYNTAX;
}




/*** Do absolutely nothing, not even a token count check ***/
int comDoNothing(int comnum, st_runline *runline)
{
	return OK;
}



/******************************** PROGRAM *********************************/

/*** List the program or command history. 3 formats:
     LIST/HISTORY                : List all lines
     LIST/HISTORY <from>         : List just line <from>
     LIST/HISTORY <from> TO END  : List lines <from> to the end
     LIST/HISTORY <from>,<to>    : List lines from <from> to <to>
     LIST/HISTORY TO <to>        : List lines from the start to <to>
     LIST/HISTORY TO END         : Same as LIST/HISTORY on their own.
  ***/
int comListHistory(int comnum, st_runline *runline)
{
	st_value fromval;
	st_value toval;
	FILE *pfp;
	int pos;
	int cnt;
	int from;
	int to;
	int pc;
	int err;

	if (runline->num_tokens == 1)
	{
		/* Command on its own */
		from = (comnum == COM_HISTORY); /* Start at 1 for HISTORY */
		to = 0;
	}
	else
	{
		pc = 1;
		if (IS_COM_TYPE(&runline->tokens[pc],COM_TO))
		{
			if (pc == runline->num_tokens - 1) return ERR_SYNTAX;
			from = (comnum == COM_HISTORY);
		}
		else
		{
			/* Get FROM line */
			initValue(&fromval);
			if ((err = evalExpression(runline,&pc,&fromval)) != OK)
				return err;
			from = fromval.dval;

			if (pc == runline->num_tokens)
			{
				to = from;
				goto EXEC;
			}
			if (runline->num_tokens - pc < 2 || 
			    !IS_COM_TYPE(&runline->tokens[pc],COM_TO))
			{
				return ERR_SYNTAX;
			}
		}

		/* Get TO line */
		initValue(&toval);
		++pc;
		if (IS_COM_TYPE(&runline->tokens[pc],COM_END))
		{
			if (pc < runline->num_tokens - 1) return ERR_SYNTAX;
			to = 0;
		}
		else
		{
			if ((err = getRunLineValue(
				runline,&pc,VAL_NUM,TRUE,&toval)) != OK)
			{
				return err;
			}
			to = toval.dval;
		}

		if (from < 0 || to < 0) return ERR_INVALID_LINENUM;
		if (to && from > to) return ERR_INVALID_ARG;
	}

	EXEC:
	switch(comnum)
	{
	case COM_HISTORY:
		pos = (next_keyb_line + keyb_lines_free + from - 1) % num_keyb_lines;
		if (pos < 0 || !keyb_line[pos].len)
			return ERR_INVALID_HISTORY_LINE;
		for(cnt=from;keyb_line[pos].len && (!to || cnt <= to);++cnt)
		{
			printf("%2d: %s\n",cnt,keyb_line[pos].str);
			pos = (pos + 1) % num_keyb_lines;
		}
		return OK;
		
	case COM_LIST:
	case COM_PLIST:
		return listProgram(stdout,(u_int)from,(u_int)to,comnum == COM_PLIST);

	case COM_LLIST:
		/* Only fails if fork() or pipe() fails */
		if (!(pfp = popen("lp","w"))) return ERR_LP;
		printf("Spooling to printer: ");
		fflush(stdout);	
		err = listProgram(pfp,(u_int)from,(u_int)to,FALSE);
		pclose(pfp);
		return err;

	default:
		break;
	}

	/* Should never get here */
	assert(0);
	return OK;
}




/*** Renumber the lines. Format:
     RENUM
     RENUM <gap>
     RENUM <gap> FROM <start line>
 ***/
int comRenum(int comnum, st_runline *runline)
{
	st_progline *pl;
	st_runline *rl;
	st_token *token;
	st_value result;
	char linestr[20];
	int gap;
	int linenum;
	int pc;
	int err;
	int num_tokens;
	int start = 0;
	int start_set = 0;

	if (runline->num_tokens > 1)
	{
		pc = 1;
		initValue(&result);
		if ((err = getRunLineValue(runline,&pc,VAL_NUM,EITHER,&result)) != OK)
			return err;

		if (result.dval < 1) return ERR_INVALID_ARG;
		gap = (int)result.dval;

		if (pc < runline->num_tokens)
		{
			/* Get the start line */
			if (runline->num_tokens - pc < 2 ||
			    !IS_COM_TYPE(&runline->tokens[pc],COM_FROM))
				return ERR_SYNTAX;

			++pc;
			if ((err = getRunLineValue(
				runline,&pc,VAL_NUM,TRUE,&result)) != OK)
			{
				return err;
			}
			if (result.dval < 1) return ERR_INVALID_LINENUM;
			start = (int)result.dval;
			start_set = 1;
		}
	}
	else gap = 10;

	if (!prog_first_line) return OK;

	linenum = (start_set ? start : gap);

	/* Reset all renumbered flags */
	for(rl=prog_first_line->first_runline;rl;rl=rl->next)
		rl->renumbered = FALSE;

	for(pl=prog_first_line;pl;pl=pl->next)
	{
		if (pl->linenum < start) continue;

		/* Go through all runlines and change any commands that
		   pointed to this line number */
		for(rl=prog_first_line->first_runline;rl;rl=rl->next)
		{
			if (rl->num_tokens < 2 || 
			    rl->renumbered || 
			    !IS_COM(&rl->tokens[0])) continue;

			if (rl->tokens[0].subtype == COM_ON)
				num_tokens = 4;
			else
				num_tokens = 2;

			/* Only want line numbers that arn't part of an
			   expression */
			if (rl->num_tokens != num_tokens ||
 			    !IS_NUM(&rl->tokens[num_tokens-1])) continue;

			token = &rl->tokens[num_tokens-1];
			if ((u_int)token->dval == pl->linenum)
			{
				switch(rl->tokens[0].subtype)
				{
				case COM_GOTO:
				case COM_GOSUB:
				case COM_RESTORE:
				case COM_AUTORESTORE:
				case COM_ON:
					sprintf(linestr,"%u",linenum);
					token->dval = linenum;
					free(token->str);
					token->str = strdup(linestr);
					assert(token->str);
					rl->renumbered = TRUE;
					break;
				}
			}
		}
		pl->linenum = linenum;
		linenum += gap;
	}
	return OK;
}




int comNew(int comnum, st_runline *runline)
{
	deleteProgram();
	killChildProcesses();
	resetSystemVariables();
	clearWatchVars();
	clearDefMods();
	ready();
	return OK;
}




int comRun(int comnum, st_runline *runline)
{
	return doComRun(comnum,runline,1);
}
	


/*** Too much hassle to rejig listProgram() to write into memory just for
     this command so it does it own half arsed line listing which occasionally
     adds too many spaces. Not a big deal. ***/
int comEdit(int comnum, st_runline *runline)
{
	st_progline *pl;
	st_runline *rl;
	st_value result;
	st_token *token;
	st_token *next_token;
	char str[20];
	bool add_colon;
	int err;
	int pc;
	int i;

	if (runline->parent->linenum) return ERR_NOT_ALLOWED_IN_PROG;

	pc = 1;
	initValue(&result);
	if ((err = getRunLineValue(runline,&pc,VAL_NUM,TRUE,&result)) != OK)
		return err;

	if (result.dval < 0) return ERR_INVALID_ARG;
	if (!(pl = getProgLine(runline->tokens[1].dval)))
		return ERR_NO_SUCH_LINE;

	sprintf(str,"%u ",pl->linenum);
	clearKeyLine(next_keyb_line);
	addWordToKeyLine(str);

	for(rl=pl->first_runline;rl;rl=rl->next)
	{
		add_colon = TRUE;

		for(i=0;i < rl->num_tokens;++i)
		{
			token = &rl->tokens[i];
			next_token = (i+1 < rl->num_tokens) ?
			             &rl->tokens[i+1] : NULL;

			if (token->negative) addWordToKeyLine("-");

			switch(token->type)
			{
			case TOK_COM:
				if (rl != pl->first_runline || i)
					addWordToKeyLine(" ");
				addWordToKeyLine(token->str);
				if (IS_COM_TYPE(token,COM_THEN) ||
				    IS_COM_TYPE(token,COM_ELSE))
				{
					add_colon = FALSE;
				}

				/* Add a trailing space if next token is not a 
				   command */
				if (next_token && !IS_COM(next_token))
					addWordToKeyLine(" ");
				break;

			case TOK_OP:
				/* Some operators need spaces, others don't */
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
					addWordToKeyLine(token->str);
					break;

				case OP_NOT:
					addWordToKeyLine(token->str);
					addWordToKeyLine(" ");
					break;

				default:
					addWordToKeyLine(" ");
					addWordToKeyLine(token->str);
					addWordToKeyLine(" ");
				}
				break;

			case TOK_STR:
				addWordToKeyLine("\"");
				addWordToKeyLine(token->str);
				addWordToKeyLine("\"");
				break;

			default:
				addWordToKeyLine(token->str);
			}
		}
		if (rl == pl->last_runline) break;
		addWordToKeyLine(" ");
		if (add_colon) addWordToKeyLine(":");
	}
	
	prompt();
	drawKeyLine(next_keyb_line);
	flags.draw_prompt = FALSE;
	return OK;
}




/*** Format:
     DELETE <line>         : Delete a single line.
     DELETE <from> TO <to> : Delete all the lines from <from> to <to> inclusive.
     DELETE <from> TO END  : Delete all the lines from <from> to the end of the
                             program.
     DELETE TO <line>      : Delete all the lines from the start to the given
                             line.
     DELETE to END         : Delete all the lines.
     MOVE   <from> TO <to> : Move/renumber the line from <from> to <to>
 ***/
int comDeleteMove(int comnum, st_runline *runline)
{
	st_value fromval;
	st_value toval;
	int from;
	int to;
	int pc;
	int err;

	/* Don't allow delete because deleting a range in the program will 
	   cause a coredump and I can't be arsed to fix it so it doesn't. */
	if (comnum == COM_DELETE && runline->parent->linenum)
		return ERR_NOT_ALLOWED_IN_PROG;

	pc = 1;
	if (IS_COM_TYPE(&runline->tokens[pc],COM_TO))
	{
		if (comnum == COM_MOVE ||
		    pc == runline->num_tokens - 1) return ERR_SYNTAX;
		from = 0;
	}
	else
	{
		/* Get FROM line */
		initValue(&fromval);
		if ((err = evalExpression(runline,&pc,&fromval)) != OK)
			return err;
		from = fromval.dval;

		if (pc == runline->num_tokens)
		{
			if (comnum == COM_MOVE) return ERR_SYNTAX;
			to = from;
			goto EXEC;
		}
		if (runline->num_tokens - pc < 2 ||
		    !IS_COM_TYPE(&runline->tokens[pc],COM_TO))
		{
			return ERR_SYNTAX;
		}
	}

	/* Get TO line */
	initValue(&toval);
	++pc;
	if (IS_COM_TYPE(&runline->tokens[pc],COM_END))
	{
		if (comnum == COM_MOVE ||
		    pc < runline->num_tokens - 1) return ERR_SYNTAX;
		to = 0;
	}
	else
	{
		if ((err = getRunLineValue(
			runline,&pc,VAL_NUM,TRUE,&toval)) != OK)
		{
			return err;
		}
		to = toval.dval;
	}
	if (from < 0 || to < 0) return ERR_INVALID_LINENUM;
	if (comnum == COM_DELETE && to && from > to)
		return ERR_INVALID_ARG;

	EXEC:
	if (comnum == COM_DELETE)
		return deleteProgLines((u_int)from,(u_int)to) ? OK : ERR_NO_SUCH_LINE;
	return moveProgLine((u_int)from,(u_int)to);
}




/*** Switches line wrap on & off in LIST ***/
int comWrap(int comnum, st_runline *runline)
{
	flags.listing_line_wrap = (comnum == COM_WRON);
	return OK;
}




/*** Sets number of spaces to indent by ***/
int comIndent(int comnum, st_runline *runline)
{
	st_value result;
	int err;
	int pc = 1;

	initValue(&result);
	if ((err = getRunLineValue(runline,&pc,VAL_NUM,TRUE,&result)) != OK)
		return err;

	if (result.dval < 0) return ERR_INVALID_ARG;
	indent_spaces = (int)result.dval;
	setValue(indent_var->value,VAL_NUM,NULL,indent_spaces);
	return OK;
}



/******************************** VARIABLES **********************************/

int comClear(int comnum, st_runline *runline)
{
	st_token *token;
	st_var *var;
	int pc;

	/* If no arguments delete all variables and reset program */
	if (runline->num_tokens == 1)
	{
		deleteVariables(runline);
		resetSystemVariables();
		clearDefMods();
		subInitProgram();
		return OK;
	}

	/* If comma seperated list, eg: CLEAR a,b,c then just delete the
	   variables given */
	for(pc=1;pc < runline->num_tokens;++pc)
	{
		token = &runline->tokens[pc];
		if (!IS_VAR(token)) return ERR_INVALID_ARG;
		if (!(var = getVariable(token->str))) return ERR_UNDEFINED_VAR_FUNC;
		deleteVariable(var,runline);

		if (++pc == runline->num_tokens) return OK;
		if (NOT_COMMA(pc)) break;
	}
	return ERR_SYNTAX;
}




/*** DIM/CDIM/LET ***/
int comDimLet(int comnum, st_runline *runline)
{
	st_token *token;
	st_value result;
	st_value icnt_or_key;
	st_var *var;
	int index[MAX_INDEXES];
	int index_cnt;
	int err;
	int pc;
	int type;

	initValue(&result);
	initValue(&icnt_or_key);

	pc = IS_COM(&runline->tokens[0]);
	if (comnum == COM_LET && runline->num_tokens <= pc + 2)
		return ERR_SYNTAX;

	for(;pc < runline->num_tokens;++pc)
	{
		token = &runline->tokens[pc];
		if (!IS_VAR(token)) return ERR_SYNTAX;
		if (getDefExp(token->str)) return ERR_DEFEXP_ALREADY_HAS_NAME;

		switch(comnum)
		{
		case COM_DIM:
			var = getVariable(token->str);
			if (var) return ERR_VAR_ALREADY_DEF;
			break;

		case COM_CDIM:
			var = getVariable(token->str);
			if (var) deleteVariable(var,runline);
			break;

		case COM_LET:
			/* If strict set then error if var doesn't exist */
			if (!token->var && 
			    !(token->var = getVariable(token->str)) &&
			    strict_mode)
			{
				return ERR_UNDEFINED_VAR_FUNC;
			}
			break;

		default:
			assert(0);
		}

		type = VAR_STD;

		/* Get index(es). eg: a(1,2,3) */
		clearValue(&icnt_or_key);

		if (++pc < runline->num_tokens)
		{
			if (IS_OP_TYPE(&runline->tokens[pc],OP_AT))
			{
				if (comnum == COM_LET)
				{
					err = ERR_SYNTAX;
					goto ERROR;
				}
				type = VAR_MEM;
				++pc;
			}
			if (IS_OP_TYPE(&runline->tokens[pc],OP_HASH))
			{
				if (comnum == COM_LET || type != VAR_STD)
				{
					err = ERR_SYNTAX;
					goto ERROR;
				}
				type = VAR_MAP;
				++pc;
			}
			if (IS_OP_TYPE(&runline->tokens[pc],OP_L_BRACKET))
			{
				if (type == VAR_MAP)
				{
					err = ERR_MAP_ARRAY;
					goto ERROR;
				}
				
				if ((err = getVarIndexes(runline,&pc,&icnt_or_key,index)) != OK)
					goto ERROR;

				/* Can't dimension with a string */
				if (comnum != COM_LET && 
				    icnt_or_key.type == VAL_STR)
				{
					err = ERR_INVALID_ARG;
					goto ERROR;
				}
			}
		}
		index_cnt = (int)icnt_or_key.dval;

		switch(comnum)
		{
		case COM_DIM:
		case COM_CDIM:
			if (type == VAR_MEM)
			{
				if (!index_cnt)
				{
					err = ERR_VAR_NO_MEM_SIZE;
					goto ERROR;
				}
				if (index_cnt != 1)
				{
					err = ERR_INVALID_ARRAY_TYPE;
					goto ERROR;
				}
			}

			/* Check name only contains valid characters */
			if (!validVariableName(token->str))
			{
				err = ERR_INVALID_VAR_NAME;
				goto ERROR;
			}

			if (!(token->var = createVariable(
				token->str,type,index_cnt,index)))
			{
				err = ERR_SHARMEM;
				goto ERROR;
			}

			/* Reset for assignment */
			if (type != VAR_MAP) clearValue(&icnt_or_key);
			break;

		case COM_LET:
			if (!token->var)
			{
				/* Create the variable */
				if (!validVariableName(token->str)) 
					return ERR_INVALID_VAR_NAME;
				token->var = createVariable(
					token->str,VAR_STD,index_cnt,index);
				clearValue(&icnt_or_key);
			}
			break;

		default:
			assert(0);
		}

		/* Check for assignment */
		if (IS_OP_TYPE(&runline->tokens[pc],OP_EQUALS)) 
		{
			/* Evaluate expression */
			if (++pc == runline->num_tokens)
			{
				err = ERR_SYNTAX;
				goto ERROR;
			}
			if ((err = evalExpression(runline,&pc,&result)) != OK)
				goto ERROR;

			if ((err = setVarValue(
				token->var,
				&icnt_or_key,index,&result,FALSE)) != OK)
			{
				goto ERROR;
			}
		}
		/* Must have assignment with LET */
		else if (comnum == COM_LET) return ERR_SYNTAX;

		if (NOT_COMMA(pc)) break;
	}
	err = (pc < runline->num_tokens ? ERR_SYNTAX : OK);

	ERROR:
	clearValue(&result);
	clearValue(&icnt_or_key);
	return err;
}




int comRedim(int comnum, st_runline *runline)
{
	st_token *token;
	st_value result;
	st_value icnt_or_key;
	int index[MAX_INDEXES];
	int err;
	int pc;

	initValue(&result);
	initValue(&icnt_or_key);

	for(pc=1;pc < runline->num_tokens;++pc)
	{
		token = &runline->tokens[pc];
		if (!IS_VAR(token)) 
		{
			err = ERR_SYNTAX;
			goto ERROR;
		}

		/* Variable must be defined to REDIM it */
		if (!token->var && !(token->var = getVariable(token->str)))
		{
			err = ERR_UNDEFINED_VAR_FUNC;
			goto ERROR;
		}

		if (token->var->type != VAR_STD) return ERR_VAR_IS_NOT_ARRAY;

		if (++pc == runline->num_tokens ||
		    !IS_OP_TYPE(&runline->tokens[pc],OP_L_BRACKET))
		{
			err = ERR_SYNTAX;
			goto ERROR;
		}
		
		/* Get index(es). eg: a(1,2,3) */
		if ((err = getVarIndexes(runline,&pc,&icnt_or_key,index)) != OK)
			goto ERROR;

		/* Can't dimension with a string */
		if (icnt_or_key.type == VAL_STR)
		{
			err = ERR_INVALID_ARG;
			goto ERROR;
		}
		if ((err = reDimArray(token->var,icnt_or_key.dval,index)) != OK)
			goto ERROR;

		if (pc == runline->num_tokens) break;

		/* Check for assignment */
		if (IS_OP_TYPE(&runline->tokens[pc],OP_EQUALS))
		{
			clearValue(&icnt_or_key);
			if (++pc == runline->num_tokens)
			{
				err = ERR_SYNTAX;
				goto ERROR;
			}
			if ((err = evalExpression(runline,&pc,&result)) != OK)
				goto ERROR;

			if ((err = setVarValue(
				token->var,
				&icnt_or_key,index,&result,FALSE)) != OK)
			{
				goto ERROR;
			}
		}
		else if (NOT_COMMA(pc)) break;
	}
	err = (pc < runline->num_tokens ? ERR_SYNTAX : OK);
	/* Fall through */

	ERROR:
	clearValue(&result);
	clearValue(&icnt_or_key);
	return err;
}




/*** Dump all variables to the screen or printer. I thought about allowing
     BASIC streams to output anywhere but because they use file descriptors not
     file pointers it would be too much hassle to convert all the printfs() and
     *put*s to sprintf() and write(). ***/
int comDump(int comnum, st_runline *runline)
{
	FILE *fp;
	st_token *token;
	bool dump_contents;
	int total;
	int err;
	int cnt;
	int pc;

	if (IS_COM_TYPE(&runline->tokens[1],COM_FULL))
	{
		dump_contents = TRUE;
		pc = 2;
	}
	else
	{
		dump_contents = FALSE;
		pc = 1;
	}
	if (comnum == COM_LDUMP)
	{
		if (!(fp = popen("lp","w"))) return ERR_LP;
		puts("Spooling to printer...");
	}
	else fp = stdout;

	err = OK;
	if (runline->num_tokens == 1 + dump_contents)
	{
		/* Dump everything */
		dumpVariables(fp,NULL,dump_contents);
		dumpDefExps(fp,NULL);
		goto DONE;
	}

	/* Dump specific wildcard matches */
	for(total=0;pc < runline->num_tokens;++pc)
	{
		token = &runline->tokens[pc];
		cnt = 0;

		/* The ! isn't part of the name so won't match */
		if (token->str[0] == '!') cnt = dumpDefExps(fp,token->str+1);
		else
		{
			cnt = dumpVariables(fp,token->str,dump_contents);
			cnt += dumpDefExps(fp,token->str);
		}
		if (cnt)
			total += cnt;
		else
			printf("%-15s [NO MATCH]\n",token->str);

		if (++pc == runline->num_tokens || NOT_COMMA(pc)) break;
	}
	fprintf(fp,"%d matches.\n",total);

	DONE:
	if (fp != stdout) pclose(fp);
	if (pc < runline->num_tokens) return ERR_SYNTAX;
	return err;
}




/*** Format: RENAME <old name> TO <new name> ***/
int comRename(int comnum, st_runline *runline)
{
	st_token *fromtok = &runline->tokens[1];
	st_token *totok = &runline->tokens[3];
	int ret;
	int cnt;

	if (runline->num_tokens != 4 ||
	    !IS_COM_TYPE(&runline->tokens[2],COM_TO)) return ERR_SYNTAX;

	if (fromtok->type != TOK_VAR || totok->type != TOK_VAR)
		return ERR_RENAME_TYPE;

	if (!strcmp(fromtok->str,totok->str)) return ERR_RENAME_SAME;

	if ((ret = renameProgVarsAndDefExps(fromtok->str,totok->str,&cnt)) != OK)
		return ret;
	printf("%d renamings.\n",cnt);
	return OK;
}




int comStrict(int comnum, st_runline *runline)
{
	strict_mode = (comnum == COM_STON);
	setValue(strict_mode_var->value,VAL_NUM,NULL,strict_mode);
	return OK;
}


/*************************** CONTROL CONSTRUCTS ******************************/

int comIf(int comnum, st_runline *runline)
{
	st_value result;
	int err;
	int pc;

	/* If jump pointer not set then find our matching FI */
	if (!runline->jump && !setBlockEnd(runline,COM_IF,COM_FI))
		return ERR_MISSING_FI;

	if (!runline->else_jump) setBlockEnd(runline,COM_IF,COM_ELSE);

	pc = 1;
	initValue(&result);
	if ((err = evalExpression(runline,&pc,&result)) != OK) 
	{
		clearValue(&result);
		return err;
	}
	if (pc != runline->num_tokens - 1) return ERR_SYNTAX;

	if (!trueValue(&result)) 
	{
		/* Jump past ELSE if it exists, otherwise FI/FIALL */
		if (runline->else_jump)
			setNewRunLine(runline->else_jump->next);
		else
			setNewRunLine(runline->jump->next);
	}
	clearValue(&result);
	return OK;
}




int comElse(int comnum, st_runline *runline)
{
	/* If jump pointer not set then find our matching FI */
	if (!runline->jump && !setBlockEnd(runline,COM_ELSE,COM_FI))
		return ERR_MISSING_FI;

	setNewRunLine(runline->jump ? runline->jump->next : NULL);
	return OK;
}




/*** GOTO & GOSUB, VGOSUB. Using a fixed line number is much faster otherwise
     the expression must be evaluated ***/
int comGotoGosub(int comnum, st_runline *runline)
{
	st_progline *progline;
	st_value result;
	int err;
	int pc;

	progline = NULL;

	switch(comnum)
	{
	case COM_GOSUB:
		if (return_stack_cnt == MAX_RETURN_STACK)
			return ERR_MAX_RECURSION;
		/* Fall through */

	case COM_GOTO:
		/* Check for direct line number, not an expression */
		if (runline->num_tokens == 2 && 
		    IS_NUM(&runline->tokens[1]) && !runline->jump)
		{
			if (runline->tokens[1].dval < 1)
				return ERR_INVALID_LINENUM;
			if (!(progline = getProgLine(runline->tokens[1].dval)))
				return ERR_NO_SUCH_LINE;
			runline->jump = progline->first_runline;
			break;
		}

		/* Get line number from expression */
		initValue(&result);
		pc = 1;
		if ((err = getRunLineValue(runline,&pc,VAL_NUM,TRUE,&result)) != OK)
			return err;

		if (result.dval < 1) return ERR_INVALID_LINENUM;
		if (!(progline = getProgLine(result.dval)))
			return ERR_NO_SUCH_LINE;
		runline->jump = progline->first_runline;
		clearValue(&result);
		break;

	default:
		assert(0);
	}

	/* Its set to a runline not a basline because we could have more 
	   runlines following in the program line that need to be executed 
	   when we return. eg: 10 GOSUB 50: print "DONE" */
	if (comnum == COM_GOSUB)
		return_stack[return_stack_cnt++] = runline->next;

	setNewRunLine(runline->jump);
	return OK;
}




int comReturn(int comnum, st_runline *runline)
{
	if (!return_stack_cnt) return ERR_UNEXPECTED_RETURN;
	setNewRunLine(return_stack[--return_stack_cnt]);
	return OK;
}




int comChoose(int comnum, st_runline *runline)
{
	st_value result;
	st_value case_result;
	st_runline *rl;
	st_runline *default_rl;
	st_runline *prev_case;
	int err;
	int pc;
	int nest;

	initValue(&result);
	initValue(&case_result);

	/* If jump pointer not set then find our matching CHOSEN */
	if (!runline->jump && !setBlockEnd(runline,COM_CHOOSE,COM_CHOSEN))
		return ERR_MISSING_CHOSEN;

	pc = 1;
	if ((err = getRunLineValue(runline,&pc,VAL_UNDEF,TRUE,&result)) != OK)
		return err;

	/* Find matching CASE or DEFAULT */
	err = OK;
	default_rl = NULL;
	prev_case = NULL;

	for(rl=runline->next,nest=0;rl != runline->jump;)
	{
		if (!IS_COM(&rl->tokens[0]))
		{
			rl = rl->next;
			continue;
		}

		switch(rl->tokens[0].subtype)
		{
		case COM_CHOOSE:
			++nest;
			break;

		case COM_CHOSEN:
			--nest;
			/* If it goes below zero then runline->jump isn't set
			   correctly */
			assert(nest >= 0);
			break;

		case COM_CASE:
			if (nest)
			{
				if (rl->next_case)
				{
					rl = rl->next_case;
					continue;
				}
				break;
			}
			pc = 1;
			if (prev_case && !prev_case->next_case)
				prev_case->next_case = rl;

			prev_case = rl;
			if ((err = getRunLineValue(
				rl,&pc,VAL_UNDEF,TRUE,&case_result)) != OK)
			{
				goto DONE;
			}
			if (case_result.type == result.type &&
			    ((case_result.type == VAL_NUM && 
			      case_result.dval == result.dval) ||
			     (case_result.type == VAL_STR &&
			      !strcmp(result.sval,case_result.sval))))
			{
				setNewRunLine(rl);
				goto DONE;
			}
			if (rl->next_case)
			{
				rl = rl->next_case;
				continue;
			}
			break;

		case COM_DEFAULT:
			if (prev_case && !prev_case->next_case)
				prev_case->next_case = rl;
			prev_case = rl;
			default_rl = rl;

			/* Because DEFAULT doesn't have to be last */
			if (rl->next_case)
			{
				rl = rl->next_case;
				continue;
			}
			break;
		}
		rl = rl->next;
	}

	/* If DEFAULT jump to it else jump to CHOSEN */
	if (default_rl)
		setNewRunLine(default_rl);
	else
		setNewRunLine(runline->jump);

	DONE:
	clearValue(&result);
	clearValue(&case_result);
	return err;
}




int comChosen(int comnum, st_runline *runline)
{
	return runline->jump ? OK : ERR_UNEXPECTED_CHOSEN;
}


/********************************** LOOPS ************************************/

int comWhile(int comnum, st_runline *runline)
{
	st_value result;
	int err;
	int pc;

	/* If jump pointer not set then find our matching WEND */
	if (!runline->jump && !setBlockEnd(runline,COM_WHILE,COM_WEND))
		return ERR_MISSING_WEND;

	pc = 1;
	initValue(&result);
	if ((err = evalExpression(runline,&pc,&result)) != OK) 
	{
		clearValue(&result);
		return err;
	}
	if (pc != runline->num_tokens)
	{
		clearValue(&result);
		return ERR_SYNTAX;
	}

	/* Not empty string or zero jump past WEND */
	if (!trueValue(&result)) setNewRunLine(runline->jump->next);

	clearValue(&result);
	return OK;
}




int comWend(int comnum, st_runline *runline)
{
	if (!runline->jump) return ERR_UNEXPECTED_WEND;
	setNewRunLine(runline->jump);
	return OK;
}




int comRepeat(int comnum, st_runline *runline)
{
	/* If jump pointer not set then find our matching UNTIL */
	if (!runline->jump && !setBlockEnd(runline,COM_REPEAT,COM_UNTIL))
		return ERR_MISSING_UNTIL;
	return OK;
}




int comUntil(int comnum, st_runline *runline)
{
	st_value result;
	int pc;
	int err;

	if (!runline->jump) return ERR_UNEXPECTED_UNTIL;

	/* Evaluate expression. If its zero or an empty string then jump back
	   to the REPEAT */
	pc = 1;
	initValue(&result);
	if ((err = evalExpression(runline,&pc,&result)) != OK) 
	{
		clearValue(&result);
		return err;
	}
	if (pc != runline->num_tokens)
	{
		clearValue(&result);
		return ERR_SYNTAX;
	}
	if (!trueValue(&result)) setNewRunLine(runline->jump);

	return OK;
}




/*** Format is: FOR <var> = <expr from> TO <expr to> [STEP <step expr>] */
int comFor(int comnum, st_runline *runline)
{
	st_token *token;
	st_value result;
	st_value from;
	double to;
	double step;
	double val;
	int pc;
	int err;
	int diff;

	token = &runline->tokens[1];

	/* Always get variable at start in case its been deleted during loop */
	if ((err = getOrCreateTokenVariable(token)) != OK) return err;

	/* If already looping do check, update var etc */
	if (runline->for_loop && runline->for_loop->looped)
	{
		assert(token->var);
		assert(runline->jump);

		/* Set this in case we jump out of the loop before the end. 
		   NEXT resets it to TRUE if we don't */
		runline->for_loop->looped = FALSE;

		val = token->var->value[0].dval + runline->for_loop->step;

		/* See if we're done */
		if ((runline->for_loop->step < 0 && val < runline->for_loop->to) ||
		    (runline->for_loop->step > 0 && val > runline->for_loop->to))
		{
			setNewRunLine(runline->jump->next);
			return OK;
		}

		/* Update variable value directly since we know its not an 
		   array or map */
		setValue(&token->var->value[0],VAL_NUM,NULL,val);

		return OK;
	}

	initValue(&result);

	/* Get FROM value */
	initValue(&from);
	pc = 3;
	if ((err = getRunLineValue(runline,&pc,VAL_NUM,FALSE,&from)) != OK)
		return err;

	if (!IS_COM_TYPE(&runline->tokens[pc],COM_TO))
	{
		err = ERR_SYNTAX;
		goto ERROR;
	}
	
	/* Get TO value */
	initValue(&result);
	++pc;
	if ((err = getRunLineValue(runline,&pc,VAL_NUM,EITHER,&result)) != OK)
		return err;
	to = result.dval;

	/* Get optional STEP value */
	if (pc < runline->num_tokens)
	{
		if (pc == runline->num_tokens - 1 || 
		    !IS_COM_TYPE(&runline->tokens[pc],COM_STEP))
		{
			err = ERR_SYNTAX;
			goto ERROR;
		}
		++pc;
		if ((err = getRunLineValue(runline,&pc,VAL_NUM,TRUE,&result)) != OK)
			return err;

		step = result.dval;
	}
	else step = 1;

	/* Set jump locations */
	if (!setBlockEnd(runline,COM_FOR,COM_NEXT))
	{
		err = ERR_MISSING_NEXT;
		goto ERROR;
	}
	
	/* Create loop struct if not already there */
	if (!runline->for_loop)
	{
		runline->for_loop = (st_for_loop *)malloc(sizeof(st_for_loop));
		assert(runline->for_loop);
	}
	runline->for_loop->looped = FALSE;
	runline->for_loop->from = from.dval;
	runline->for_loop->to = to;
	runline->for_loop->step = step;

	if ((err = setVarValue(token->var,NULL,NULL,&from,FALSE)) != OK)
		goto ERROR;

	/* See if stepping direction doesn't match loop dir */
	diff = runline->for_loop->to - runline->for_loop->from;

	if (diff && runline->for_loop->step &&
	    SGN(runline->for_loop->step) != SGN(diff))
	{
		/* Jump to line following NEXT */
		setNewRunLine(runline->jump->next);
	}
	err = OK;
	/* Fall through */

	ERROR:
	clearValue(&result);
	return err;
}




/*** NEXT is on its own. No point having the variable too since its unnecessary
     as there's no need to interleave these loops ***/
int comNext(int comnum, st_runline *runline)
{
	if (!runline->jump) return ERR_UNEXPECTED_NEXT;
	assert(runline->jump->for_loop);

	runline->jump->for_loop->looped = TRUE;
	setNewRunLine(runline->jump);
	return OK;
}




/*** Format is: FOREACH k,v IN m ***/
int comForEach(int comnum, st_runline *runline)
{
	st_foreach_loop *fe_loop;
	st_token *token;
	st_var *key;
	st_var *value;
	st_keyval *kv;
	int err;
	int i;

	/* Always get variables in case they've been deleted during the
	   loop. If the variable doesn't exist and not in strict mode then
	   create it. */
	for(i=1;i <= 5;i+=2)
	{	
		token = &runline->tokens[i];
		if (!token->var && (err = getOrCreateTokenVariable(token)))
			return err;
	}

	/* Last one must be a map */
	if (token->var->type != VAR_MAP) return ERR_VAR_IS_NOT_MAP;

	key = runline->tokens[1].var;
	value = runline->tokens[3].var;
	
	/* If first entry into loop or haven't come from a NEXTEACH then set 
	   everything up */
	if (!(fe_loop = runline->foreach_loop))
	{
		/* Set jump locations */
		if (!setBlockEnd(runline,COM_FOREACH,COM_NEXTEACH))
			return ERR_MISSING_NEXTEACH;
		
		fe_loop = runline->foreach_loop = (st_foreach_loop *)malloc(
			sizeof(st_foreach_loop));
		assert(fe_loop);
		bzero(fe_loop,sizeof(st_foreach_loop));
	}
	else if (!fe_loop->looped)
	{
		fe_loop->map_index = 0;
		fe_loop->map_pos = 0;
	}

	/* Find map key and value. Can't keep a pointer to the map object
	   itself because it may also be deleted during the loop. This is
	   slow but safe. */
	SEARCH:
	for(i=0,kv=token->var->first_keyval[fe_loop->map_index];
	    i < fe_loop->map_pos && kv;++i,kv=kv->next);

	/* Set variables if not end of loop. Can set them direct since we know
	   they're not arrays or maps themselves */
	if (kv)
	{
		setValue(&key->value[0],VAL_STR,kv->key,0);
		copyValue(&value->value[0],&kv->value);
		++fe_loop->map_pos;
	}
	else
	{
		if (++fe_loop->map_index > MAX_UCHAR)
		{
			setNewRunLine(runline->jump->next);
			fe_loop->looped = FALSE;
			return OK;
		}
		fe_loop->map_pos = 0;
		goto SEARCH;
	}

	return OK;
}




int comNextEach(int comnum, st_runline *runline)
{
	if (!runline->jump) return ERR_UNEXPECTED_NEXTEACH;
	assert(runline->jump->foreach_loop);

	runline->jump->foreach_loop->looped = TRUE;
	setNewRunLine(runline->jump);
	return OK;
}




/*** This co-opts the for-loop structure - "to" is the value to count up to
     and "from" is the current count. "step" is not used since its always 1 ***/
int comLoop(int comnum, st_runline *runline)
{
	st_value count;
	int pc;
	int err;

	if (!runline->for_loop || !runline->for_loop->looped)
	{
		/* Get count */
		initValue(&count);
		pc = 1;
		if ((err = getRunLineValue(runline,&pc,VAL_NUM,TRUE,&count)) != OK)
			return err;

		if (!setBlockEnd(runline,COM_LOOP,COM_LEND))
			return ERR_MISSING_LEND;

		if (!runline->for_loop)
		{
			runline->for_loop = (st_for_loop *)malloc(sizeof(st_for_loop));
			assert(runline->for_loop);
		}
		runline->for_loop->from = 0;
		runline->for_loop->to = count.dval;
		runline->for_loop->step = 0;
	}

	if (runline->for_loop->from == runline->for_loop->to)
		setNewRunLine(runline->jump->next);
	else
		++runline->for_loop->from;

	/* Set in case we jump out of loop before the end */
	runline->for_loop->looped = FALSE;
	return OK;
}




int comLend(int comnum, st_runline *runline)
{
	if (!runline->jump) return ERR_UNEXPECTED_LEND;
	assert(runline->jump->for_loop);

	runline->jump->for_loop->looped = TRUE;
	setNewRunLine(runline->jump);
	return OK;
}




/*** Jump out of CHOOSE-CHOSEN and loop blocks ***/
int comBreak(int comnum, st_runline *runline)
{
	if (runline->jump)
	{
		setNewRunLine(runline->jump);
		return OK;
	}
	return ERR_UNEXPECTED_BREAK;
}




int comContLoop(int comnum, st_runline *runline)
{
	if (runline->jump)
	{
		if (runline->jump->for_loop)
			runline->jump->for_loop->looped = TRUE;
		setNewRunLine(runline->jump);
		return OK;
	}
	return ERR_UNEXPECTED_CONTLOOP;
}


/******************************** DATA commands *****************************/

/*** Read data from a DATA line. Nothing to do with I/O ***/
int comRead(int comnum, st_runline *runline)
{
	st_runline *orig_data_runline;
	st_token *token;
	st_value result;
	st_value icnt_or_key;
	int index[MAX_INDEXES];
	int err;
	int pc;
	int create_var;
	int new_data_line;
	bool autorest;

	if (!data_runline) return ERR_UNEXPECTED_READ;

	initValue(&result);
	initValue(&icnt_or_key);

	/* Go through variables to have data read into them */
	for(pc=1;pc < runline->num_tokens;++pc)
	{
		token = &runline->tokens[pc];
		if (!IS_VAR(token)) return ERR_SYNTAX;
		if (!token->var && !(token->var = getVariable(token->str)))
		{
			if (strict_mode) return ERR_UNDEFINED_VAR_FUNC;
			create_var = 1;
		}
		else create_var = 0;

		/* Get index(es). eg: a(1,2,3) */
		if (++pc < runline->num_tokens && 
		    IS_OP_TYPE(&runline->tokens[pc],OP_L_BRACKET))
		{
			if ((err = getVarIndexes(runline,&pc,&icnt_or_key,index)) != OK)
				goto ERROR;
		}
		else setValue(&icnt_or_key,VAL_NUM,NULL,0);

		if (create_var)
		{
			if (!validVariableName(token->str)) 
				return ERR_INVALID_VAR_NAME;
			token->var = createVariable(
				token->str,VAR_STD,(int)icnt_or_key.dval,index);
		}

		/* If we're off the end of our current DATA line or its empty
		   then find a new one */
		new_data_line = 0;
		LOOP:
		for(autorest=FALSE;data_pc >= data_runline->num_tokens;)
		{
			/* Find the next DATA statement */
			if (data_runline->jump)
			{
				data_runline = data_runline->jump;
				new_data_line = 1;
				data_pc = 1;
				continue;
			}

			/* Jump pointer not set so go through each line until
			   we hit the next DATA or the end */
			orig_data_runline = data_runline;
			for(data_runline = data_runline->next;
			    data_runline;data_runline=data_runline->next)
			{
				if (IS_COM_TYPE(&data_runline->tokens[0],COM_DATA))
				{
					orig_data_runline->jump = data_runline;
					new_data_line = 1;
					data_pc = 1;
					goto LOOP;
				}
			}

			/* No DATA line. If we have autorestore then do it
			   otherwise error */
			if (data_autorestore_runline)
			{
				/* Prevents endless loop with empty DATA 
				   lines */
				if (autorest) return ERR_DATA_EXHAUSTED;
				data_runline = data_autorestore_runline;
				new_data_line = 1;
				autorest = TRUE;
			}
			else return ERR_DATA_EXHAUSTED;

			data_pc = 1;
		}
		if (new_data_line)
		{
			setValue(
				data_line_var->value,
				VAL_NUM,NULL,data_runline->parent->linenum);
		}

		/* Evaluated DATA expression */
		if ((err = evalExpression(data_runline,&data_pc,&result)) != OK)
			goto ERROR;

		if (data_pc < data_runline->num_tokens && 
		    !IS_OP_TYPE(&data_runline->tokens[data_pc],OP_COMMA))
		{
			err = ERR_SYNTAX;
			goto ERROR;
		}
		++data_pc;

		/* Set variable value */
		if ((err = setVarValue(
			token->var,&icnt_or_key,index,&result,FALSE)) != OK)
		{
			goto ERROR;
		}
		if (pc < runline->num_tokens && 
		    (pc == runline->num_tokens-1 || NOT_COMMA(pc)))
		{
			err = ERR_SYNTAX;
			goto ERROR;
		}
	}
	err = OK;
	/* Fall through */

	ERROR:
	clearValue(&result);
	clearValue(&icnt_or_key);
	return err;
}




int comRestore(int comnum, st_runline *runline)
{
	st_value result;
	st_progline *pl;
	int err;
	int linenum;
	int pc = 1;
	
	/* Get line number */
	initValue(&result);
	if ((err = getRunLineValue(runline,&pc,VAL_NUM,TRUE,&result)) != OK)
		return err;
		
	linenum = result.dval;
	clearValue(&result);
	if (linenum < 1) return ERR_INVALID_ARG;
	if (!(pl = getProgLine(linenum))) return ERR_NO_SUCH_LINE;
	if (!IS_COM_TYPE(&pl->first_runline->tokens[0],COM_DATA))
		return ERR_CANT_RESTORE;

	data_runline = pl->first_runline;
	data_pc = 1;
	data_autorestore_runline = (comnum == COM_AUTORESTORE ? data_runline : NULL);
	setValue(data_line_var->value,VAL_NUM,NULL,linenum);
	return OK;
}




/*** Exit the interpreter back to the shell ***/
int comExit(int comnum, st_runline *runline)
{
	st_value result;
	int code;
	int err;
	int pc;

	if (runline->num_tokens > 1)
	{
		initValue(&result);
		pc = 1;
		if ((err = getRunLineValue(runline,&pc,VAL_NUM,TRUE,&result)) != OK)
			return err;

		if (result.dval < 0)
		{
			clearValue(&result);
			return ERR_INVALID_ARG;
		}
		code = result.dval;
	}
	else code = 0;

	doExit(code);
	return OK;
}




int comStop(int comnum, st_runline *runline)
{
	if (runline->parent->linenum)
	{
		if (flags.child_process)
		{
			printf("*** STOP in line %d, pid %u ***\n",
				runline->parent->linenum,getpid());
		}
		else printf("*** STOP in line %d ***\n",runline->parent->linenum);
		interrupted_runline = runline;
	}
	setNewRunLine(NULL);
	return OK;
}




/*** Restart from last BREAKed location ***/
int comCont(int comnum, st_runline *runline)
{
	if (interrupted_runline)
	{
		setNewRunLine(interrupted_runline->next);
		interrupted_runline = NULL;
		return OK;
	}
	return ERR_CANT_CONTINUE;
}



/*********************************** I/O ***********************************/

/*** LOAD, CHAIN and MERGE commands ***/
int comLoad(int comnum, st_runline *runline)
{
	st_value result;
	int linenum;
	int err;
	int pc = 1;

	if ((err = getRunLineValue(
		runline,
		&pc,VAL_STR,comnum == COM_CHAIN ? EITHER : TRUE,&result)) != OK)
	{
		return err;
	}

	linenum = runline->parent->linenum;

	if (comnum != COM_MERGE)
		err = loadProgram(result.sval,0,TRUE);
	else
		err = loadProgram(result.sval,linenum,FALSE);

	if (err == OK)
	{
		/* If we were in a running program reset to run new code from
		   the start */
		if (comnum != COM_MERGE && linenum)
			resetProgram();
		else if (comnum != COM_CHAIN)
			ready();
	}

	clearValue(&result);

	if (comnum == COM_CHAIN && err == OK)
	{
		puts("RUNNING...");
		return doComRun(COM_CHAIN,runline,pc);
	}
	return err;
}




int comSave(int comnum, st_runline *runline)
{
	FILE *fp;
	st_value result;
	char path[PATH_MAX+1];
	char *fullpath;
	char *filename;
	char *ptr;
	char *tmp = NULL;
	int err;
	int pc = 1;

	if (!prog_first_line) return ERR_NOTHING_TO_SAVE;

	if ((err = getRunLineValue(runline,&pc,VAL_STR,TRUE,&result)) != OK)
		return err;

	/* If we have a path rather than just a filename then match any 
	   wildcards in the directory name */
	if ((ptr = strrchr(result.sval,'/')) && ptr > result.sval)
	{
		*ptr = 0;

		/* path doesn't need to be initialised */
		if ((err = matchPath(S_IFDIR,result.sval,path,TRUE)) != OK)
		{
			clearValue(&result);
			return err;
		}
		*ptr = '/';
		assert(asprintf(&fullpath,"%s%s",path,ptr) != -1);
	}
	else fullpath = result.sval;

	/* Now match any wildcards in the filename */
	if (hasWildCards(fullpath))
	{
		if ((err = matchPath(S_IFREG,fullpath,path,TRUE)) == OK)
		{
			if (fullpath != result.sval) free(fullpath);
			assert((fullpath = strdup(path)));
		}

		/* Make sure non left */
		if (hasWildCards(fullpath))
		{
			err = ERR_INVALID_PATH;
			goto ERROR;
		}
	}
		
	if ((tmp = addFileExtension(fullpath))) 
		filename = tmp;
	else
		filename = fullpath;

	if (!(fp = fopen(filename,"w")))
	{
		err = ERR_CANT_OPEN_FILE;
		goto ERROR;
	}
	printf("Saving \"%s\": ",filename);
	fflush(stdout);

	err = listProgram(fp,0,0,FALSE);
	fclose(fp);
	if (err == OK) ready();
	/* Fall through */

	ERROR:
	if (fullpath != result.sval) free(fullpath);
	clearValue(&result);
	FREE(tmp);
	return err;
}




/*** All the *DIR* commands ***/
int comDir(int comnum, st_runline *runline)
{
	st_value pattern;
	DIR *dir;
	struct dirent *de;
	struct stat fs;
	bool pause;
	bool show_links;
	int err;
	int pc;
	int len;
	int bas_cnt;
	int file_cnt;
	int link_cnt;
	int other_cnt;
	int dir_cnt;
	int bytes;
	int kbytes;
	int num_cols;
	int col_cnt;
	int pause_linecnt;
	char dirname[PATH_MAX+1];
	char path[PATH_MAX*2+10];
	char linkpath[PATH_MAX+1];
	char *link;
	size_t basic_bytes;
	size_t total_bytes;

	err = OK;
	initValue(&pattern);

	if (runline->num_tokens == 1) strcpy(dirname,".");
	else
	{
		pc = 1;
		if ((err = getRunLineValue(runline,&pc,VAL_STR,TRUE,&pattern)) != OK)
			return err;
		if ((err = matchPath(S_IFDIR,pattern.sval,dirname,TRUE)) != OK)
		{
			clearValue(&pattern);
			return err;
		}
	}

	if (!(dir = opendir(dirname)))
	{
		err = ERR_CANT_OPEN_DIR;
		goto DONE;
	}
	if (strlen(dirname) > PATH_MAX - 2)
	{
		err = ERR_INVALID_ARG;
		goto DONE;
	}

	printf("\nDIRECTORY: %s\n\n",dirname);

	bas_cnt = 0;
	file_cnt = 0;
	dir_cnt = 0;
	link_cnt = 0;
	other_cnt = 0;
	basic_bytes = 0;
	total_bytes = 0;
	pause_linecnt = 0;
	pause = (comnum == COM_PDIR || comnum == COM_PDIRL);
	show_links = (comnum == COM_DIRL || comnum == COM_PDIRL);

	num_cols = (term_cols < 40) ? 1 : (term_cols + 7) / 40;
	col_cnt = 0;

	while((de = readdir(dir)))
	{
		if (pause && pause_linecnt == term_rows - 3)
		{
			pressAnyKey("Press any key to page: ");
			pause_linecnt = 0;
		}
		if (last_signal == SIGINT) goto DONE;
		
		len = strlen(de->d_name);
		
		snprintf(path,PATH_MAX,"%s/%s",dirname,de->d_name);
		if (lstat(path,&fs) == -1)
		{
			err = ERR_READ;
			goto DONE;
		}

		switch(fs.st_mode & S_IFMT)
		{
		case S_IFREG:
			if (len > 4 && !strcasecmp(de->d_name+len-4,".bas"))
			{
				++bas_cnt;
				basic_bytes += fs.st_size;
			}

			/* %f doesn't allow length formatting so do it as 
			   integers */
			kbytes = fs.st_size / kilobyte;
			bytes = (fs.st_size % kilobyte) / 100;
			len = printf("%-25s  %3d.%dK",
				de->d_name,kbytes,bytes);
			total_bytes += fs.st_size;
			++file_cnt;
			break;

		case S_IFLNK:
			if (show_links)
			{
				/* Find out what link is pointing at */
				if ((len = readlink(path,linkpath,PATH_MAX)) == -1)
					link = "?";
				else
				{
					linkpath[len] = 0;
					link = linkpath;
				}
				sprintf(path,"%s -> %s",de->d_name,link);
			}
			else sprintf(path,"%s@",de->d_name);

			len = printf("%-33s",path);
			++link_cnt;
			break;

		case S_IFDIR:
			if (!strcmp(de->d_name,".") || !strcmp(de->d_name,".."))
				continue;
			sprintf(path,"%s/",de->d_name);
			len = printf("%-33s",path);
			++dir_cnt;
			break;

		case S_IFSOCK:
			sprintf(path,"%s=",de->d_name);
			len = printf("%-33s",path);
			++other_cnt;
			break;

		default:
			len = printf("%-33s",de->d_name);
			++other_cnt;
		}

		if (len > 33 || ++col_cnt == num_cols)
		{
			putchar('\n');
			col_cnt = 0;
			++pause_linecnt;
		}
		else printf("       ");
	}

	if (col_cnt) putchar('\n');

	if (pause && pause_linecnt >= term_rows - 7)
		pressAnyKey("Press any key to page: ");

	if (last_signal != SIGINT)
	{
		printf("\nTotal files: %-2d (%.1fK)\nBASIC files: %-2d (%.1fK)\n",
			file_cnt,(float)total_bytes / kilobyte,
			bas_cnt,(float)basic_bytes / kilobyte);
		printf("Symlinks   : %d\nDirectories: %d\nOther      : %d\n\n",
			link_cnt,dir_cnt,other_cnt);
	}

	DONE:
	closedir(dir);
	clearValue(&pattern);
	return err;
}




/*** Delete a file. Complements the REMOVE() function as this is easier to
     use at the command line. Also it always adds .bas onto the end of any
     filename given ***/
int comRm(int comnum, st_runline *runline)
{
	st_value pattern;
	char filename[PATH_MAX+1];
	char *fullpath;
	char *tmp;
	int pc;
	int err;
	int ret;

	initValue(&pattern);
	for(pc=1;pc < runline->num_tokens;++pc)
	{
		if ((err = getRunLineValue(runline,&pc,VAL_STR,EITHER,&pattern)) != OK)
			return err;

		/* Do this before matching */
		if ((tmp = addFileExtension(pattern.sval)))
			fullpath = tmp;
		else
			fullpath = pattern.sval;

		if ((err = matchPath(S_IFREG,fullpath,filename,TRUE)) != OK)
		{
			clearValue(&pattern);
			return err;
		}

		printf("DELETING: %s\n",filename);
		ret = unlink(filename);
		clearValue(&pattern);
		FREE(tmp);
		if (ret == -1) return ERR_CANT_DEL_FILE;

		if (pc == runline->num_tokens) return OK;
		if (NOT_COMMA(pc)) break;
	}
	return ERR_SYNTAX;
}




int comPrint(int comnum, st_runline *runline)
{
	FILE *pfp;
	st_value result;
	int pc;
	int diff;
	int err;
	int snum;
	int fd;
	int ret;
	char *numstr;

	initValue(&result);
	pfp = NULL;

	/* Check for a stream number */
	if (runline->num_tokens > 2 && IS_OP_TYPE(&runline->tokens[1],OP_HASH))
	{
		pc = 2;

		if ((err = getRunLineValue(
			runline,&pc,VAL_NUM,FALSE,&result)) != OK)
		{
			return err;
		}
		if ((snum = (int)result.dval) >= MAX_STREAMS)
			return ERR_DIR_STREAM;

		switch(--snum)
		{
		case STREAM_STDIO:
			/* #0 = stdout. With INPUT its stdin */
			fd = STDOUT;
			break;
		case STREAM_PRINTER:
			if (!(pfp = popen("lp > /dev/null","w")))
				return ERR_LP;
			fd = -1;
			break;
		default:
			CHECK_STREAM(snum)
			fd = stream[snum];
		}
		if (pc < runline->num_tokens && NOT_COMMA(pc))
			return ERR_SYNTAX;
		++pc;
	}
	else
	{
		/* Default to stdout */
		fd = STDOUT;
		pc = 1;
	}

	/* Go through data to print */
	for (;pc < runline->num_tokens;++pc)
	{
		if ((err = evalExpression(runline,&pc,&result)) != OK)
		{
			clearValue(&result);
			goto ERROR;
		}
		if (result.type == VAL_STR)
		{
			if (pfp)
				ret = fwrite(result.sval,strlen(result.sval),1,pfp);
			else
				ret = write(fd,result.sval,strlen(result.sval));
		}
		else
		{
			/* If we have fractional bit print as float else int */
			if (IS_FLOAT(result.dval))
				ret = asprintf(&numstr,"%f",result.dval);
			else
				ret = asprintf(&numstr,"%ld",(long)result.dval);
			assert(ret != -1);
			if (pfp)
				ret = fwrite(numstr,strlen(numstr),1,pfp);
			else
				ret = write(fd,numstr,strlen(numstr));
			free(numstr);
		}
		clearValue(&result);
		if (ret == -1) 
		{
			err = ERR_WRITE;
			goto ERROR;
		}

		diff = runline->num_tokens - pc;
		if (diff == 1)
		{
			if (IS_OP_TYPE(&runline->tokens[pc],OP_SEMI_COLON))
				return OK;

			err = ERR_SYNTAX;
			goto ERROR;
		}
		if (diff > 1 && NOT_COMMA(pc))
		{
			err = ERR_SYNTAX;
			goto ERROR;
		}
	}
	if (pfp)
	{
		fwrite("\n",1,1,pfp);
		err = OK;
	}
	else err = ((write(fd,"\n",1) == -1) ? ERR_WRITE : OK);

	ERROR:
	if (pfp) pclose(pfp);
	return err;
}




/*** Input a whole line or single character. The single character option also
     has a timeout. Too many gotos in this function. Oh well. ***/
int comInput(int comnum, st_runline *runline)
{
	struct dirent *de;
	st_value icnt_or_key;
	st_value result;
	st_token *token;
	st_keybline kbline;
	struct timeval tv;
	struct timeval *tvptr;
	fd_set mask;
	char esc_str[MAX_ESC_SEQ_LEN+1];
	char cstr[2];
	char *str;
	char c;
	int pc;
	int index[MAX_INDEXES];
	int esc_cnt;
	int seq_num;
	int mod_index;
	/* Pre initialisations to prevent gcc warnings */
	int snum = -1;
	int fd = STDIN;
	int err = OK;
	bool dir_read = FALSE;
	bool set_value = FALSE;
	bool insert = TRUE;
	double sleep_time = 0;

	setValue(eof_var->value,VAL_NUM,NULL,0);
	setValue(interrupted_var->value,VAL_NUM,NULL,0);

	/* Check for a stream number */
	if (IS_OP_TYPE(&runline->tokens[1],OP_HASH))
	{
		pc = 2;
		if ((err = getRunLineValue(
			runline,&pc,VAL_NUM,FALSE,&result)) != OK)
		{
			clearValue(&result);
			return err;
		}

		/* For a directory read do it immediately */
		if (result.dval >= MAX_STREAMS)
		{
			if (comnum == COM_CINPUT) return ERR_DIR_CINPUT;
			snum = result.dval - MAX_STREAMS - 1;
			CHECK_DIR_STREAM(snum);
			dir_read = TRUE;
		}
		else 
		{
			snum = result.dval - 1;
			switch(snum)
			{
			case STREAM_STDIO:
				/* #0 = stdin. With PRINT its stdout */
				fd = STDIN;
				break;
			case STREAM_PRINTER:
				/* Can't read from the printer */
				return ERR_LP_INPUT;
			default:
				CHECK_STREAM(snum);
				fd = stream[snum];
			}
		}
		if (pc >= runline->num_tokens - 1 || NOT_COMMA(pc))
			return ERR_SYNTAX;
		++pc;
	}
	else pc = 1;

	/* Get variable */
	token = &runline->tokens[pc];
	if (!token->var && (err = getOrCreateTokenVariable(token))) return err;

	initValue(&result);
	initValue(&icnt_or_key);
	tvptr = NULL;
	bzero(&kbline,sizeof(kbline));

	if (comnum == COM_CINPUT)
	{
		cstr[0] = 0;
		str = cstr;
	}
	else str = kbline.str;

	/* Get index */
	if (runline->num_tokens > 2)
	{
		++pc;
		if (IS_OP_TYPE(&runline->tokens[pc],OP_L_BRACKET) &&
		    (err = getVarIndexes(runline,&pc,&icnt_or_key,index)) != OK)
		{
			goto DONE;
		}

		/* Get timeout val for CINPUT */
		if (pc < runline->num_tokens)
		{
			if (comnum == COM_INPUT || NOT_COMMA(pc))
			{
				err = ERR_SYNTAX;
				goto DONE;
			}
			++pc;
			if ((err = getRunLineValue(runline,&pc,VAL_NUM,TRUE,&result)) != OK)
				goto DONE;

			if (result.dval < 0)
			{
				err = ERR_INVALID_ARG;
				goto DONE;
			}

			/* Timeout is in floating point seconds - convert */
			sleep_time = result.dval;
			tv.tv_sec = (int)sleep_time;
			tv.tv_usec = (sleep_time - tv.tv_sec) * 1000000;
			tvptr = &tv;
		}
		if (pc < runline->num_tokens)
		{
			err = ERR_SYNTAX;
			goto DONE;
		}
	}

	set_value = TRUE;

	/* If its a directory read can't use select() or timeout. Do it now */
	if (dir_read)
	{
		if ((de = readdir(dir_stream[snum]))) str = strdup(de->d_name);
		else
		{
			str = strdup("");
			setValue(eof_var->value,VAL_NUM,NULL,1);
		}
		assert(str);
		goto DONE;
	}

	esc_cnt = 0;

	/* Wait for input then set variable */
	while(1)
	{
		FD_ZERO(&mask);
		FD_SET(fd,&mask);

		switch(select(FD_SETSIZE,&mask,0,0,tvptr))
		{
		case -1:
		case 0:
			goto DONE;
		default:
			switch(read(fd,&c,1))
			{
			case -1:
				err = ERR_READ;
				goto DONE;

			case 0:
				setValue(eof_var->value,VAL_NUM,NULL,1);
				goto DONE;

			default:
				break;
			}

			if (comnum == COM_CINPUT)
			{
				cstr[0] = c;
				cstr[1] = 0;
				goto DONE;
			}

			/* Some escape codes for the INPUT command are processed
			   differently to the BASIC command prompt hence some
			   duplicated code here. Also esc_cnt is only set when
			   reading from STDIN, not a file. */
			if (esc_cnt)
			{
				esc_str[esc_cnt++] = c;
				esc_str[esc_cnt] = 0;
				if ((seq_num = getEscapeSeq(esc_str+1,esc_cnt-1)) < 0)
				{
					if (seq_num == ESC_SEQ_INVALID)
						esc_cnt = 0;
					continue;
				}

				esc_cnt = 0;

				switch(seq_num)
				{
				case ESC_K:
				case ESC_J:
				case ESC_UP_ARROW:
				case ESC_DOWN_ARROW:
				case ESC_LEFT_ARROW:
					leftCursor(&kbline);
					break;

				case ESC_RIGHT_ARROW:
					rightCursor(&kbline);
					break;

				case ESC_DELETE:
					delCharFromKeyLine(&kbline,FALSE);
					break;

				case ESC_INSERT:
					insert = !insert;
					break;

				case ESC_PAGE_UP:
				case ESC_PAGE_DOWN:
					/* Do nothing, just ignore */
					break;

				case ESC_CON_F1:
				case ESC_CON_F2:
				case ESC_CON_F3:
				case ESC_CON_F4:
				case ESC_CON_F5:
					mod_index = 256 + (seq_num - ESC_CON_F1);
					if (defmod[mod_index])
					{
						addDefModStrToKeyLine(
							&kbline,mod_index,
							fd == STDIN,insert);
					}
					break;
				case ESC_TERM_F1:
				case ESC_TERM_F2:
				case ESC_TERM_F3:
				case ESC_TERM_F4:
				case ESC_TERM_F5:
					mod_index = 256 + (seq_num - ESC_TERM_F1);
					if (defmod[mod_index])
					{
						addDefModStrToKeyLine(
							&kbline,mod_index,
							fd == STDIN,insert);
					}
					break;
				default:
					assert(0);
				}
				continue;
			}

			switch(c)
			{
			case ESC:
				if (fd == STDIN)
				{
					esc_str[0] = ESC;
					esc_cnt = 1;
					continue;
				}
				addCharToKeyLine(&kbline,c,FALSE,insert);
				break;

			case '\n':
				err = OK;
				if (fd == STDIN) PRINT("\n",1);
				goto DONE;

			case DEL1:
			case DEL2:
				if (fd == STDIN)
					delCharFromKeyLine(&kbline,TRUE);
				else
					addCharToKeyLine(&kbline,c,FALSE,FALSE);
				break;

			default:
				if (defmod[(u_char)c])
					addDefModStrToKeyLine(&kbline,(u_char)c,fd == STDIN,insert);
				else
					addCharToKeyLine(&kbline,c,fd == STDIN,insert);
				/* kbline.str is realloced so reassign */
				str = kbline.str;
			}
		}
	}

	DONE:
	if (set_value)
	{
		/* Set variable with whatever we have in case we're been
		   interrupted by a signal */
		setValue(&result,VAL_STR,str,0);
		err = setVarValue(token->var,&icnt_or_key,index,&result,FALSE);
	}
	if (str != cstr && str) free(str);
	clearValue(&result);
	clearValue(&icnt_or_key);
	return err;
}




/*** Close a stream thats been created by the OPEN(), PIPE(),  LISTEN(), 
     ACCEPT() and CONNECT() functions. This is command rather than a function 
     because it doesn't return a value ***/
int comClose(int comnum, st_runline *runline)
{
	st_value result;
	int err;
	int pc;
	int snum;

	for(pc=1;pc < runline->num_tokens;++pc)
	{
		if ((err = getRunLineValue(
			runline,&pc,VAL_NUM,EITHER,&result)) != OK) return err;

		snum = result.dval - 1;
		CHECK_STREAM(snum);
		close(stream[snum]);
		stream[snum] = 0;

		/* If it was a process stream then close the pointer which will
		   kill the process */
		if (popen_fp[snum]) 
		{
			pclose(popen_fp[snum]);
			popen_fp[snum] = NULL;
		}

		if (pc == runline->num_tokens) return OK;
		if (NOT_COMMA(pc)) return ERR_SYNTAX;
	}
	return ERR_SYNTAX; 
}




/*** Close a directory stream created by OPENDIR() function ***/
int comCloseDir(int comnum, st_runline *runline)
{
	st_value result;
	int err;
	int pc;
	int snum;

	for(pc=1;pc < runline->num_tokens;++pc)
	{
		if ((err = getRunLineValue(runline,&pc,VAL_NUM,EITHER,&result)) != OK)
			return err;

		snum = result.dval - MAX_STREAMS - 1;
		CHECK_DIR_STREAM(snum);
		closedir(dir_stream[snum]);
		dir_stream[snum] = NULL;

		if (pc == runline->num_tokens) return OK;
		if (NOT_COMMA(pc)) return ERR_SYNTAX;
	}
	return ERR_SYNTAX; 
}


/******************************* TERM SCREEN *********************************/

int comCls(int comnum, st_runline *runline)
{
	/* Clear screen then move cursor to home position (top left) */
	PRINT("\033[2J\033[H",7);
	return OK;
}




int comLocatePlot(int comnum, st_runline *runline)
{
	st_value result;
	int pc;
	int err;
	int x;
	int y;
	int slen;
	char *str;

	pc = 1;	
	initValue(&result);
	if ((err = getRunLineValue(runline,&pc,VAL_NUM,FALSE,&result)) != OK)
		return err;

	if (NOT_COMMA(pc)) return ERR_SYNTAX;
	x = (int)result.dval;

	++pc;
	if ((err = getRunLineValue(runline,&pc,VAL_NUM,EITHER,&result)) != OK)
		return err;
	y = (int)result.dval;

	if (comnum == COM_LOCATE)
	{
		if (pc < runline->num_tokens) return ERR_SYNTAX;
		locate(x,y);
		return OK;
	}

	/* PLOT */
	if (pc < runline->num_tokens)
	{
		if (NOT_COMMA(pc)) return ERR_SYNTAX;
		++pc;
		if ((err = getRunLineValue(runline,&pc,VAL_STR,TRUE,&result)) != OK)
		{
			clearValue(&result);
			return err;
		}
		str = result.sval;
		slen = result.slen;
	}
	else
	{
		str = " ";
		slen = 1;
	}

	drawString(x,y,str,slen);
	clearValue(&result);
	return OK;
}




/*** Terminal attributes - eg blink, underline etc.
     See https://en.wikipedia.org/wiki/ANSI_escape_code ***/
int comAttr(int comnum, st_runline *runline)
{
	st_value result;
	int pc;
	int err;
	char ansi[10];

	pc = 1;
	initValue(&result);
	if ((err = getRunLineValue(runline,&pc,VAL_NUM,TRUE,&result)) != OK)
		return err;

	if (result.dval < 0) return ERR_INVALID_ARG;

	sprintf(ansi,"\033[%dm",(int)result.dval);
	PRINT(ansi,strlen(ansi));
	return OK;
}




/*** PEN & PAPER: Set the foreground and background colours. Either standard
     ansi terminal 8 colours or RGB in r,g,b format ***/
int comColour(int comnum, st_runline *runline)
{
	st_value result;
	int pc;
	int err;
	int red;
	int green;
	int blue;
	char ansi[30];

	/* Get colour or RED */
	pc = 1;
	initValue(&result);
	if ((err = getRunLineValue(runline,&pc,VAL_NUM,EITHER,&result)) != OK)
		return err;

	if (pc < runline->num_tokens)
	{
		/* RGB code */
		if (NOT_COMMA(pc)) return ERR_SYNTAX;
		red = (int)result.dval;

		++pc;
		if ((err = getRunLineValue(
			runline,&pc,VAL_NUM,FALSE,&result)) != OK) return err;
		if (NOT_COMMA(pc)) return ERR_SYNTAX;
		green = (int)result.dval;

		++pc;
		if ((err = getRunLineValue(
			runline,&pc,VAL_NUM,TRUE,&result)) != OK) return err;
		blue = (int)result.dval;

		if (red < 0 || red > 255 || 
		    green < 0 || green > 255 || blue < 0 || blue > 255)
			return ERR_INVALID_ARG;

		if (comnum == COM_PEN)
			sprintf(ansi,"\033[38;2;%d;%d;%dm",red,green,blue);
		else
			sprintf(ansi,"\033[48;2;%d;%d;%dm",red,green,blue);
	}
	else
	{
		/* Single colour code */
		if (result.dval > 7) return ERR_INVALID_ARG;

		if (comnum == COM_PEN)
			result.dval += 30;
		else
			result.dval += 40;
		sprintf(ansi,"\033[%dm",(int)result.dval);
	}
	PRINT(ansi,strlen(ansi));
	return OK;
}




/*** Scroll the screen up or down by the given number of lines ***/
int comScroll(int comnum, st_runline *runline)
{
	st_value result;
	int pc;
	int err;
	int i;
	int lines;
	char ansi[3];

	pc = 1;
	initValue(&result);
	if ((err = getRunLineValue(runline,&pc,VAL_NUM,TRUE,&result)) != OK)
		return err;

	ansi[0] = 27;
	ansi[1] = (result.dval < 0 ? 'M' : 'D');
	lines = abs((int)result.dval);
	for(i=0;i < lines;++i) PRINT(ansi,2);

	return OK;
}




int comCursor(int comnum, st_runline *runline)
{
	enum 
	{
		CUR_ON,
		CUR_OFF,
		CUR_UP,
		CUR_DOWN,
		CUR_FWD,
		CUR_BACK,
		CUR_SAVE,
		CUR_RSTR,

		NUM_CURSOR_COMS
	};
	struct st_cursor
	{
		char *com;
		char *seq;
	} cursor[NUM_CURSOR_COMS] =
	{
		{ "ON",  "\033[?25h" },
		{ "OFF", "\033[?25l" },
		{ "UP",  "\033[%dA" },
		{ "DOWN","\033[%dB" },
		{ "FWD", "\033[%dC" },
		{ "BACK","\033[%dD" },
		{ "SAVE","\033[s" },
		{ "RSTR","\033[u" },
	};
	st_value seqval;
	st_value cntval;
	char seqtext[20];
	int pc;
	int err;
	int cnt;
	int i;

	pc = 1;
	initValue(&seqval);
	if ((err = getRunLineValue(runline,&pc,VAL_STR,EITHER,&seqval)) != OK)
		return err;

	if (pc < runline->num_tokens)
	{
		if (NOT_COMMA(pc) || ++pc == runline->num_tokens)
		{
			err = ERR_SYNTAX;
			goto ERROR;
		}
		initValue(&cntval);
		if ((err = getRunLineValue(runline,&pc,VAL_NUM,TRUE,&cntval)) != OK)
			goto ERROR;
		if ((cnt = (int)cntval.dval) < 0)
		{
			err = ERR_INVALID_ARG;
			goto ERROR;
		}
	}
	else cnt = 1;

	err = ERR_INVALID_ARG;

	for(i=0;i < NUM_CURSOR_COMS;++i)
	{
		if (strcasecmp(seqval.sval,cursor[i].com)) continue;

		switch(i)
		{
		case CUR_ON:
		case CUR_OFF:
		case CUR_SAVE:
		case CUR_RSTR:
			PRINT(cursor[i].seq,strlen(cursor[i].seq));
			break;

		default:
			sprintf(seqtext,cursor[i].seq,cnt);
			PRINT(seqtext,strlen(seqtext));
		}
		err = OK;
		break;
	}

	ERROR:
	clearValue(&seqval);
	return err;
}




/*** Formats are:
	LINE x1,y1,x2,y2[,<character>] 
	RECT x,y,width,height,fill[,<character>]
             where x,y is the top left vertex.
 ***/
int comLineRect(int comnum, st_runline *runline)
{
	st_value result;
	int data[5];
	int i;
	int cnt;
	int pc = 1;
	int err = OK;
	int slen = 1;
	char *str = " ";

	err = OK;
	pc = 1;
	initValue(&result);
	cnt = (comnum == COM_RECT ? 5 : 4);

	/* Get x,y from -> to */
	for(i=0;i < cnt;++i)
	{
		if ((err = getRunLineValue(
			runline,
			&pc,VAL_NUM,i < (cnt-1) ? FALSE : EITHER,&result)) != OK)
		{
			goto DONE;
		}
		data[i] = result.dval;
		if (i < cnt-1)
		{
			if (NOT_COMMA(pc))
			{
				err = ERR_SYNTAX;
				goto DONE;
			}
			++pc;
		}
	}

	/* Get optional character */
	if (pc < runline->num_tokens)
	{
		if (NOT_COMMA(pc)) 
		{
			err = ERR_SYNTAX;
			goto DONE;
		}
		++pc;
		if ((err = getRunLineValue(
			runline,&pc,VAL_STR,TRUE,&result)) != OK)
		{
			goto DONE;
		}
		str = result.sval;
		slen = result.slen;
	}

	switch(comnum)
	{
	case COM_LINE:
		drawLine(data[0],data[1],data[2],data[3],str,slen);
		break;

	case COM_RECT:
		if (data[2] < 0 || data[3] < 0) return ERR_INVALID_ARG;
		drawRect(data[0],data[1],data[2],data[3],data[4],str,slen);
		break;

	default:
		assert(0);
	}

	DONE:
	clearValue(&result);
	return err;
}




/*** Formats are:
	CIRCLE x,y,radius,fill[,<fill character>]

        or optionally from version 1.8.1:

	CIRCLE x,y,x_radius,y_radius,fill[,<fill character>] 
 ***/
int comCircle(int comnum, st_runline *runline)
{
	st_value result;
	int data[5];
	int i;
	int x;
	int y;
	int x_radius;
	int y_radius;
	int fill;
	int pc = 1;
	int err = OK;
	int slen = 1;
	char *str = " ";

	initValue(&result);

	/* Arguments:
	   0 = x
	   1 = y
	   2 = radius/x_radius
	   3 = y_radius/fill
	   4 = fill/fill char
	   5 = fill char
	*/
	for(i=0;i < 6;++i,++pc)
	{
		if ((err = getRunLineValue(
			runline,&pc,
			i < 3 ? VAL_NUM : VAL_UNDEF,EITHER,&result)) != OK)
		{
			goto DONE;
		}
		if (i < 5)
			data[i] = (result.type == VAL_NUM ? result.dval : -1);

		if (pc >= runline->num_tokens) break;

		if (NOT_COMMA(pc)) 
		{
			err = ERR_SYNTAX;
			goto DONE;
		}
	}

	x = data[0];
	y = data[1];
	x_radius = data[2];

	switch(i)
	{
	case 3:
		/* args = x,y,radius,fill */
		y_radius = data[2];
		fill = data[3];
		break;
	case 4:
		if (result.type == VAL_STR)
		{
			/* args = x,y,radius,fill,fill char */
			y_radius = data[2];
			fill = data[3];
			str = result.str;
			slen = result.slen;
		}
		else
		{
			/* args = x,y,x_radius,y_radius,fill */
			y_radius = data[3];
			fill = data[4];
		}
		break;
	case 5:
		/* If its not a string type don't need to clear it */
		if (result.type != VAL_STR) return ERR_INVALID_ARG;

		/* args = x,y,x_radius,y_radius,fill,fill char */
		y_radius = data[3];
		fill = data[4];
		str = result.str;
		slen = result.slen;
		break;
	default:
		err = ERR_SYNTAX;
		goto DONE;
	}

	if (x_radius < 0 || y_radius < 0 || fill < 0)
		err = ERR_INVALID_ARG;
	else
		drawCircle(x,y,x_radius,y_radius,fill,str,slen);

	DONE:
	clearValue(&result);
	return err;
}


/********************************* KEYBOARD ********************************/

/*** Map a key to a string that will be displayed when the key is pressed ***/
int comDefMod(int comnum, st_runline *runline)
{
	st_value result;
	int pc;
	int err;
	int index;
	int cnt;

	if (runline->num_tokens == 1)
	{
		dumpDefMods();
		return OK;
	}

	err = ERR_SYNTAX;
	index = 0;
	initValue(&result);

	for(pc=cnt=1;pc < runline->num_tokens;++pc,++cnt)
	{
		if ((err = evalExpression(runline,&pc,&result)) != OK) break;

		if (result.type != VAL_STR)
		{
			err = ERR_INVALID_ARG;
			goto DONE;
		}
		err = ERR_SYNTAX;
		if (cnt == 1)
		{
			if (pc == runline->num_tokens || NOT_COMMA(pc)) break;

			switch(strlen(result.sval))
			{
			case 1:
				/* Ascii char */
				index = (int)result.sval[0];
				break;
			case 2:
				/* Function key */
				if (toupper(result.sval[0]) == 'F')
				{
					index = (int)result.sval[1] - '0';
					if (index >= 0 && index <= NUM_F_KEYS)
					{
						index += 255;
						break;
					}
				}
				/* Fall through */
			default:
				err = ERR_INVALID_ARG;
				goto DONE;
			}
		}
		else
		{
			addDefMod(index,result.sval);
			err = OK;
			break;
		}
	}
	if (pc < runline->num_tokens) err = ERR_SYNTAX;

	DONE:
	clearValue(&result);
	return err;
}




/*** Clear the defined key mappings ***/
int comClearDefMods(int comnum, st_runline *runline)
{
	if (runline->num_tokens != 1) return ERR_SYNTAX;
	clearDefMods();
	return OK;
}


/********************************* WATCHING ********************************/

int comWatch(int comnum, st_runline *runline)
{
	st_token *token;
	int ret;
	int pc;

	/* If no arguments print the current watched variables */
	if (runline->num_tokens == 1)
	{
		printWatchVars();
		return OK;
	}
	for(pc=1;pc < runline->num_tokens;++pc)
	{
		token = &runline->tokens[pc];
		if (!IS_VAR(token)) return ERR_INVALID_ARG;
		if ((ret = addWatchVar(token->str)) != OK)
			return ret;
		if (++pc == runline->num_tokens) return OK;
		if (NOT_COMMA(pc)) break;
	}
	return ERR_SYNTAX;
}




int comUnwatch(int comnum, st_runline *runline)
{
	st_token *token;
	int ret;
	int pc;

	/* If no arguments clear all variables */
	if (runline->num_tokens == 1)
	{
		clearWatchVars();
		puts("All watch variables cleared.");
		return OK;
	}
	for(pc=1;pc < runline->num_tokens;++pc)
	{
		token = &runline->tokens[pc];
		if (!IS_VAR(token)) return ERR_INVALID_ARG;
		if ((ret = removeWatchVar(token->str)) != OK)
			return ret;
		if (++pc == runline->num_tokens) return OK;
		if (NOT_COMMA(pc)) break;
	}
	return ERR_SYNTAX;
}


/************************************ MISC ************************************/

/*** Delete a key/value pair from a map ***/
int comDelKey(int comnum, st_runline *runline)
{
	st_value result;
	st_token *token;
	int pc;
	int err;

	token = &runline->tokens[1];
	if (!token->var && !(token->var = getVariable(token->str)))
		return ERR_UNDEFINED_VAR_FUNC;

	initValue(&result);
	pc = 3;
	if ((err = getRunLineValue(runline,&pc,VAL_STR,TRUE,&result)) != OK)
		return err;

	err = deleteMapKeyValue(token->var,result.sval);
	clearValue(&result);
	return err;
}





/*** Set the place to jump to on an error or break (control-C) ***/
int comOn(int comnum, st_runline *runline)
{
	st_progline *progline;
	st_token *token;
	st_value result;
	bool is_cont;
	bool is_break;
	int err;
	int pc;

	token = &runline->tokens[1];
	if (token->type != TOK_COM) return ERR_SYNTAX;

	is_cont = IS_COM_TYPE(&runline->tokens[2],COM_CONT);
	is_break = IS_COM_TYPE(&runline->tokens[2],COM_BREAK);

	/* ERROR or BREAK. Keywords following already syntax checked already 
	   in tokeniser.c */
	switch(token->subtype)
	{
	case COM_ERROR:
		/* ON ERROR CONT */
		if (is_cont)
		{
			flags.on_error_cont = TRUE;
			on_jump[ERR_GOTO] = NULL;
			on_jump[ERR_GOSUB] = NULL;
			return OK;
		}
		/* ON ERROR BREAK */
		if (is_break)
		{
			flags.on_error_cont = FALSE;
			on_jump[ERR_GOTO] = NULL;
			on_jump[ERR_GOSUB] = NULL;
			return OK;
		}
		break;
	case COM_BREAK:
		/* ON BREAK CONT */
		if (is_cont)
		{
			flags.on_break_cont = TRUE;
			on_jump[BRK_GOTO] = NULL;
			on_jump[BRK_GOSUB] = NULL;
			return OK;
		}
		/* ON BREAK BREAK */
		if (is_break)
		{
			flags.on_break_cont = FALSE;
			on_jump[BRK_GOTO] = NULL;
			on_jump[BRK_GOSUB] = NULL;
			return OK;
		}
		/* This works alongside the other ON BREAK options so don't
		   clear them */	
		if (IS_COM_TYPE(&runline->tokens[2],COM_CLEAR))
		{
			flags.on_break_clear = TRUE;
			return OK;
		}
		break;
	case COM_TERMSIZE:
		/* ON TERMSIZE CONT */
		if (is_cont)
		{
			flags.on_termsize_cont = 1;
			on_jump[TERM_GOTO] = NULL;
			on_jump[TERM_GOSUB] = NULL;
			return OK;
		}
		/* ON TERMSIZE BREAK */
		if (is_break)
		{
			flags.on_termsize_cont = 0;
			on_jump[TERM_GOTO] = NULL;
			on_jump[TERM_GOSUB] = NULL;
			return OK;
		}
		break;
	default:
		assert(0);
	}

	/* Eval line number */
	pc = 3;
	initValue(&result);
	if ((err = getRunLineValue(runline,&pc,VAL_NUM,TRUE,&result)) != OK)
		return err;

	if (result.dval < 1) return ERR_INVALID_LINENUM;

	if (!(progline = getProgLine(result.dval))) return ERR_NO_SUCH_LINE;

	/* Can only have one or the other set for obvious reasons */
	if (IS_COM_TYPE(&runline->tokens[2],COM_GOTO))
	{
		switch(token->subtype)
		{
		case COM_ERROR:
			flags.on_error_cont = FALSE;
			on_jump[ERR_GOTO] = progline;
			on_jump[ERR_GOSUB] = NULL;
			break;
		case COM_BREAK:
			flags.on_break_cont = FALSE;
			on_jump[BRK_GOTO] = progline;
			on_jump[BRK_GOSUB] = NULL;
			break;
		case COM_TERMSIZE:
			on_jump[TERM_GOTO] = progline;
			on_jump[TERM_GOSUB] = NULL;
			break;
		default:
			assert(0);
		}
		return OK;
	}
	/* GOSUB */
	switch(token->subtype)
	{
	case COM_ERROR:
		flags.on_error_cont = FALSE;
		on_jump[ERR_GOTO] = NULL;
		on_jump[ERR_GOSUB] = progline;
		break;
	case COM_BREAK:
		flags.on_break_cont = FALSE;
		on_jump[BRK_GOTO] = NULL;
		on_jump[BRK_GOSUB] = progline;
		break;
	case COM_TERMSIZE:
		on_jump[TERM_GOTO] = NULL;
		on_jump[TERM_GOSUB] = progline;
		break;
	default:
		assert(0);
	}

	return OK;
}




int comAngleType(int comnum, st_runline *runline)
{
	flags.angle_in_degrees = (comnum == COM_DEG);
        setValue(
		angle_mode_var->value,
		VAL_STR,flags.angle_in_degrees ? "DEG" : "RAD",0);
	return OK;
}




/*** Value is given in seconds ***/
int comSleep(int comnum, st_runline *runline)
{
	st_value result;
	int pc;
	int err;

	setValue(interrupted_var->value,VAL_NUM,NULL,0);

	pc = 1;
	initValue(&result);
	if ((err = getRunLineValue(runline,&pc,VAL_NUM,TRUE,&result)) != OK)
		return err;

	if (result.dval < 0) return ERR_INVALID_ARG;
	usleep((long)(result.dval * 1000000));
	return OK;
}




/*** Sets tracing mode ***/
int comTrace(int comnum, st_runline *runline)
{
	tracing_mode = (comnum == COM_TROFF ? TRACING_OFF : 
	                (comnum == COM_TRON ? TRACING_NOSTEP : TRACING_STEP));
	return OK;
}




int comSeed(int comnum, st_runline *runline)
{
	st_value result;
	int pc;
	int err;

	pc = 1;
	initValue(&result);
	if ((err = getRunLineValue(runline,&pc,VAL_NUM,TRUE,&result)) != OK)
		return err;

	if (result.dval < 0) return ERR_INVALID_ARG;
	srandom(result.dval);
	return OK;
}




/*** Define an expression ***/
int comDefExp(int comnum, st_runline *runline)
{
	return createDefExp(runline);
}




/*** Currently just prints all the commands and functions available ***/
int comHelp(int comnum, st_runline *runline)
{
	st_value result;
	int match;
	int pc;
	int cnt;
	int err;
	int i;

	initValue(&result);
	if (runline->num_tokens > 1)
	{
		pc = 1;
		if ((err = evalExpression(runline,&pc,&result)) != OK)
		{
			clearValue(&result);
			return err;
		}
		if (pc < runline->num_tokens)
		{
			clearValue(&result);
			return ERR_SYNTAX;
		}
		if (result.type != VAL_STR)
		{
			clearValue(&result);
			return ERR_INVALID_ARG;
		}
		match = TRUE;
	}
	else match = FALSE;

	printf("\nCommands & keywords:");
	for(i=cnt=0,pc=1;i < NUM_COMMANDS;++i)
	{
		if (!result.sval || 
		    wildMatch(sorted_commands[i],result.sval,FALSE))
		{
			if (!(cnt++ % 4)) printf("\n    ");
			printf("%-18s",sorted_commands[i]);
		}
	}
	if (!cnt) printf("\n\n*** No matches ***");

	pressAnyKey("\n\nPress any key for functions: ");
	if (last_signal == SIGINT)
	{
		clearValue(&result);
		return OK;
	}
	
	printf("Functions:");
	for(i=cnt=0;i < NUM_FUNCTIONS;++i)
	{
		if (!result.sval || 
		    wildMatch(sorted_functions[i],result.sval,FALSE))
		{
			if (!(cnt++ % 4)) printf("\n    ");
			printf("%-18s",sorted_functions[i]);
		}
	}
	if (cnt)
	{
		if (match) puts("\n");
		else
		{
			puts("\n\nNotes:");
			puts("- You can do case insensitive wildcard searches with HELP. eg: help \"rm*$\"");
			puts("- Functions with $ in their name return a string. All other functions return");
			puts("  a number.\n");
		}
	}
	else puts("\n\n*** No matches ***\n");

	clearValue(&result);
	return OK;
}




/*** Evaluate an expression and dump result. Allows calling functions like
     kill() without having to assign result to anything ***/
int comEval(int comnum, st_runline *runline)
{
	st_value result;
	int pc;
	int err;

	initValue(&result);
	for(pc=1;pc < runline->num_tokens;++pc)
	{
		err = evalExpression(runline,&pc,&result);
		clearValue(&result);
		if (err != OK) return err;
		if (pc == runline->num_tokens) return OK;
		if (NOT_COMMA(pc)) return ERR_SYNTAX;
	}
	return ERR_SYNTAX;
}




/*** Kill all current child processes ***/
int comKillAll(int comnum, st_runline *runline)
{
	killChildProcesses();
	return OK;
}


/********************************* STATICS *********************************/

int doComRun(int comnum, st_runline *runline, int pc)
{
	st_value result;
	int err;

	deleteDefExps();
	deleteVariables(runline);
	resetSystemVariables();

	/* This sets the new runline to the first in the program */
	resetProgram();

	/* Set $run_arg if an argument was passed */
	if (runline->num_tokens > pc)
	{
		/* Pc > 1 if CHAIN command */
		if (pc > 1)
		{
			if (NOT_COMMA(pc)) return ERR_SYNTAX;
			++pc;
		}
		initValue(&result);

		if ((err = getRunLineValue(
			runline,&pc,VAL_UNDEF,TRUE,&result)) != OK)
		{
			return err;
		}
		if (pc < runline->num_tokens)
		{
			clearValue(&result);
			return ERR_SYNTAX;
		}
		copyValue(run_arg_var->value,&result);
		clearValue(&result);
	}

	return OK;
}




/*** Saves duplicated code ***/
int getRunLineValue(
	st_runline *runline, int *pc, int type, int eol, st_value *result)
{
	int err;

	if (*pc >= runline->num_tokens) return ERR_SYNTAX;

	if ((err = evalExpression(runline,pc,result)) != OK)
	{
		clearValue(result);
		return err;
	}
	if ((eol == TRUE && *pc < runline->num_tokens) ||
	    (eol == FALSE && *pc >= runline->num_tokens))
	{
		clearValue(result);
		return ERR_SYNTAX;
	}
	if (type != VAL_UNDEF && result->type != type)
	{
		clearValue(result);
		return ERR_INVALID_ARG;
	}
	return OK;
}




/*** Find the matching end block command ***/
bool setBlockEnd(st_runline *runline, int start_com, int end_com)
{
	st_runline *rl;
	st_runline *end_rl;
	st_token *token;
	bool found_break;
	int nest;
	
	/* Only need to check the first token of each runline. If the end
	   commands are elsewhere its a syntax error anyway. */
	found_break = FALSE;
	for(rl=runline->next,nest=1;rl;rl=rl->next)
	{
		token = &rl->tokens[0];
		if (end_com == COM_FI &&
		    (start_com == COM_IF || start_com == COM_ELSE) &&
		    IS_COM_TYPE(token,COM_FIALL))
		{
			nest = 0;
		}
		else if (IS_COM_TYPE(token,start_com)) ++nest;
		else if (IS_COM_TYPE(token,end_com)) --nest;

		if (nest == 1)
		{
			switch(start_com)
			{
			case COM_FOR:
			case COM_FOREACH:
			case COM_WHILE:
			case COM_REPEAT:
			case COM_LOOP:
				if (IS_COM_TYPE(token,COM_CONTLOOP))
					rl->jump = runline;
				else if (IS_COM_TYPE(token,COM_BREAK))
				{
					rl->jump = runline; 
					found_break = TRUE;
				}
				break;
			case COM_CHOOSE:
				if (IS_COM_TYPE(token,COM_BREAK))
				{
					rl->jump = runline; 
					found_break = TRUE;
				}
				break;
			}
		}

		/* Do now so rl not set to rl->next */
		if (!nest) break;
	}
	if (nest)
	{
		/* Didn't find a block end command. If its a direct IF command
		   ie no line number, then don't worry about FI */
		return (nest == 1 && !runline->parent->linenum &&
		        (start_com == COM_IF || start_com == COM_ELSE));
	}

	if (end_com == COM_ELSE)
	{
		runline->else_jump = rl;
		return TRUE;
	}

	runline->jump = rl;
	rl->jump = runline;

	if (found_break)
	{
		end_rl = rl;
		for(rl=runline->next;rl;rl=rl->next)
		{
			if (rl->jump && IS_COM_TYPE(&rl->tokens[0],COM_BREAK))
				rl->jump = end_rl->next;
		}
	}
	return TRUE;
}

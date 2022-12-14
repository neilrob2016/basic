#include "globals.h"

static st_runline *checkEOLCommand(
	st_progline *progline, st_runline *runline, st_token *token);
static st_runline *checkSOLCommand(
	st_progline *progline, st_runline *runline, st_token *token);
static bool        commandSanityCheck(st_progline *progline);
static bool        isSingleCharOp(char c);
static st_progline *createProgLine();
static void        addRunLineToProgLine(
	st_progline *progline, st_runline *runline);
static int         countBrackets(st_progline *progline);
static st_runline *createRunLine();
static st_token   *createToken(st_runline *runline);
static void        deleteEndToken(st_runline *runline);
static void        processTokens(st_runline *runline);
static void        addCharToToken(st_token *token, char c);
static int         setTokenType(st_token *token);
static void        clearToken(st_token *token);

/*****************************************************************************/

#define CHECK_TOKEN() \
	in_rem = (IS_COM_TYPE(token,COM_REM)); \
	runline = checkSOLCommand(progline,runline,token); \
	runline = checkEOLCommand(progline,runline,token);


/*** Split up the line into tokens ***/
st_progline *tokenise(char *line)
{
	st_progline *progline;
	st_runline *runline;
	st_runline *prev_runline;
	st_token *token;
	bool in_quotes;
	bool in_rem;
	int err;
	char *s;

	progline = createProgLine();
	runline = createRunLine(progline);
	token = createToken(runline);

	in_quotes = FALSE;
	in_rem = FALSE;

	for(s=line;*s;++s)
	{
		/* Just keep adding characters to the token */
		if (in_rem)
		{
			addCharToToken(token,*s);
			continue;
		}
		switch(*s)
		{
		case '"':
			if (in_quotes)
			{
				in_quotes = FALSE;
				if ((err = setTokenType(token)) != OK)
					goto ERROR;
				token = createToken(runline);
				break;
			}
			/* If we're already updating a token then we're done
			   with it. Get a new one */
			if (token->str)
			{
				if ((err = setTokenType(token)) != OK)
					goto ERROR;
				CHECK_TOKEN();
				token = createToken(runline);
			}
			token->quoted = TRUE;
			in_quotes = TRUE;
			break;

		case '\n':
			/* Should never hit a newline. Ignore if we do */
			continue;

		case ':':
			if (in_quotes)
			{
				addCharToToken(token,*s);
				break;
			}
			if (runline->num_tokens)
			{
				prev_runline = runline;
				if (token->str)
				{
					if ((err = setTokenType(token)) != OK)
						goto ERROR;
					CHECK_TOKEN();
				}
				else deleteEndToken(runline);

				/* If the checkS*() functions didn't create
				   a new runline then do it here */
				if (!token->str || prev_runline == runline)
				{
					addRunLineToProgLine(progline,runline);
					runline = createRunLine(progline);
				}
				token = createToken(runline);
			}
			break;

		case '?':
			/* PRINT shortcut */
			if (in_quotes)
			{
				addCharToToken(token,*s);
				break;
			}
			if (token->str)
			{
				if ((err = setTokenType(token)) != OK)
					goto ERROR;
				CHECK_TOKEN();
				token = createToken(runline);
			}
			token->type = TOK_COM;
			if ((err = setTokenType(token)) != OK) goto ERROR;
			token = createToken(runline);
			break;
			
		default:
			if (in_quotes)
			{
				addCharToToken(token,*s);
				break;
			}
			if (isspace(*s))
			{
				if (token->str)
				{
					if ((err = setTokenType(token)) != OK)
						 goto ERROR;
					CHECK_TOKEN();
					token = createToken(runline);
				}
				break;
			}
			/* If its an operator character then set token and get 
			   new one. We'll worry about multi character operators
			   and negative numbers in processTokens() */
			if (isSingleCharOp(*s))
			{
				/* Finish current token */
				if (token->str)
				{
					if ((err = setTokenType(token)) != OK)
						 goto ERROR;
					CHECK_TOKEN();
					token = createToken(runline);
				}
				addCharToToken(token,*s);

				if (!in_rem)
				{
					if ((err = setTokenType(token)) != OK)
						 goto ERROR;
					token = createToken(runline);
				}
			}
			else addCharToToken(token,*s);
		}
	}
	if (in_quotes) 
	{
		err = ERR_MISSING_END_QUOTES;
		goto ERROR;
	}

	/* Set type unless string not set (because all whitespace) in which
	   case delete it */
	if (token->str)
	{
		if ((err = setTokenType(token)) != OK) goto ERROR;
		runline = checkSOLCommand(progline,runline,token);
	}
	else deleteEndToken(runline);

	/* If tokens in runline then add to program line, else free it */
	if (runline->num_tokens)
		addRunLineToProgLine(progline,runline);
	else
		free(runline);

	/* If no runlines in program line then free it else check bracket
	   counts and min command token numbers */
	if (!progline->first_runline)
	{
		FREE(progline)
		return NULL;
	}
	if ((err = countBrackets(progline)) != OK) goto ERROR;
	if (commandSanityCheck(progline)) return progline;
	err = ERR_SYNTAX;
	/* Fall through */

	ERROR:
	doError(err,progline);
	deleteProgLine(progline,TRUE,TRUE);
	return NULL;
}




/*** Check End Of Line. Checks if the token is a command that can only appear 
     at the end of the line. If it is it adds the runline to the program, 
     creates a new runline and returns the pointer ***/
static st_runline *checkEOLCommand(
	st_progline *progline, st_runline *runline, st_token *token)
{
	if (!IS_COM(token)) return runline;

	/* Only some commands must be at the end of a line */
	switch(token->subtype)
	{
	case COM_THEN:
	case COM_ELSE:
	case COM_FI:
	case COM_FIALL:
	case COM_REPEAT:
		break;

	default:
		return runline;
	}
	addRunLineToProgLine(progline,runline);
	return createRunLine(progline);
}




/*** Check Start Of Line. As above except it does it for start of line 
     commands. Which is pretty much all of them ***/
static st_runline *checkSOLCommand(
	st_progline *progline, st_runline *runline, st_token *token)
{
	st_runline *new_runline;
	st_token *new_token;

	/* Do nothing if token is not a command or is the first token anyway */
	if (!IS_COM(token) || 
	    runline->num_tokens < 2 ||
	    (IS_NUM_TYPE(&runline->tokens[0],NUM_INT) && runline->num_tokens < 3))
		return runline;

	switch(token->subtype)
	{
	case COM_THEN:
	case COM_TO:
	case COM_STEP:
	case COM_IN:
	case COM_ERROR:
	case COM_BREAK:
	case COM_END:
	case COM_FROM:
	case COM_TERMSIZE:
		/* Ignore these */
		return runline;

	case COM_GOTO:
	case COM_GOSUB:
	case COM_CONT:
	case COM_CLEAR:
		/* Won't be at the start if preceeded by o
		   ON ERROR/BREAK/TERMSIZE */
		new_token = &runline->tokens[runline->num_tokens-2];
		if (IS_COM_TYPE(new_token,COM_ERROR) ||
		    IS_COM_TYPE(new_token,COM_BREAK) ||
		    IS_COM_TYPE(new_token,COM_TERMSIZE)) return runline;
		break;
	}

	/* Create a new runline, get a new token and copy across current one
	   then delete current token from end of current runline */
	new_runline = createRunLine(progline);
	new_token = createToken(new_runline);

	/* Copy except for string which must be dup'd since it will be free'd
	   in original */
	memcpy(new_token,token,sizeof(st_token));
	if (token->str) new_token->str = strdup(token->str);

	deleteEndToken(runline);
	addRunLineToProgLine(progline,runline);
	return new_runline;
}




/*** Check specific commands have the required max/min number of tokens and
     have the correct format. Saves doing it all the time during a run. ***/
static bool commandSanityCheck(st_progline *progline)
{
	st_runline *runline;
	st_token *token;
	int pos;
	
	for(runline=progline->first_runline;runline;runline=runline->next)
	{
		/* Take account of line number token */
		token = &runline->tokens[0];
		if (IS_NUM(token) && token->subtype == NUM_INT)
		{
			/* Just line number? */
			if (runline->num_tokens == 1) continue;
			token = &runline->tokens[1];
			pos = 1;
		}
		else pos = 0;

		if (token->type != TOK_COM) continue;

		switch(token->subtype)
		{
		case COM_NEW:
		case COM_RETURN:
		case COM_WEND:
		case COM_NEXT:
		case COM_LEND:
		case COM_STOP:
		case COM_CONT:
		case COM_FI:
		case COM_FIALL:
		case COM_CLS:
		case COM_TRON:
		case COM_TROFF:
		case COM_WRON:
		case COM_WROFF:
		case COM_STON:
		case COM_STOFF:
		case COM_CHOSEN:
		case COM_DEFAULT:
		case COM_ERROR:
		case COM_BREAK:
		case COM_END:
		case COM_TERMSIZE:
			if (runline->num_tokens > 1+pos) return FALSE;
			break;

		case COM_DIM:
		case COM_REDIM:
		case COM_GOTO:
		case COM_GOSUB:
		case COM_WHILE:
		case COM_UNTIL:
		case COM_LOOP:
		case COM_READ:
		case COM_RESTORE:
		case COM_AUTORESTORE:
		case COM_LOAD:
		case COM_CHAIN:
		case COM_MERGE:
		case COM_SAVE:
		case COM_RM:
		case COM_CLOSE:
		case COM_INPUT:
		case COM_CINPUT:
		case COM_DELETE:
		case COM_PEN:
		case COM_PAPER:
		case COM_ATTR:
		case COM_SCROLL:
		case COM_SEED:
		case COM_EDIT:
		case COM_INDENT:
		case COM_CHOOSE:
		case COM_CASE:
		case COM_MOVE:
			if (runline->num_tokens < 2+pos) return FALSE;
			break;

		case COM_IF:
			if (runline->num_tokens < 3+pos ||
			    !IS_COM_TYPE(&runline->tokens[runline->num_tokens-1],COM_THEN))
				return FALSE;
			break;

		case COM_LET:
		case COM_LOCATE:
		case COM_PLOT:
			if (runline->num_tokens < 4+pos) return FALSE;
			break;

		case COM_LINE:
		case COM_CIRCLE:
			if (runline->num_tokens < 8+pos) return FALSE;
			break;

		case COM_RECT:
			if (runline->num_tokens < 10+pos) return FALSE;
			break;

		case COM_DELKEY:
			if (runline->num_tokens < 4+pos ||
			    !IS_VAR(&runline->tokens[1+pos]) ||
			    NOT_COMMA(2+pos))
				return FALSE;
			break;

		case COM_FOR:
			if (runline->num_tokens < 6+pos ||
			    !IS_VAR(&runline->tokens[1+pos]) ||
			    !IS_OP_TYPE(&runline->tokens[2+pos],OP_EQUALS))
				return FALSE;
			break;

		case COM_FOREACH:
			/* FOREACH a,b IN c */
			if (runline->num_tokens != 6+pos ||
			    !IS_VAR(&runline->tokens[1+pos]) ||
			    NOT_COMMA(2+pos) ||
			    !IS_VAR(&runline->tokens[3+pos]) ||
			    !IS_COM_TYPE(&runline->tokens[4+pos],COM_IN) ||
			    !IS_VAR(&runline->tokens[5+pos]))
				return FALSE;
			break;

		case COM_ON:
			if (runline->num_tokens < 3+pos) return FALSE;
			token = &runline->tokens[2+pos];

			if (IS_COM_TYPE(&runline->tokens[1+pos],COM_BREAK) ||
			    IS_COM_TYPE(&runline->tokens[1+pos],COM_ERROR) ||
			    IS_COM_TYPE(&runline->tokens[1+pos],COM_TERMSIZE))
			{
				if (IS_COM_TYPE(token,COM_CONT) ||
				    IS_COM_TYPE(token,COM_CLEAR) ||
				    IS_COM_TYPE(token,COM_BREAK))
				{
					if (runline->num_tokens != 3+pos)
						return FALSE;
				}
				else if (runline->num_tokens < 4+pos ||
				        (!IS_COM_TYPE(token,COM_GOTO) &&
			    	         !IS_COM_TYPE(token,COM_GOSUB)))
				{
					return FALSE;
				}
			}
			break;

		case COM_DEFEXP:
			/* token 1 is TOK_VAR since we don't put ! in front
			   of expression name so tokeniser defaults to var.
			   Doesn't matter because we never look it up */
			if (runline->num_tokens < 4+pos ||
			    !IS_VAR(&runline->tokens[1+pos]) ||
			    !IS_OP_TYPE(&runline->tokens[2+pos],OP_EQUALS))
				return FALSE;
			break;
		}
	}
	return TRUE;
}


/******************************** OPERATORS *******************************/

static bool isSingleCharOp(char c)
{
	int i;

	for(i=0;i < NUM_OPS;++i)
		if (!op_info[i].str[1] && c == op_info[i].str[0]) return TRUE;
	return FALSE;
}


/******************************** PROGRAM LINE *******************************/

static st_progline *createProgLine()
{
	st_progline *progline;

	assert((progline = (st_progline *)malloc(sizeof(st_progline))));
	progline->linenum = 0;
	progline->first_runline = NULL;
	progline->last_runline = NULL;
	progline->prev = NULL;
	progline->next = NULL;

	return progline;
}




/*** Add the run line to the program line ***/
static void addRunLineToProgLine(st_progline *progline, st_runline *runline)
{
	if (!runline->num_tokens) return;

	processTokens(runline);

	if (progline->last_runline)
	{
		progline->last_runline->next = runline;
		runline->prev = progline->last_runline;
	}
	else progline->first_runline = runline;

	progline->last_runline = runline;
}




/*** Make sure each runline has matching brackets ***/
static int countBrackets(st_progline *progline)
{
	st_runline *runline;
	st_token *token;
	int cnt;
	int i;

	assert(progline);

	for(runline=progline->first_runline;;runline=runline->next)
	{
		for(cnt=i=0;i < runline->num_tokens;++i)
		{
			token = &runline->tokens[i];
			if (IS_OP_TYPE(token,OP_L_BRACKET)) ++cnt;
			else if (IS_OP_TYPE(token,OP_R_BRACKET)) --cnt;
		}
		if (cnt) return ERR_MISSING_BRACKET;
		if (runline == progline->last_runline) return OK;
	}

	assert(0);
	return OK;
}


/********************************** RUN LINE *********************************/

static st_runline *createRunLine(st_progline *progline)
{
	st_runline *runline;

	assert((runline = (st_runline *)malloc(sizeof(st_runline))));
	bzero(runline,sizeof(st_runline));
	runline->parent = progline;

	return runline;
}




static st_token *createToken(st_runline *runline)
{
	st_token *token;

	if (runline->num_tokens >= runline->alloc_tokens)
	{
		runline->tokens = (st_token *)realloc(
			runline->tokens,sizeof(st_token) * (runline->num_tokens+1));
		assert(runline->tokens);
		++runline->alloc_tokens;
	}

	token = &runline->tokens[runline->num_tokens++];
	bzero(token,sizeof(st_token));
	return token;
}




/*** End token not used , get rid of it ***/
static void deleteEndToken(st_runline *runline)
{
	st_token *token = &runline->tokens[--runline->num_tokens];
	clearToken(token);
}




/*** Go through tokens and amalgamate multi character tokens and deal with
     negative numbers ***/
static void processTokens(st_runline *runline)
{
	st_token *tok1;
	st_token *tok2;
	int i;
	int j;

	if (!runline->num_tokens) return;

	if (runline->tokens[runline->num_tokens-1].str == NULL)
		--runline->num_tokens;

	/* Check for negative numbers and double operators. Word 
	   operators are already found */
	for(i=0;i < runline->num_tokens-1;++i)
	{
		tok1 = &runline->tokens[i];
		tok2 = &runline->tokens[i+1];

		/* Check for negative number at start */
		if (!i && IS_OP_TYPE(tok1,OP_SUB))
		{
			tok2->negative = TRUE;
			/* Get rid of minus operator token */
			deleteTokenFromRunLine(runline,i);
			continue;
		}

		/* Check for negative following commands or most ops.
		   eg: PRINT -2 or PRINT (-2) or PRINT -(-2) */
		if (i < runline->num_tokens - 1 && 
		    (IS_OP(tok1) || IS_COM(tok1)) && 
		    IS_OP_TYPE(tok2,OP_SUB) &&
		    !IS_OP_TYPE(tok1,OP_R_BRACKET))
		{
			runline->tokens[i+2].negative = TRUE;
			deleteTokenFromRunLine(runline,i+1);
			continue;
		}

		/* Check for double ops. They are: <>, >=, <=, << and >> */
		if (IS_OP(tok1) && tok1->len == 1 && 
		    IS_OP(tok2) && tok2->len == 1)
		{
			for(j=0;j < NUM_OPS;++j)
			{
				if (strlen(op_info[j].str) == 2 &&
				    tok1->str[0] == op_info[j].str[0] &&
				    tok2->str[0] == op_info[j].str[1])
				{
					addCharToToken(tok1,tok2->str[0]);
					tok1->subtype = j;
					deleteTokenFromRunLine(runline,i+1);

					/* So double char op is next tok1 in
					   order to check for op then neg */
					--i;
					break;
				}
			}
		}
	}
}




/*** Shift the tokens list to the left/down deleting the token at "toknum" ***/
void deleteTokenFromRunLine(st_runline *runline, int toknum)
{
	int i;

	clearToken(&runline->tokens[toknum]);
	--runline->num_tokens;

	for(i=toknum;i < runline->num_tokens;++i)
		runline->tokens[i] = runline->tokens[i+1];
}




void deleteRunLine(st_runline *runline, bool force)
{
	int i;

	assert(runline);

	if (runline->prev) runline->prev->next = runline->next;
	if (runline->next) runline->next->prev = runline->prev;

	if (runline->defexp && !force)
	{
		runline->prev = NULL;
		runline->next = NULL;
		return;
	}

	for(i=0;i < runline->num_tokens;++i)
		clearToken(&runline->tokens[i]);
	FREE(runline->tokens);
	if (runline->for_loop) free(runline->for_loop);
	if (runline->foreach_loop) free(runline->foreach_loop);

	free(runline);
}



/********************************** TOKEN **********************************/

static void addCharToToken(st_token *token, char c)
{
	token->str = (char *)realloc(token->str,token->len+2);
	assert(token->str);
	token->str[token->len] = c;
	token->str[++token->len] = 0;
}




/*** Set the type to command, function etc ***/
static int setTokenType(st_token *token)
{
	int i;

	/* Will only be already set if '?' found */
	if (token->type == TOK_COM)
	{
		token->str = strdup("PRINT");
		assert(token->str);
		token->type = TOK_COM;
		token->subtype = COM_PRINT;
		return OK;
	}

	/* If string not set then it means empty quotes: "". So allocate
	   1 byte of memory */
	if (!token->str)
	{
		assert(token->quoted);
		token->str = (char *)malloc(1);
		assert(token->str);
		token->str[0] = 0;
	}

	if (token->quoted)
	{
		token->type = TOK_STR;
		return OK;
	}
	if (token->str[0] == '!')
	{
		token->type = TOK_DEFEXP;
		return OK;
	}
	for(i=0;i < NUM_OPS;++i)
	{
		if (!strcasecmp(token->str,op_info[i].str))
		{
			token->type = TOK_OP;
			token->subtype = i;

			/* Make uppercase to make listings look nicer */
			toUpperStr(token->str);
			return OK;
		}
	}
	for(i=0;i < NUM_COMMANDS;++i)
	{
		if (!strcasecmp(token->str,command[i].name))
		{
			token->type = TOK_COM;
			if (i == COM_REM_SHORTCUT) i = COM_REM;
			token->subtype = i;
			toUpperStr(token->str);
			return OK;
		}
	}
	for(i=0;i < NUM_FUNCTIONS;++i)
	{
		if (!strcasecmp(token->str,function[i].name))
		{
			assert(function[i].num_params <= MAX_FUNC_PARAMS);
			token->type = TOK_FUNC;
			token->subtype = i;
			toLowerStr(token->str);
			return OK;
		}
	}
	if ((i = numType(token->str)) != NOT_NUM)
	{
		token->type = TOK_NUM;
		token->subtype = i;

		/* Base 8 and 16 are integer only */
		switch(i)
		{
		case NUM_BIN:
			token->dval = strtol(token->str+2,NULL,2);
			break;

		case NUM_OCT:
			token->dval = strtol(token->str,NULL,8);
			break;

		case NUM_HEX:
			token->dval = strtol(token->str,NULL,16);
			break;

		case NUM_INT:
		case NUM_FLOAT:
			errno = 0;
			token->dval = strtod(token->str,NULL);
			if (errno == ERANGE) return ERR_OVERFLOW;
			break;

		default:
			assert(0);
		}
	}
	else token->type = TOK_VAR;

	return OK;
}




static void clearToken(st_token *token)
{
	assert(token);
	FREE(token->str);
	bzero(token,sizeof(st_token));
}

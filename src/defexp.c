#include "globals.h"

int createDefExp(st_runline *runline)
{
	st_defexp *exp;
	char *name;

	name = runline->tokens[1].str;
	if (getDefExp(name)) return ERR_DUPLICATE_DEFEXP;
	if (getVariable(name)) return ERR_VAR_ALREADY_HAS_NAME;

	/* Same naming rules as variables */
	if (!validVariableName(runline->tokens[1].str))
		return ERR_INVALID_DEFEXP_NAME;
	
	assert((exp = (st_defexp *)malloc(sizeof(st_defexp))));
	exp->name = strdup(name);
	exp->runline = runline;
	exp->next = NULL;

	/* Only set this if we're a direct command, ie no line number, so the
	   runline isn't deleted as it would otherwise be when the program line
	   is deleted  */
	if (!runline->parent->linenum) runline->defexp = exp;

	if (first_defexp)
	{
		last_defexp->next = exp;
		exp->prev = last_defexp;
	}
	else
	{
		first_defexp = exp;
		exp->prev = NULL;
	}

	last_defexp = exp;
	return OK;
}




st_defexp *getDefExp(char *name)
{
	st_defexp *exp;

	for(exp=first_defexp;exp;exp=exp->next)
		if (!strcmp(exp->name,name)) return exp;
	return NULL;
}




/*** Reset the exp pointer in every token that referenced this expression ***/
void resetTokenExps(st_defexp *exp)
{
	st_runline *rl;
	int i;

	for(rl=prog_first_line ? prog_first_line->first_runline : NULL;
	    rl;rl=rl->next)
	{
		for(i=0;i < rl->num_tokens;++i)
			if (rl->tokens[i].exp == exp) rl->tokens[i].exp = NULL;
	}
}




void renameDefExp(st_defexp *exp, char *new_name)
{
	free(exp->name);
	exp->name = strdup(new_name);
	assert(exp->name);
}




/*** Delete the expression that uses this runline ***/
void deleteDefExp(st_runline *runline, bool force)
{
	st_defexp *exp;

	/* If runline->defexp is set it means this runline was a direct 
	   command DEFEXP (ie not on a program line), so don't delete the 
	   expression on this runline unless forced */
	if (runline->defexp)
	{
		if (!force) return;
		exp = runline->defexp;
	}
	else
	{
		/* Find the expression on this runline */
		for(exp=first_defexp;
		    exp && exp->runline != runline;exp=exp->next);
		if (!exp) return;
	}

	if (exp == first_defexp)
		first_defexp = exp->next;
	else
		exp->prev->next = exp->next;

	if (exp == last_defexp)
		last_defexp = exp->prev;
	else
		exp->next->prev = exp->prev;
	free(exp->name);
	free(exp);
	resetTokenExps(exp);
}




/*** Delete all expressions ***/
void deleteDefExps()
{
	st_defexp *exp;
	st_defexp *next;

	for(exp=first_defexp;exp;exp=next)
	{
		/* Delete runline we've kept a note of since it'll be 
		   orphaned from the main program */
		if (exp->runline->defexp) deleteRunLine(exp->runline,TRUE);
		next = exp->next;
		free(exp->name);
		free(exp);
		resetTokenExps(exp);
	}
	first_defexp = last_defexp = NULL;
}




/*** Dump all expressions to the console ***/
void dumpDefExps(FILE *fp, char *pat)
{
	st_defexp *exp;
	for(exp=first_defexp;exp;exp=exp->next)
		if (!pat || wildMatch(exp->name,pat,TRUE)) dumpDefExp(fp,exp);
}




/*** Dump the expression to the console ***/
void dumpDefExp(FILE *fp, st_defexp *exp)
{
	st_token *token;
	st_token *next_token;
	int i;

	fprintf(fp,"!%-13s = ",exp->name);

	/* Tokens will be "DEFEXP <name> = ..." hence start i at 3 */
	for(i=3;i < exp->runline->num_tokens;++i)
	{
		token = &exp->runline->tokens[i];
		if (i < exp->runline->num_tokens-1)
			next_token = &exp->runline->tokens[i+1];
		else
			next_token = NULL;
		fprintf(fp,"%s",token->str);

		/* Don't print spaces after function names or before or after
		   brackets so it looks nicer */
		if (token->type != TOK_FUNC &&
		    !(token->type == TOK_OP && 
		      token->subtype == OP_L_BRACKET) && 
		    !(next_token && 
		      next_token->type == TOK_OP && 
	              next_token->subtype == OP_R_BRACKET))
		{
			fputc(' ',fp);
		}
	}
	fputc('\n',fp);
}

#include "globals.h"

void initLabels(void)
{
	first_label = NULL;
	last_label = NULL;
}




/*** Create all the labels on a given program line ***/
int createLabels(st_progline *progline)
{
	st_runline *rl;
	st_token *tok;
	st_label *lbl;

	for(rl=progline->first_runline;rl;rl=rl->next)
	{
		if (!IS_COM_TYPE(&rl->tokens[0],COM_LABEL)) continue;

		/* Format is LABEL "<name>". Can't have an expression because 
		   the labels need to be created before the program runs but 
		   must be a quoted string or it'll be parsed as something 
		   else */
		if (rl->num_tokens != 2) return ERR_SYNTAX;
		tok = &rl->tokens[1];
		if (tok->type != TOK_STR) return ERR_INVALID_ARG;
		if (getLabel(tok->str)) return ERR_DUPLICATE_LABEL;

		/* Create new label */
		lbl = (st_label *)malloc(sizeof(st_label));
		assert(lbl);
		lbl->name = strdup(tok->str);
		lbl->runline = rl;
		lbl->next = NULL;

		/* Add to linked list */
		if (first_label)
		{
			last_label->next = lbl;
			lbl->prev = last_label;
		}
		else
		{
			first_label = lbl;
			lbl->prev = NULL;
		}
		last_label = lbl;
	}
	return OK;
}




/*** Delete all the labels on a given program line ***/
void deleteProgLineLabels(st_progline *progline)
{
	st_runline *rl;
	st_label *lbl;

	for(rl=progline->first_runline;rl && rl->parent==progline;rl=rl->next)
	{
		if (!IS_COM_TYPE(&rl->tokens[0],COM_LABEL)) continue;

		/* Could be null if all labels already deleted */
		if (!(lbl = getLabel(rl->tokens[1].str))) continue;

		if (lbl == first_label)
			first_label = lbl->next;
		else
			lbl->prev->next = lbl->next;

		if (lbl == last_label)
			last_label = lbl->prev;
		else
			lbl->next->prev = lbl->prev;

		free(lbl->name);
		free(lbl);
	}
}




/*** Delete all labels ***/
void deleteAllLabels(void)
{
	st_label *lbl;
	st_label *next;

	for(lbl=first_label;lbl;lbl=next)
	{
		next = lbl->next;
		free(lbl->name);
		free(lbl);
	}
	initLabels();
}




st_label *getLabel(char *name)
{
	st_label *lbl;
	assert(name);
	for(lbl=first_label;lbl;lbl=lbl->next)
		if (!strcmp(lbl->name,name)) return lbl;	
	return NULL;
}

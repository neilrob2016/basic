#include "globals.h"

void initLabels(void)
{
	bzero(first_label,sizeof(first_label));
	bzero(last_label,sizeof(last_label));
}




/*** Create all the labels on a given program line ***/
int createLabels(st_progline *progline)
{
	st_runline *rl;
	st_token *tok;
	st_label *lbl;
	int c;

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
		if (getLabel(tok->str,tok->len)) return ERR_DUPLICATE_LABEL;

		/* Create new label */
		lbl = (st_label *)malloc(sizeof(st_label));
		assert(lbl);
		lbl->name = strdup(tok->str);
		lbl->len = strlen(tok->str);
		lbl->runline = rl;
		lbl->next = NULL;
		c = lbl->name[0];

		/* Add to linked list */
		if (first_label[c])
		{
			last_label[c]->next = lbl;
			lbl->prev = last_label[c];
		}
		else
		{
			first_label[c] = lbl;
			lbl->prev = NULL;
		}
		last_label[c] = lbl;
	}
	return OK;
}




/*** Delete all the labels on a given program line ***/
void deleteProgLineLabels(st_progline *progline)
{
	st_runline *rl;
	st_label *lbl;
	st_token *tok;
	int c;

	for(rl=progline->first_runline;rl && rl->parent==progline;rl=rl->next)
	{
		if (!IS_COM_TYPE(&rl->tokens[0],COM_LABEL)) continue;

		/* Could be null if all labels already deleted */
		tok = &rl->tokens[1];
		if (!(lbl = getLabel(tok->str,tok->len))) continue;
		c = lbl->name[0];

		if (lbl == first_label[c])
			first_label[c] = lbl->next;
		else
			lbl->prev->next = lbl->next;

		if (lbl == last_label[c])
			last_label[c] = lbl->prev;
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
	int c;

	for(c=0;c <= MAX_UCHAR;++c)
	{
		for(lbl=first_label[c];lbl;lbl=next)
		{
			next = lbl->next;
			free(lbl->name);
			free(lbl);
		}
	}
	initLabels();
}




/*** Passing the length is a lookup efficiency measure ***/
st_label *getLabel(char *name, int len)
{
	st_label *lbl;
	int c;
	assert(name);
	c = name[0];

	for(lbl=first_label[c];lbl;lbl=lbl->next)
		if (lbl->len == len && !strcmp(lbl->name,name)) return lbl;	
	return NULL;
}




int dumpLabels(FILE *fp, char *pat)
{
	st_label *lbl;
	char *text;
	int cnt;
	int c;

	for(c=cnt=0;c <= MAX_UCHAR;++c)
	{
		for(lbl=first_label[c];lbl;lbl=lbl->next)
		{
			if (!pat || wildMatch(lbl->name,pat,TRUE))
			{
				asprintf(&text,"\"%s\"",lbl->name);
				fprintf(fp,"%-15s LABEL on line %d\n",
					text,lbl->runline->parent->linenum);
				free(text);
				++cnt;
			}
		}
	}
	return cnt;
}

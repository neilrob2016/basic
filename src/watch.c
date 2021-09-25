#include "globals.h"

void initWatchVars()
{
	watch_vars = NULL;
	watch_alloc = 0;
	watch_cnt = 0;
}




/*** Not using indexes because there won't be enough variables being watched to 
     make it worthwhile ***/
int findWatchVar(char *varname)
{
	int i;
 
	if (watch_cnt)
	{
		for(i=0;i < watch_alloc;++i)
		{
			if (watch_vars[i] && !strcmp(watch_vars[i],varname))
				return i;
		}
	}
	return -1;
}




/*** Could just have a flag in st_var that a variable is being watched but 
     then we couldn't see it being created ***/
int addWatchVar(char *varname)
{
	int pos;

	if (!validVariableName(varname)) return ERR_INVALID_VAR_NAME;
	if (findWatchVar(varname) != -1) return ERR_VAR_ALREADY_WATCHED;

	/* Find null entry */
	for(pos=0;pos < watch_alloc && watch_vars[pos];++pos);
	if (pos == watch_alloc) 
	{
		pos = watch_alloc++;
		watch_vars = (char **)realloc(
			watch_vars,watch_alloc * sizeof(char **));
		assert(watch_vars);
		++watch_cnt;
	}
	watch_vars[pos] = strdup(varname);
	return OK;
}




int removeWatchVar(char *varname)
{
	int pos;
	if ((pos = findWatchVar(varname)) == -1)
		return ERR_VAR_NOT_WATCHED;

	free(watch_vars[pos]);
	watch_vars[pos] = NULL;
	--watch_cnt;
	return OK;
}




void printWatchVars()
{
	int comma = 0;
	int i;

	if (!watch_cnt)
	{
		puts("There are no variables being watched.");
		return;
	}

	printf("Watched variables:");
	for(i=0;i < watch_alloc;++i)
	{
		if (watch_vars[i])
		{
			if (comma)
				putchar(',');
			else
				comma = 1;
			printf(" %s",watch_vars[i]);
		}
	}
	putchar('\n');
}




void clearWatchVars()
{
	int i;
	for(i=0;i < watch_alloc;++i) FREE(watch_vars[i]);
	FREE(watch_vars);
	initWatchVars();
}




/*** Called from various places in variables.c ***/
void printWatchLet(st_var *var, char *key, int arrpos, st_value *value)
{
	if (key)
	{
		if (value->type == VAL_STR)
			printf("{LET,%s(\"%s\")=\"%s\"}\n",var->name,key,value->str);
		else
			printf("{LET,%s(\"%s\")=%f}\n",var->name,key,value->dval);
		return;
	}
	if (var->index_cnt)
	{
		++arrpos;
		if (value->type == VAL_STR)
			printf("{LET,%s(%d)=\"%s\"}\n",var->name,arrpos,value->str);
		else
			printf("{LET,%s(%d)=%f}\n",var->name,arrpos,value->dval);
		return;
	}
	if (value->type == VAL_STR)
		printf("{LET,%s=\"%s\"}\n",var->name,value->str);
	else
		printf("{LET,%s=%f}\n",var->name,value->dval);
}

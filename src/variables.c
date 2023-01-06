#include "globals.h"

/* Anything not in this list is already an operator */
#define INVALID_NAME_CHARS "`'$&!?{}[].\\"

/* Don't want all the system variables indexed by '$', use 2nd character
   instead */
#define INDEX_CHAR(N) (N[0] == '$' ? N[1] : N[0])

static char *vartype[NUM_VAR_TYPES] = { "STD","MAP","MEM" };

void  addVarToList(st_var *var);
void  removeVarFromList(st_var *var);
int   checkIndex(st_var *var, int index_cnt, int *index, int *arrpos);
int   setMapValue(st_var *var, char *key, st_value *value);
int   getMapValue(st_var *var, char *key, st_value *result);
void  resetRunLineVariablePointer(st_runline *runline, st_var *var);


void createSystemVariables(int argc, char **argv, char **env)
{
	st_var *var;
	char **eptr;
	uid_t rid;
	uid_t eid;
	uid_t sid;
	int envc;
	int i;

	/* BASIC variables */
	var = createVariable("$interpreter",VAR_STD,0,NULL);
	setValue(var->value,VAL_STR,INTERPRETER,0);

	var = createVariable("$copyright",VAR_STD,0,NULL);
	setValue(var->value,VAL_STR,COPYRIGHT,0);

	var = createVariable("$version",VAR_STD,0,NULL);
	setValue(var->value,VAL_STR,VERSION,0);

	var = createVariable("$build_date",VAR_STD,0,NULL);
	setValue(var->value,VAL_STR,BUILD_DATE,0);

	var = createVariable("$build_options",VAR_STD,0,NULL);
	setValue(var->value,VAL_STR,build_options,0);

	var = createVariable("$pi",VAR_STD,0,NULL);
	setValue(var->value,VAL_NUM,NULL,3.14159265358979323846);

	var = createVariable("$e",VAR_STD,0,NULL);
	setValue(var->value,VAL_NUM,NULL,2.71828182845904523536);

	var = createVariable("$true",VAR_STD,0,NULL);
	setValue(var->value,VAL_NUM,NULL,1);

	var = createVariable("$false",VAR_STD,0,NULL);
	setValue(var->value,VAL_NUM,NULL,0);

	angle_mode_var = createVariable("$angle_mode",VAR_STD,0,NULL);
	setValue(angle_mode_var->value,VAL_STR,"DEG",0);

	i = argc - basic_argc_start;
	var = createVariable("$argv",VAR_STD,1,&i);
	for(i=basic_argc_start;i < argc;++i)
		setValue(&var->value[i-basic_argc_start],VAL_STR,argv[i],0);

	/* Get enviroment variable count first then set */
	for(eptr=env,envc=0;*eptr;++eptr,++envc);
	var = createVariable("$env",VAR_STD,1,&envc);
	for(i=0;i < envc;++i) setValue(&var->value[i],VAL_STR,env[i],0);

	var = createVariable("$num_errors",VAR_STD,0,NULL);
	setValue(var->value,VAL_NUM,NULL,NUM_ERRORS);

	run_arg_var = createVariable("$run_arg",VAR_STD,0,NULL);
	setValue(run_arg_var->value,VAL_STR,cmdline_run_arg,0);

	indent_var = createVariable("$indent",VAR_STD,0,NULL);
	setValue(indent_var->value,VAL_NUM,NULL,indent_spaces);

	strict_mode_var = createVariable("$strict_mode",VAR_STD,0,NULL);
	setValue(strict_mode_var->value,VAL_NUM,NULL,strict_mode);

	interrupted_var = createVariable("$interrupted",VAR_STD,0,NULL);
	setValue(interrupted_var->value,VAL_NUM,NULL,0);

	/* Unix variables */
	pid_var = createVariable("$pid",VAR_STD,0,NULL);
	setValue(pid_var->value,VAL_NUM,NULL,getpid());

	ppid_var = createVariable("$ppid",VAR_STD,0,NULL);
	setValue(ppid_var->value,VAL_NUM,NULL,getppid());

	/* Get user ids - real, effective and saved set (only available on
	   linux) */
#ifdef __linux__
	getresuid(&rid,&eid,&sid);
#else
	rid = getuid();
	eid = geteuid();
	sid = -1;
#endif
	uid_var = createVariable("$uid",VAR_STD,0,NULL);
	setValue(uid_var->value,VAL_NUM,NULL,rid);

	var = createVariable("$euid",VAR_STD,0,NULL);
	setValue(var->value,VAL_NUM,NULL,eid);

	var = createVariable("$ssuid",VAR_STD,0,NULL);
	setValue(var->value,VAL_NUM,NULL,(int)sid);

	/* Do the same for groups */
#ifdef __linux__
	getresgid(&rid,&eid,&sid);
#else
	rid = getgid();
	eid = getegid();
	sid = -1;
#endif
	gid_var = createVariable("$gid",VAR_STD,0,NULL);
	setValue(gid_var->value,VAL_NUM,NULL,rid);

	var = createVariable("$egid",VAR_STD,0,NULL);
	setValue(var->value,VAL_NUM,NULL,eid);

	var = createVariable("$ssgid",VAR_STD,0,NULL);
	setValue(var->value,VAL_NUM,NULL,(int)sid);

	/* Error vars */
	error_var = createVariable("$error",VAR_STD,0,NULL);
	syserror_var = createVariable("$syserror",FALSE,0,NULL);
	reserror_var = createVariable("$reserror",FALSE,0,NULL);
	error_line_var = createVariable("$error_line",VAR_STD,0,NULL);
	setValue(error_line_var->value,VAL_NUM,NULL,0);

	prog_line_var = createVariable("$prog_line",VAR_STD,0,NULL);
	setValue(prog_line_var->value,VAL_NUM,NULL,0);

	break_line_var = createVariable("$break_line",VAR_STD,0,NULL);
	setValue(break_line_var->value,VAL_NUM,NULL,0);

	data_line_var = createVariable("$data_line",VAR_STD,0,NULL);
	setValue(data_line_var->value,VAL_NUM,NULL,0);

	eof_var = createVariable("$eof",VAR_STD,0,NULL);

	term_cols_var = createVariable("$term_cols",VAR_STD,0,NULL);
	setValue(term_cols_var->value,VAL_NUM,NULL,term_cols);
	
	term_rows_var = createVariable("$term_rows",VAR_STD,0,NULL);
	setValue(term_rows_var->value,VAL_NUM,NULL,term_rows);

	kilobyte_var = createVariable("$kilobyte",VAR_STD,0,NULL);
	setValue(kilobyte_var->value,VAL_NUM,NULL,kilobyte);

	processes_var = NULL;
	createProcessArray();
}




/*** Reset the volatile variables ***/
void resetSystemVariables()
{
	setValue(break_line_var->value,VAL_NUM,NULL,0);
	setValue(eof_var->value,VAL_NUM,NULL,0);
	setValue(error_var->value,VAL_NUM,NULL,0);
	setValue(syserror_var->value,VAL_NUM,NULL,0);
	setValue(
		angle_mode_var->value,
		VAL_STR,flags.angle_in_degrees ? "DEG" : "RAD",0);
	setValue(run_arg_var->value,VAL_STR,NULL,0);
	setValue(interrupted_var->value,VAL_NUM,NULL,0);
	createProcessArray();
}




void setTermVariables()
{
	if (term_cols_var)
	{
		setValue(term_cols_var->value,VAL_NUM,NULL,term_cols);
		setValue(term_rows_var->value,VAL_NUM,NULL,term_rows);
	}
}




/*** A load of common code used in command.c grouped together ***/
int getOrCreateTokenVariable(st_token *token)
{
	if ((token->var = getVariable(token->str))) return OK;

	/* Has to be created using DIM if in strict mode */
	if (strict_mode) return ERR_UNDEFINED_VAR_FUNC;
	if (!validVariableName(token->str)) return ERR_INVALID_VAR_NAME;
	token->var = createVariable(token->str,VAR_STD,0,NULL);
	return OK;
}




st_var *createVariable(char *name, int type, int index_cnt, int *index)
{
	st_var *var;
	int size;
	int i;

	assert((var = (st_var *)malloc(sizeof(st_var))));
	bzero(var,sizeof(st_var));

	assert((var->name = strdup(name)));
	var->type = type;

	switch(type)
	{
	case VAR_MAP:
		size = sizeof(st_keyval *) * (MAX_UCHAR + 1);

		/* Create these at runtime since we don't want to waste 
		   512 bytes of memory on non map variables */
		assert((var->first_keyval = (st_keyval **)malloc(size)));
		assert((var->last_keyval = (st_keyval **)malloc(size)));
		bzero(var->first_keyval,size);
		bzero(var->last_keyval,size);
		break;

	case VAR_STD:
	case VAR_MEM:
		var->arrsize = 1;
		for(i=0;i < index_cnt;++i) var->arrsize *= index[i];

		if (type == VAR_MEM)
		{
			/* For shared memory we just allocate the one value
			   structure regardless of array size */
			var->value = (st_value *)malloc(sizeof(st_value));
			assert(var->value);
			if (!initMemValue(var->value,var->arrsize)) return NULL;
			break;
		}
		
		if (var->arrsize)
		{
			var->value = (st_value *)malloc(var->arrsize * sizeof(st_value));
			assert(var->value);
			for(i=0;i < var->arrsize;++i) initValue(&var->value[i]);
		}

		var->index_cnt = index_cnt;
		for(i=0;i < index_cnt;++i) var->index[i] = index[i];
		break;

	default:
		assert(0);
	}
	
	addVarToList(var);

	if (findWatchVar(name) != -1)
	{
		printf("{DIM,%s%s,%s(%d)}\n",
			vartype[type],index_cnt ? "-ARRAY" : "",name,var->arrsize);
	}

	return var;
}




void addVarToList(st_var *var)
{
	int c = INDEX_CHAR(var->name);

	if (first_var[c])
	{
		last_var[c]->next = var;
		var->prev = last_var[c];
	}
	else first_var[c] = var;

	last_var[c] = var;
}




/*** Resize an array keeping the data already there. If resizing 2 or more
     dimensional array then data will probably move around compared to their 
     old index position. This can't be helped ***/
int reDimArray(st_var *var, int index_cnt, int *index)
{
	st_value *val;
	int arrsize;
	int i;

	arrsize = 1;
	for(i=0;i < index_cnt;++i) arrsize *= index[i];
	if (var->type == VAR_MEM || arrsize < var->arrsize)
		return ERR_CANT_REDIM;

	/* If its a string type and sval is pointing to str[] then after 
	   realloc str[] may have a different address so if sval is pointing to
	   str[] set to null and reset after */
	for(i=0;i < var->arrsize;++i)
	{
		val = &var->value[i];
		if (val->type == VAL_STR && val->sval == val->str)
			val->sval = NULL;
	}

	var->value = (st_value *)realloc(var->value,arrsize * sizeof(st_value));
	assert(var->value);

	/* Reset sval */
	for(i=0;i < var->arrsize;++i)
	{
		val = &var->value[i];
		if (val->type == VAL_STR && !val->sval) val->sval = val->str;
	}

	/* Init the new elements */
	for(i=var->arrsize;i < arrsize;++i) initValue(&var->value[i]);

	var->arrsize = arrsize;
	var->index_cnt = index_cnt;
	for(i=0;i < index_cnt;++i) var->index[i] = index[i];

	if (findWatchVar(var->name) != -1)
	{
		printf("{REDIM,%s%s,%s(%d)}\n",
			vartype[var->type],
			index_cnt ? "-ARRAY" : "",var->name,var->arrsize);
	}
	return OK;
}




st_var *getVariable(char *name)
{
	st_var *var;
	int c;

	c = INDEX_CHAR(name);
	for(var=first_var[c];var;var=var->next)
		if (!strcmp(var->name,name)) return var;
	return NULL;
}




/*** Get the indexes inside brackets. eg (1,2,3). "pc" should point to the
     first bracket ***/
int getVarIndexes(
	st_runline *runline, int *pc, st_value *icnt_or_key, int *index)
{
	st_value result;
	int err;
	int cnt;
	int pc2;

	pc2 = *pc;
	if (!IS_OP_TYPE(&runline->tokens[pc2],OP_L_BRACKET)) return ERR_SYNTAX;

	initValue(&result);
	clearValue(icnt_or_key);

	for(cnt=0,++pc2;;++pc2,++cnt)
	{
		if (cnt == MAX_INDEXES) return ERR_VAR_MAX_INDEX;

		if ((err = evalExpression(runline,&pc2,&result)) != OK)
			return err;

		if (!cnt && result.type == VAL_STR)
		{
			copyValue(icnt_or_key,&result);
			clearValue(&result);
			if (IS_OP_TYPE(&runline->tokens[pc2],OP_R_BRACKET))
			{
				*pc = pc2+1;
				return OK;
			}
			return ERR_SYNTAX;
		}
		if (result.type != VAL_NUM)
		{
			clearValue(&result);
			return ERR_INVALID_ARG;
		}
		if (result.dval < 1)
		{
			clearValue(&result);
			return ERR_VAR_INDEX_OOB;
		}
		index[cnt] = (int)result.dval;
		clearValue(&result);
		if (pc2 >= runline->num_tokens) return ERR_SYNTAX;

		if (IS_OP_TYPE(&runline->tokens[pc2],OP_R_BRACKET)) break;

		if (!IS_OP_TYPE(&runline->tokens[pc2],OP_COMMA)) 
			return ERR_SYNTAX;
	}
	*pc = pc2+1;
	setValue(icnt_or_key,VAL_NUM,NULL,cnt+1);
	return OK;
}




/*** Note that indexing in BASIC starts at 1, not zero */
int getVarValue(
	st_var *var, st_value *icnt_or_key, int *index, st_value *result)
{
	int err;
	int arrpos;
	int index_cnt;

	if (icnt_or_key && icnt_or_key->type == VAL_STR)
		return getMapValue(var,icnt_or_key->sval,result);

	if (var->type == VAR_MAP) return ERR_VAR_IS_MAP;

	index_cnt = (icnt_or_key ? icnt_or_key->dval : 0);
	if ((err = checkIndex(var,index_cnt,index,&arrpos)) != OK)
		return err;

	/* If no value set then default to numeric zero */
	if (var->value[arrpos].type == VAL_UNDEF) 
		setValue(result,VAL_NUM,NULL,0);
	else
		copyValue(result,&var->value[arrpos]);
	return OK;
}




/*** Set the variable whether map, array or standard ***/
int setVarValue(
	st_var *var,
	st_value *icnt_or_key, int *index, st_value *value, bool force)
{
	int err;
	int arrpos;
	int index_cnt;
	int watch_pos;
	int i;

	if (var->name[0] == '$' && !force) return ERR_VAR_READ_ONLY;
	if (icnt_or_key && icnt_or_key->type == VAL_STR)
		return setMapValue(var,icnt_or_key->sval,value);

	if (var->type == VAR_MAP) return ERR_VAR_IS_MAP;

	index_cnt = (icnt_or_key ? icnt_or_key->dval : 0);
	watch_pos = findWatchVar(var->name);

	if (var->index_cnt && !index_cnt)
	{
		/* If variable is an array but no index is given then set 
		   entire array to the value. This allows easy initialisation */
		for(i=0;i < var->arrsize;++i)
		{
			if (watch_pos != -1) printWatchLet(var,NULL,i,value);
			copyValue(&var->value[i],value);
		}
	}
	else
	{
		/* Set variable/array */
		if ((err = checkIndex(var,index_cnt,index,&arrpos)) != OK)
			return err;
		if (watch_pos != -1) printWatchLet(var,NULL,arrpos,value);
		copyValue(&var->value[arrpos],value);
	}
	return OK;
}




/*** Returns OK if index is a valid one and what its actual position is 
     in the 1 dimensional st_var->value array ***/
int checkIndex(st_var *var, int index_cnt, int *index, int *arrpos)
{
	int i;
	int ap;
	int mult;

	if (index_cnt && !var->index_cnt) return ERR_VAR_IS_NOT_ARRAY;
	if (index_cnt != var->index_cnt) return ERR_VAR_INDEX_OOB;

	ap = 0;
	mult = 1;
	for(i=index_cnt-1;i >= 0;--i)
	{
		assert(index[i] > 0);

		if (index[i] > var->index[i]) return ERR_VAR_INDEX_OOB;
		ap += (index[i] - 1) * mult;
		mult *= var->index[i];
	}
	*arrpos = ap;
	return OK;
}




int getMapValue(st_var *var, char *key, st_value *result)
{
	st_keyval *kv;

	if (var->type != VAR_MAP) return ERR_VAR_IS_NOT_MAP;

	if (!(kv = findKeyValue(var,key))) return ERR_KEY_NOT_FOUND;
	copyValue(result,&kv->value);
	return OK;
}




int setMapValue(st_var *var, char *key, st_value *value)
{
	st_keyval *kv;
	int c;

	if (var->type != VAR_MAP) return ERR_VAR_IS_NOT_MAP;

	/* If we don't have it yet create it */
	if (!(kv = findKeyValue(var,key)))
	{
		assert((kv = (st_keyval *)malloc(sizeof(st_keyval))));
		bzero(kv,sizeof(st_keyval));

		assert((kv->key = strdup(key)));
		initValue(&kv->value);

		c = key[0];
		if (var->last_keyval[c])
		{
			var->last_keyval[c]->next = kv;
			kv->prev = var->last_keyval[c];
		}
		else var->first_keyval[c] = kv;

		var->last_keyval[c] = kv;
		++var->map_cnt;
	}
	if (findWatchVar(var->name) != -1) printWatchLet(var,key,0,value);
	copyValue(&kv->value,value);
	return OK;
}




int deleteMapKeyValue(st_var *var, char *key)
{
	st_keyval *kv;
	int c;

	if (var->type != VAR_MAP) return ERR_VAR_IS_NOT_MAP;

	if (!(kv = findKeyValue(var,key))) return ERR_KEY_NOT_FOUND;

	c = key[0];
	if (kv == var->first_keyval[c])
		var->first_keyval[c] = kv->next;
	else
		kv->prev->next = kv->next;

	if (kv == var->last_keyval[c])
		var->last_keyval[c] = kv->prev;
	else 
		kv->next->prev = kv->prev;

	if (findWatchVar(var->name) != -1)
		printf("{DEL,%s(\"%s\")}\n",var->name,kv->key);

	free(kv->key);
	clearValue(&kv->value);
	free(kv);
	--var->map_cnt;
	assert(var->map_cnt >= 0);

	return OK;
}




st_keyval *findKeyValue(st_var *var, char *key)
{
	st_keyval *kv;

	for(kv=var->first_keyval[(int)key[0]];kv;kv=kv->next)
		if (!strcmp(kv->key,key)) return kv;
	return NULL;
}




/*** Returns TRUE if its a valid variable name else FALSE ***/
int validVariableName(char *name)
{
	char *s;

	if (!name || name[0] == '$') return FALSE;
	for(s=name;*s;++s) if (strchr(INVALID_NAME_CHARS,*s)) return FALSE;
	return TRUE;
}




/*** Delete a particular variable ***/
void deleteVariable(st_var *var, st_runline *runline)
{
	st_runline *rl;
	st_keyval *kv;
	st_keyval *next;
	st_defexp *exp;
	int c;
	int h;
	int i;
	
	removeVarFromList(var);

	/* Go through all program tokens and unset any var pointers */
	if (prog_first_line)
		rl = prog_first_line->first_runline;
	else if (runline)
	{
		rl = runline->parent->first_runline;
		runline = NULL;
	}
	else rl = NULL;

	for(h=0;h < 2 && rl;++h)
	{
		for(;rl;rl=rl->next) resetRunLineVariablePointer(rl,var);

		/* Now go through particular line we're in because it might be 
		   a direct line and so won't have been caught yet. If 
		   prog_first_line not set this will have been done in the
		   first loop */
		if (!runline) break;
		rl = runline->parent->first_runline;
	}

	/* Go through any orphaned runlines stored in defexps */
	for(exp=first_defexp;exp;exp=exp->next)
	{
		if (exp->runline->defexp)
			resetRunLineVariablePointer(exp->runline,var);
	}

	/* Delete variable data */
	switch(var->type)
	{
	case VAR_MAP:
		for(c=0;c <= MAX_UCHAR;++c)
		{
			for(kv=var->first_keyval[c];kv;kv=next)
			{
				next = kv->next;
				free(kv->key);
				clearValue(&kv->value);
			}
		}
		free(var->first_keyval);
		free(var->last_keyval);
		break;

	case VAR_MEM:
		/* Remove the shared memory segment */
		if (var->value[0].sval) shmdt(var->value[0].sval);
		break;

	case VAR_STD:
		for(i=0;i < var->arrsize;++i) clearValue(&var->value[i]);
		break;

	default:
		assert(0);
	}

	if (findWatchVar(var->name) != -1)
	{
		printf("{CLR,%s%s,%s}\n",
			vartype[var->type],
			var->index_cnt ? "-ARRAY" : "",var->name);
	}
	free(var->name);
	FREE(var->value);
	free(var);
}




void removeVarFromList(st_var *var)
{
	int c = INDEX_CHAR(var->name);

	/* Remove from linked list */
	if (var == first_var[c])
		first_var[c] = var->next;
	else
		var->prev->next = var->next;

	if (var == last_var[c])
		last_var[c] = var->prev;
	else 
		var->next->prev = var->prev;
}




void resetRunLineVariablePointer(st_runline *runline, st_var *var)
{
	int i;
	for(i=0;i < runline->num_tokens;++i)
	{
		if (runline->tokens[i].var == var)
			runline->tokens[i].var = NULL;
	}
}




/*** Delete all variables apart from system ones ***/
void deleteVariables(st_runline *runline)
{
	st_var *var;
	st_var *next;
	int c;

	for(c=0;c <= MAX_UCHAR;++c)
	{
		for(var=first_var[c];var;var=next)
		{
			next = var->next;

			/* Don't delete system variables. Can't check 'c' 
			   since sysvars are indexed by their 2nd character */
			if (var->name[0] != '$') deleteVariable(var,runline);
		}
	}
}




void renameVariable(st_var *var, char *new_name)
{
	/* The shortcut list uses the first letter of the name as an index */
	removeVarFromList(var);

	FREE(var->name);
	var->name = strdup(new_name);
	assert(var->name);

	addVarToList(var);
}




/*** Dump all variables to the console ***/
void dumpVariables(char *pat, bool dump_contents)
{
	st_var *var;
	int c;

	for(c=0;c <= MAX_UCHAR;++c)
	{
		for(var=first_var[c];var;var=var->next)
		{
			if (!pat || wildMatch(var->name,pat,TRUE))
				dumpVariable(var,dump_contents);
		}
	}
}




/*** Dump a specific variable ***/
void dumpVariable(st_var *var, bool dump_contents)
{
	st_keyval *kv;
	int *index;
	int ipos;
	int isize;
	int i;
	int c;

	printf("%-15s",var->name);

	switch(var->type)
	{
	case VAR_MAP:
		/* Get the entry count first */
		for(c=i=0;c <= MAX_UCHAR;++c)
		{
			for(kv=var->first_keyval[c];kv;kv=kv->next,++i);
		}
		printf(": MAP (%d entries)\n",i);
		if (!dump_contents) return;

		/* Show the entries */
		for(c=0;c <= MAX_UCHAR;++c)
		{
			for(kv=var->first_keyval[c];kv;kv=kv->next)
			{
				printf("     \"%s\" = ",kv->key);
				if (kv->value.type == VAL_STR)
					printf("\"%s\"\n",kv->value.sval);
				else if (IS_FLOAT(kv->value.dval))
					printf("%f\n",kv->value.dval);
				else
					printf("%ld\n",(long)kv->value.dval);
			}
		}
		return;

	case VAR_MEM:
		printf(": SHMEM (%d), id = %d, val = \"%s\"\n",
			var->value[0].shmlen,
			var->value[0].shmid,
			var->value[0].sval);
		return;

	case VAR_STD:
		break;

	default:
		assert(0);
	}

	if (!var->index_cnt)
	{
		/* Non array variable */
		if (var->value[0].type == VAL_STR)
			printf("= \"%s\"\n",var->value[0].sval);
		else if (IS_FLOAT(var->value[0].dval))
			printf("= %f\n",var->value[0].dval);
		else
			printf("= %ld\n",(long)var->value[0].dval);
		return;
	}

	/* Array */
	printf(": ARRAY (");

	/* Print index sizes */
	for(i=0;i < var->index_cnt;++i)
	{
		if (i)
			printf(",%d",var->index[i]);
		else
			printf("%d",var->index[i]);
	}
	puts(")");

	/* Can system vars like $argv with arrsize of 0 if no arguments
	   given */
	if (!dump_contents || !var->arrsize) return;

	/* Show array contents */
	isize = sizeof(int) * var->index_cnt;
	assert((index = (int *)malloc(isize)));
	for(i=0;i < var->index_cnt;++i) index[i] = 1;

	/* Ipos is actual index, index[] stores array indexes printed
	   out. eg: 1,4,3 */
	for(ipos=0;ipos < var->arrsize;++ipos)
	{
		/* Print index and value */
		printf("     (");
		for(i=0;i < var->index_cnt;++i)
		{
			if (i) printf(",");
			printf("%d",index[i]);
		}
		printf(") = ");

		if (var->value[ipos].type == VAL_STR)
			printf("\"%s\"\n",var->value[ipos].sval);
		else if (IS_FLOAT(var->value[ipos].dval))
			printf("%f\n",var->value[ipos].dval);
		else
			printf("%ld\n",(long)var->value[ipos].dval);

		/* Increment printed indexes */
		for(i=var->index_cnt-1;i >= 0;--i)
		{
			if (++index[i] > var->index[i]) index[i] = 1;
			else break;
		}
	}
	free(index);
}

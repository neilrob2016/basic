#include "globals.h"

#define MAX_PERMS 07777


#define FIND_FREE_STREAM(S) \
	for(S=0;S < MAX_STREAMS && stream[S];++S); \
	if (S == MAX_STREAMS) return ERR_MAX_STREAMS; \
	stream_flags[S] = 0;


#define MATCHPATH(T) \
	path = vallist[0].sval; \
	ret = ERR_INVALID_PATH; \
	if (hasWildCards(path)) ret = matchPath(T,path,matchpath,TRUE); \
	if (ret != OK && !copyStr(matchpath,path,PATH_MAX)) \
		return ERR_PATH_TOO_LONG;


int callFunction(st_runline *runline, int *pc, st_value *result)
{
	st_value vallist[MAX_FUNC_PARAMS];
	st_var *var[MAX_FUNC_PARAMS];
	st_token *token;
	st_token *start_token;
	int num_params;
	int func;
	int pc2;
	int pnum;
	int ptype;
	int err;
	int i;

	token = start_token = &runline->tokens[*pc];
	func = token->subtype;

	printTrace(runline->parent->linenum,"FUN",start_token->str);

	for(i=0;i < MAX_FUNC_PARAMS;++i)
	{
		var[i] = NULL;
		initValue(&vallist[i]);
	}

	/* If line is altered tokeniser creates a new line which will have
	   this fp_checked to FALSE */
	if (token->fp_checked) pc2 = *pc + 2;
	else
	{
		pc2 = *pc+1;

		/* Check for opening brackets */
		if (pc2 >= runline->num_tokens || 
		    !IS_OP_TYPE(&runline->tokens[pc2],OP_L_BRACKET))
			return ERR_SYNTAX;

		/* Check we have some params */
		++pc2;
		if (function[func].num_params)
		{
			if (IS_OP_TYPE(&runline->tokens[pc2],OP_R_BRACKET))
				return ERR_MISSING_PARAMS;
		}
		else if (!IS_OP_TYPE(&runline->tokens[pc2],OP_R_BRACKET))
			return ERR_TOO_MANY_PARAMS;
	}

	/* Get the function parameters */
	num_params = abs(function[func].num_params);
	for(pnum=0;num_params && pc2 < runline->num_tokens;++pc2)
	{
		/* < 0 means variadic function where all parameters must be the
		   same type and min number of params is abs(num_params) */
		if (function[func].num_params < 0)
		{
			if (pnum >= MAX_FUNC_PARAMS)
			{
				err = ERR_TOO_MANY_PARAMS;
				goto ERROR;
			}
			/* With variadics all arguments are the same type 
			   except for open() and mkdir() which have optional
			   numeric permissions parameters */
			if (pnum >= num_params)
			{
				if (func == FUNC_OPEN)
				{
					if (pnum > 2)
					{
						err = ERR_TOO_MANY_PARAMS;
						goto ERROR;
					}
					ptype = VAL_NUM;
				}
				else if (func == FUNC_MKDIR)
				{
					if (pnum > 1)
					{
						err = ERR_TOO_MANY_PARAMS;
						goto ERROR;
					}
					ptype = VAL_NUM;
				}
				else ptype = function[func].param_type[0];
			}
			else ptype = function[func].param_type[0];
		}
		else 
		{
			if (pnum >= num_params)
			{
				err = ERR_TOO_MANY_PARAMS;
				goto ERROR;
			}
			ptype = function[func].param_type[pnum];
		}

		/* Undefined value means we want a pointer to the variable
		   itself unless its pipes 2nd arg which needs to be a string */
		if (ptype == VAL_UNDEF && (func != FUNC_PIPE || !pnum))
		{
			token = &runline->tokens[pc2];
			if (token->type != TOK_VAR)
			{
				err = ERR_INVALID_ARG;
				goto ERROR;
			}
			if (!token->var && !(token->var = getVariable(token)))
			{
				err = ERR_UNDEFINED_VAR_OR_FUNC;
				goto ERROR;
			}
			var[pnum] = token->var;
			++pc2;
		}
		else
		{
			initValue(&vallist[pnum]);
			if ((err = evalExpression(runline,&pc2,&vallist[pnum])) != OK)
				goto ERROR;

			/* Check value is the correct type for the param */
			if (!(func == FUNC_PIPE && vallist[pnum].type == VAL_STR) &&
			    (ptype != VAL_BOTH && vallist[pnum].type != ptype))
			{
				err = ERR_INVALID_ARG;
				goto ERROR;
			}
		}
		++pnum;
		if (start_token->fp_checked && pnum == num_params) break;

		/* End? */
		if (IS_OP_TYPE(&runline->tokens[pc2],OP_R_BRACKET)) break;

		/* Need comma seperator */
		if (!IS_OP_TYPE(&runline->tokens[pc2],OP_COMMA)) 
		{
			err = ERR_SYNTAX;
			goto ERROR;
		}
	}

	if (pnum < num_params)
	{
		err = ERR_MISSING_PARAMS;
		goto ERROR;
	}

	/* If function is variadic put the number of parameters in the
	   result */
	if (function[func].num_params < 0)
	{
		initValue(result);
		setValue(result,VAL_NUM,NULL,pnum);
	}
	err = (*function[func].funcptr)(func,var,vallist,result);
	*pc = pc2+1;
	start_token->fp_checked = TRUE;
	/* Fall through */

	ERROR:
	for(i=0;i < pnum;++i) clearValue(&vallist[i]);
	return err;
}


/********************************* MATHS ************************************/


int funcAbs(int func, st_var **var, st_value *vallist, st_value *result)
{
	setValue(result,VAL_NUM,NULL,fabs(vallist[0].dval));
	return OK;
}




int funcSgn(int func, st_var **var, st_value *vallist, st_value *result)
{
	setValue(result,VAL_NUM,NULL,SGN(vallist[0].dval));
	return OK;
}




int funcRound(int func, st_var **var, st_value *vallist, st_value *result)
{
	setValue(result,VAL_NUM,NULL,round(vallist[0].dval));
	return OK;
}




int funcFloor(int func, st_var **var, st_value *vallist, st_value *result)
{
	setValue(result,VAL_NUM,NULL,floor(vallist[0].dval));
	return OK;
}




int funcCeil(int func, st_var **var, st_value *vallist, st_value *result)
{
	setValue(result,VAL_NUM,NULL,ceil(vallist[0].dval));
	return OK;
}




int funcSqrt(int func, st_var **var, st_value *vallist, st_value *result)
{
	if (vallist[0].dval < 0) return ERR_INVALID_ARG;
	setValue(result,VAL_NUM,NULL,sqrt(vallist[0].dval));
	return OK;
}




int funcPow(int func, st_var **var, st_value *vallist, st_value *result)
{
	setValue(result,VAL_NUM,NULL,pow(vallist[0].dval,vallist[1].dval));
	return OK;
}




int funcTrig1(int func, st_var **var, st_value *vallist, st_value *result)
{
	double radians;
	double val;

	radians = vallist[0].dval;
	if (flags.angle_in_degrees) radians /= DEGS_PER_RADIAN;

	switch(func)
	{
	case FUNC_SIN: val = sin(radians); break;
	case FUNC_COS: val = cos(radians); break;
	case FUNC_TAN: val = tan(radians); break;
	default      : assert(0);
	}

	setValue(result,VAL_NUM,NULL,val);
	return OK;
}




int funcTrig2(int func, st_var **var, st_value *vallist, st_value *result)
{
	double val;

	switch(func)
	{
	case FUNC_ASIN:
		if (vallist[0].dval < -1 || vallist[0].dval > 1)
			return ERR_OUT_OF_RANGE;
		val = asin(vallist[0].dval);
		break;

	case FUNC_ACOS:
		if (vallist[0].dval < -1 || vallist[0].dval > 1)
			return ERR_OUT_OF_RANGE;
		val = acos(vallist[0].dval);
		break;

	case FUNC_ATAN:
		val = atan(vallist[0].dval);
		break;

	default:
		assert(0);
	}

	if (flags.angle_in_degrees) val *= DEGS_PER_RADIAN;
	setValue(result,VAL_NUM,NULL,val);
	return OK;
}




int funcLog(int func, st_var **var, st_value *vallist, st_value *result)
{
	double val;

	if (vallist[0].dval <= 0) return ERR_INVALID_ARG;

	switch(func)
	{
	case FUNC_LOG2:
#ifdef NO_LOG2
		return ERR_UNAVAILABLE;
#else
		val = log2(vallist[0].dval);
#endif
		break;
	
	case FUNC_LOG10:
		val = log10(vallist[0].dval);
		break;

	case FUNC_LOG:
		val = log(vallist[0].dval);
		break;

	default:
		assert(0);
	}
	setValue(result,VAL_NUM,NULL,val);
	return OK;
}




int funcHypot(int func, st_var **var, st_value *vallist, st_value *result)
{
	setValue(result,VAL_NUM,NULL,hypot(vallist[0].dval,vallist[1].dval));
	return OK;
}




/*** Algo from http://www.graphics.stanford.edu/~seander/bithacks.html ***/
int funcParity(int func, st_var **var, st_value *vallist, st_value *result)
{
	u_long word;

 	word = (u_long)vallist[0].dval;
	word ^= (word >> 1);
	word ^= (word >> 2);
	if (sizeof(u_long) == 8)
	{
		/* 64 bit */
		word = (word & 0x1111111111111111) * 0x1111111111111111;	
		word = (word >> 60) & 1;
	}
	else
	{
		/* Assume 32 bit */
		word = (word & 0x11111111) * 0x11111111;
		word = (word >> 28) & 1;
	}

	setValue(result,VAL_NUM,NULL,word);
	return OK;
}




/*** Finds the max or min of N values ***/
int funcMaxMin(int func, st_var **var, st_value *vallist, st_value *result)
{
	double val;
	int i;

	val = vallist[0].dval;

	/* Number of args is passed in result->dval. Yes, hacky. */
	for(i=1;i < result->dval;++i)
	{
		if ((func == FUNC_MAX && vallist[i].dval > val) ||
		    (func == FUNC_MIN && vallist[i].dval < val))
			val = vallist[i].dval;
	}
	setValue(result,VAL_NUM,NULL,val);
	return OK;
}




/*** Exponent functions. Apparently more efficient that using pow().
     Eg: exp2(10) = pow(2,10) ***/
int funcExp(int func, st_var **var, st_value *vallist, st_value *result)
{
	double val;

	switch(func)
	{
	case FUNC_EXP:
		val = exp(vallist[0].dval);
		break;
	
	case FUNC_EXP2:
		val = exp2(vallist[0].dval);
		break;

	case FUNC_EXP10:
#ifdef __APPLE__
		val = __exp10(vallist[0].dval);
#else
		val = exp10(vallist[0].dval);
#endif
		break;

	default:
		assert(0);
	}
	setValue(result,VAL_NUM,NULL,val);
	return OK;
}


/******************************** VARIABLES ********************************/

int funcArrSize(int func, st_var **var, st_value *vallist, st_value *result)
{
	if (!var[0]->index_cnt) return ERR_VAR_IS_NOT_ARRAY;
	setValue(result,VAL_NUM,NULL,var[0]->arrsize);
	return OK;
}




int funcMapSize(int func, st_var **var, st_value *vallist, st_value *result)
{
	if (var[0]->type != VAR_MAP) return ERR_VAR_IS_NOT_MAP;
	setValue(result,VAL_NUM,NULL,var[0]->map_cnt);
	return OK;
}


/****************************** STRING/CHARACTER *****************************/

int funcAsc(int func, st_var **var, st_value *vallist, st_value *result)
{
	switch(strlen(vallist[0].sval))
	{
	case 0:
		/* Empty string is null character */
		setValue(result,VAL_NUM,NULL,0);
		break;

	case 1:
		setValue(result,VAL_NUM,NULL,(u_char)vallist[0].sval[0]);
		break;

	default:
		return ERR_INVALID_ARG;
	}
	return OK;
}




int funcChrStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	char text[2];

	if (vallist[0].dval < 0 || vallist[0].dval > 255)
		return ERR_OUT_OF_RANGE;

	text[0] = (char)vallist[0].dval;
	text[1] = 0;
	setValue(result,VAL_STR,text,0);

	return OK;
}




/*** Look for needle in the haystack starting at index (from 1).
     ie: instr("<haystack>","<needle>",pos) ***/
int funcInStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	st_value *haystack;
	st_value *needle;
	st_value *from;
	char *ptr;

	haystack = &vallist[0];
	needle = &vallist[1];
	from = &vallist[2];
	if (from->dval < 1) return ERR_INVALID_ARG;

	if (from->dval >= strlen(haystack->sval))
		setValue(result,VAL_NUM,NULL,-1);
	else if ((ptr = strstr(haystack->sval+(int)from->dval-1,needle->sval)))
		setValue(result,VAL_NUM,NULL,(int)(ptr - haystack->sval)+1);
	else
		setValue(result,VAL_NUM,NULL,-1);
	return OK;
}




int funcSubStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	char *str;
	char *sub;
	int start;
	int len;
	int len2;

	str = vallist[0].sval;
	start = vallist[1].dval;
	len = vallist[2].dval;

	if (start < 1 || len < 0) return ERR_INVALID_ARG;

	if (!(len2 = strlen(str)) || start >= len2 + 1)
	{
		setValue(result,VAL_STR,"",0);
		return OK;
	}

	if (len > len2 - start + 1) len = len2 - start + 1;
	assert((sub = (char *)malloc(len+1)));
	memcpy(sub,str+start-1,len);
	sub[len] = 0;

	setValue(result,VAL_STR,sub,0);
	free(sub);

	return OK;
}




int funcLeftStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	char *str;
	char c;
	int len;

	str = vallist[0].sval;
	if ((len = vallist[1].dval) < 0) return ERR_INVALID_ARG;

	if (strlen(str) <= len) setValue(result,VAL_STR,str,0);
	else
	{
		c = str[len];
		str[len] = 0;
		setValue(result,VAL_STR,str,0);
		str[len] = c;
	}
	return OK;
}




int funcRightStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	char *str;
	int len;
	int slen;

	str = vallist[0].sval;
	if ((len = vallist[1].dval) < 0) return ERR_INVALID_ARG;

	if ((slen = strlen(str)) <= len)
		setValue(result,VAL_STR,str,0);
	else
		setValue(result,VAL_STR,str+slen-len,0);
	return OK;
}




int funcStripStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	char *str;
	char *s;

	str = vallist[0].sval;

	switch(func)
	{
	case FUNC_STRIP:
		/* Fall through */

	case FUNC_LSTRIP:
		for(s=str;*s && isspace(*s);++s);
		if (func == FUNC_LSTRIP)
		{
			setValue(result,VAL_STR,s,0);
			break;
		}
		str = s;
		/* Fall through */

	case FUNC_RSTRIP:
		for(s=str+strlen(str)-1;s >= str && isspace(*s);--s);
		*(++s) = 0;	
		setValue(result,VAL_STR,str,0);
		break;

	default:
		assert(0);
	}
	return OK;
}




/*** Sets result to 1 if the value is numeric/string ***/
int funcIsType(int func, st_var **var, st_value *vallist, st_value *result)
{
	setValue(
		result,
		VAL_NUM,
		NULL,
		func == FUNC_ISNUM ? (vallist[0].type == VAL_NUM) : (vallist[0].type == VAL_STR));
	return OK;
}




/*** Sets result to 1 or 0 depending on whether the string contains a valid 
     number ***/
int funcIsNumStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	setValue(result,VAL_NUM,NULL,numType(vallist[0].sval) != NOT_NUM);
	return OK;
}




/*** Returns the base of the number stored in the string ***/
int funcNumStrBase(int func, st_var **var, st_value *vallist, st_value *result)
{
	static int base[NUM_NUM_TYPES] = { 0,2,8,16,10,10 };
	setValue(result,VAL_NUM,NULL,base[numType(vallist[0].sval)]);
	return OK;
}




int funcStrLen(int func, st_var **var, st_value *vallist, st_value *result)
{
	setValue(result,VAL_NUM,NULL,strlen(vallist[0].sval));
	return OK;
}




int funcHasKey(int func, st_var **var, st_value *vallist, st_value *result)
{
	if (var[0]->type != VAR_MAP) return ERR_VAR_IS_NOT_MAP;
	setValue(result,VAL_NUM,NULL,!!findKeyValue(var[0],vallist[1].sval));
	return OK;
}




int funcGetKeyStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	st_keyval *kv;
	int keynum;
	int cnt;
	int c;

	if (var[0]->type != VAR_MAP) return ERR_VAR_IS_NOT_MAP;
	if ((keynum = vallist[1].dval) < 1) return ERR_INVALID_ARG;
	if (keynum > var[0]->map_cnt) return ERR_KEY_NOT_FOUND;

	/* first_keyval is indexed by 1st character of string for speed so
	   need to go through ascii codes and linked list */
	for(c=0,cnt=0,kv=NULL;c <= MAX_UCHAR && cnt < keynum;++c)
	{
		for(kv=var[0]->first_keyval[c];kv;kv=kv->next)
			if (++cnt == keynum) break;
	}
	assert(cnt);
	setValue(result,VAL_STR,kv->key,0);
	return OK;
}




int funcBinStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	char str[129];
	u_long val;
	int bitcnt;
	int bitval;
	int bit;
	int strpos;
	bool add;

	if (!vallist[0].dval)
	{
		/* Special case as loop below will never hit a 1 */
		setValue(result,VAL_STR,"0",0);
		return OK;
	}

	/* Can't cast beyond a certain value */
	if (!(val = (u_long)vallist[0].dval)) return ERR_OUT_OF_RANGE;

	bitcnt = sizeof(u_long) * 8;
	add = FALSE;
	strpos = 0;

	for(bit=bitcnt-1;bit >= 0;--bit)
	{
		/* !! converts any non zero value to 1 */
		bitval = !!(val & ((u_long)1 << bit));

		/* Don't start adding to the string until we get first 1 */
		if (bitval) add = TRUE;
		if (add) 
		{
			str[strpos] = '0' + bitval;
			++strpos;
		}
	}
	str[strpos] = 0;
	setValue(result,VAL_STR,str,0);
	return OK;
}




int funcOctStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	char *numstr;
	u_long val;

	if (!(val = (u_long)vallist[0].dval) && vallist[0].dval)
		return ERR_OUT_OF_RANGE;

	assert(asprintf(&numstr,"%lo",val) != -1);
	setValue(result,VAL_STR,numstr,0);
	free(numstr);
	return OK;
}




int funcHexStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	char *numstr;
	u_long val;

	if (!(val = (u_long)vallist[0].dval) && vallist[0].dval)
		return ERR_OUT_OF_RANGE;

	assert(asprintf(&numstr,"%lX",val) != -1);
	setValue(result,VAL_STR,numstr,0);
	free(numstr);
	return OK;
}




int funcMatch(int func, st_var **var, st_value *vallist, st_value *result)
{
	int case_insensitive;

	/* Case flag must be 0 or 1 */
	case_insensitive = (int)vallist[2].dval;
	if (case_insensitive < 0 || case_insensitive > 1)
		return ERR_OUT_OF_RANGE;

	setValue(
		result,VAL_NUM,NULL,
		wildMatch(vallist[0].sval,vallist[1].sval,case_insensitive));
	return OK;
}




int funcRegMatch(int func, st_var **var, st_value *vallist, st_value *result)
{
	regex_t regex;
	regmatch_t pmatch;
	int ret;

	/* Compile the pattern */
	if (regcomp(&regex,vallist[1].sval,REG_EXTENDED)) return ERR_REGEX;

	/* Run it. Only looking for a full match, not partials */
	ret = (!regexec(&regex,vallist[0].sval,1,&pmatch,0) &&
	       !pmatch.rm_so && pmatch.rm_eo == vallist[0].slen);
	setValue(result,VAL_NUM,NULL,ret);
	regfree(&regex);
	return OK;
}




int funcToNum(int func, st_var **var, st_value *vallist, st_value *result)
{
	double num;

	switch(numType(vallist[0].sval))
	{
	case NUM_BIN:
		/* +2 to skip 0b which strtol doesn't recognise */
		num = strtol(vallist[0].sval+2,NULL,2);
		break;

	case NUM_OCT:
		num = strtol(vallist[0].sval,NULL,8);
		break;

	case NUM_HEX:
		num = strtol(vallist[0].sval,NULL,16);
		break;

	case NUM_INT:
	case NUM_FLOAT:
		num = atof(vallist[0].sval);
		break;

	default:
		return ERR_INVALID_ARG;
	}
	setValue(result,VAL_NUM,NULL,num);
	return OK;
}




int funcToStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	char *numstr;

	if (IS_FLOAT(vallist[0].dval))
		asprintf(&numstr,"%f",vallist[0].dval);
	else
		asprintf(&numstr,"%ld",(long)vallist[0].dval);
	setValue(result,VAL_STR,numstr,0);
	free(numstr);
	return OK;
}




int funcErrorStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	int err;

	err = vallist[0].dval;
	if (err < 0 || err >= NUM_ERRORS) return ERR_OUT_OF_RANGE;
	setValue(result,VAL_STR,error_str[err],0);
	return OK;
}




int funcSysErrorStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	if (vallist[0].dval < 0) return ERR_INVALID_ARG;
	setValue(result,VAL_STR,strerror((int)vallist[0].dval),0);
	return OK;
}




int funcResErrorStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	if (vallist[0].dval < 0) return ERR_INVALID_ARG;
	setValue(result,VAL_STR,(char *)hstrerror((int)vallist[0].dval),0);
	return OK;
}




int funcUpperLowerStr(
	int func, st_var **var, st_value *vallist, st_value *result)
{
	char *str;

	assert((str = strdup(vallist[0].sval)));
	setDirectStringValue(result,str);
	if (func == FUNC_UPPER)
		toUpperStr(str);
	else
		toLowerStr(str);
	return OK;
}




/*** Returns the element at the given position for ELEMENT$() or returns the
     count of elements for ELEMENTCNT() ***/
int funcElementStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	char *s;
	char *e;
	char c;
	int pos;
	int wcnt;

	if (func == FUNC_ELEMENTSTR)
	{
		if ((pos = vallist[1].dval) < 1) return ERR_INVALID_ARG;
		setValue(result,VAL_STR,"",0);
	}
	else
	{
		setValue(result,VAL_NUM,NULL,0);
		pos = 0;  /* Avoids gcc warning */
	}

	for(wcnt=0,s=vallist[0].sval;;s=e+1)
	{
		/* Skip whitespace */
		for(;*s && isspace(*s);++s);
		if (!*s) break;

		++wcnt;

		/* Find end of word/element */
		for(e=s+1;*e && !isspace(*e);++e);

		/* If we've reached the one we need then set result */
		if (func == FUNC_ELEMENTSTR && wcnt == pos)
		{
			c = *e;
			*e = 0;
			setValue(result,VAL_STR,s,0);
			*e = c;
			break;
		}
		if (!*e) break;
	}
	if (func == FUNC_ELEMENTCNT) setValue(result,VAL_NUM,NULL,wcnt);
	return OK;
}




/*** Replace 1 or more substrings inside a string. If REPLACEFR then start at
     a given position. Could have an end position too but too complicated. ***/
int funcReplaceStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	char *str;
	char *from_str;
	char *to_str;
	char *out;
	char *ptr;
	char *s;
	int len;
	int from_len;
	int from_pos;
	int to_len;
	int out_len;

	if ((from_len = vallist[1].slen) < 1) return ERR_INVALID_ARG;
	str = vallist[0].sval;
	from_str = vallist[1].sval;
	to_str = vallist[2].sval;
	to_len = vallist[2].slen;
	out = NULL;

	if (func == FUNC_REPLACEFR)
	{
		/* Start replacing from a given position */
		from_pos = (int)vallist[3].dval - 1;
		if (from_pos < 0) return ERR_INVALID_ARG;
		if (from_pos)
		{
			out_len = from_pos;
			assert((out = (char *)malloc(out_len + 1)));
			memcpy(out,str,out_len);
			out[out_len] = 0;
		}
		else
		{
			assert((out = strdup("")));
			out_len = 0;
		}
	}
	else
	{
		from_pos = 0;
		assert((out = strdup("")));
		out_len = 0;
	}

	for(s=str+from_pos;*s;s=(ptr + from_len))
	{
		if (!(ptr = strstr(s,from_str)))
		{
			out = (char *)realloc(out,out_len + strlen(s) + 1);
			assert(out);
			strcat(out,s);
			break;
		}
		len = (int)(ptr - s);
		out_len = out_len + len + to_len;
		assert((out = (char *)realloc(out,out_len + 1)));
		if (len) strncat(out,s,len);
		strcat(out,to_str);
	}

	setDirectStringValue(result,out);

	return OK;
}




/*** Insert string 2 into string 1 at the given position ***/
int funcInsertStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	char *str1;
	char *str2;
	char *out;
	int pos;

	str1 = vallist[0].sval;
	str2 = vallist[1].sval;
	pos = (int)vallist[2].dval;

	if (pos < 1) return ERR_INVALID_ARG;

	if (!vallist[1].slen)
	{
		setValue(result,VAL_STR,str1,0);
		return OK;
	}
	pos = (pos > vallist[0].slen) ? vallist[0].slen : pos - 1;
	asprintf(&out,"%.*s%s%s",pos,str1,str2,str1+pos);
	setDirectStringValue(result,out);
	return OK;
}




int funcFormatStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	int flen;
	int nlen;
	double val;
	char nstr[50];
	char *fmt;
	char *nptr;
	char *ndot;
	char *out;
	char *outdot;
	char *outptr;
	bool lpad;
	bool rpad;

	fmt = vallist[0].sval;
	val = vallist[1].dval;

	/* An exclamation mark at the start means don't pad number before 
	   decimal point */
	if (fmt[0] == '!')
	{
		lpad = FALSE;
		++fmt;
	}
	else lpad = TRUE;

	/* An exclamation at the end means don't pad digits after point */
	if ((flen = strlen(fmt)) && *(fmt + flen - 1) == '!')
	{
		rpad = FALSE;
		*(fmt + flen - 1) = 0;
	}
	else rpad = TRUE;

	/* Only want decimal point if we have something following the . */
	if (val - (long)val)
	{
		/* 15 points of precision seems to be the max that works */
		nlen = snprintf(nstr,sizeof(nstr)-1,"%.15f",val);

		/* Strip trailing zeros. eg: 123.45000 */
		for(nptr = nstr+nlen-1;nptr > nstr && *nptr == '0';--nptr)
			*nptr = 0;
	}
	else nlen = snprintf(nstr,sizeof(nstr)-1,"%ld",(long)val);

	nptr = ndot = strchr(nstr,'.');
	assert((out = strdup(fmt)));

	/* If we have a dot in the format work right from it */
	if ((outdot = strchr(out,'.')))
	{
		for(outptr=outdot;*outptr;++outptr)
		{
			if (!rpad && (!nptr || !*nptr))
			{
				*outptr = 0;
				break;
			}

			switch(*outptr)
			{
			case '#':
				*outptr = (nptr && *nptr) ? *nptr : '0';
				break;

			case '%':
				*outptr = (nptr && *nptr) ? *nptr : ' ';
				break;

			case '.':
				break;

			default:	
				continue;
			}
			if (nptr && *nptr) ++nptr;
		}
		outptr = outdot - 1;
	}
	else outptr = out + strlen(out) - 1;

	/* Work left from dot */
	nptr = ndot ? ndot-1 : (nstr + nlen - 1);

	for(;outptr >= out;--outptr)
	{
		if (!lpad && !nptr) 
		{
			setValue(result,VAL_STR,outptr+1,0);
			free(out);
			return OK;
		}

		switch(*outptr)
		{
		case '#':
			*outptr = (nptr ? *nptr : '0');
			break;

		case '%':
			*outptr = (nptr ? *nptr : ' ');
			break;

		default:	
			continue;
		}
		if (nptr > nstr)
			--nptr;
		else
			nptr = NULL;
	}
	setDirectStringValue(result,out);
	return OK;
}




/*** Return the max/min of N strings ***/
int funcMaxMinStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	char *sval;
	int cmp;
	int i;

	sval = vallist[0].sval;
	for(i=1;i < result->dval;++i)
	{
		cmp = strcmp(vallist[i].sval,sval);
		if ((func == FUNC_MAXSTR && cmp > 0) ||
		    (func == FUNC_MINSTR && cmp < 0)) sval = vallist[i].sval;
	}
	setValue(result,VAL_STR,sval,0);
	return OK;
}




/*** Left or right pad a string.
     Arguments are: <string>,<pad character>,<pad length> ***/
int funcPadStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	int slen;
	int plen;
	char *out;
	char *ptr;

	plen = (int)vallist[2].dval;
	if (plen < 0 || strlen(vallist[1].sval) != 1) return ERR_INVALID_ARG;

	/* If string length is >= pad length just return string as is */
	if ((slen = strlen(vallist[0].sval)) >= plen)
	{
		setValue(result,VAL_STR,vallist[0].sval,0);
		return OK;
	}

	assert((out = (char *)malloc(plen+1)));

	/* Copy in padding character */
	memset(out,vallist[1].sval[0],plen);
	out[plen] = 0;

	/* Overwrite appropriate part with input string */
	ptr = (func == FUNC_RPAD ? out : (out + plen - slen));
	memcpy(ptr,vallist[0].sval,slen);

	setDirectStringValue(result,out);
	return OK;
}


/******************************** FILESYSTEM *******************************/

/*** Open a file and return the BASIC stream number ***/
int funcOpen(int func, st_var **var, st_value *vallist, st_value *result)
{
	char matchpath[PATH_MAX+1];
	char *path;
	char *flagstr;
	mode_t perms = 0666;
	bool match = FALSE;
	int ret;
	int oflags;
	int fd;
	int snum;
	
	flagstr = vallist[1].sval;
	if (!strcmp(flagstr,"r"))
	{
		oflags = O_RDONLY;
		match = TRUE;
	}
	else if (!strcmp(flagstr,"o"))  /* Overwrite - don't truncate first */
		oflags = O_CREAT | O_WRONLY;
	else if (!strcmp(flagstr,"w"))
		oflags = O_CREAT | O_TRUNC | O_WRONLY;
	else if (!strcmp(flagstr,"a"))
		oflags = O_CREAT | O_APPEND | O_WRONLY;
	else if (!strcmp(flagstr,"rw"))
		oflags = O_RDWR;
	else return ERR_INVALID_ARG;

	/* If we have file permissions then sanity check */
	if (result->dval > 2)
	{
		perms = (mode_t)vallist[2].dval;
		if (perms < 0 || perms > MAX_PERMS)
			return ERR_INVALID_FILE_PERMS;
	}

	FIND_FREE_STREAM(snum);

	setValue(syserror_var->value,VAL_NUM,NULL,0);
	setValue(result,VAL_NUM,NULL,0);

	/* Only allow wildcard matching for read otherwise things get too
	   complicated. Eg: File might or might not already exist for append */
	path = vallist[0].sval;
	ret = ERR_INVALID_PATH;
	if (match && hasWildCards(path))
		ret = matchPath(S_IFREG,path,matchpath,TRUE);
	if (ret != OK && !copyStr(matchpath,path,PATH_MAX))
		return ERR_PATH_TOO_LONG;

	/* Don't throw an error if can't open file - just set syserror */
	if ((fd = open(matchpath,oflags,perms)) == -1)
	{
		setValue(syserror_var->value,VAL_NUM,NULL,errno);
		return OK;
	}
	stream[snum] = fd;

	/* +1 because streams start at 1 */
	setValue(result,VAL_NUM,NULL,snum+1);
	setValue(eof_var->value,VAL_NUM,NULL,0);
	setValue(syserror_var->value,VAL_NUM,NULL,0);

	return OK;
}




int funcOpenDir(int func, st_var **var, st_value *vallist, st_value *result)
{
	char matchpath[PATH_MAX+1];
	char *path;
	int snum;
	int ret;
	
	for(snum=0;snum < MAX_DIR_STREAMS;++snum) if (!dir_stream[snum]) break;
	if (snum == MAX_DIR_STREAMS) return ERR_MAX_DIR_STREAMS;

	setValue(syserror_var->value,VAL_NUM,NULL,0);

	MATCHPATH(S_IFDIR)

	/* As with open() don't just error if we can't open dir, return
	   zero and set system error */
	if (!(dir_stream[snum] = opendir(matchpath)))
	{
		setValue(syserror_var->value,VAL_NUM,NULL,errno);
		setValue(result,VAL_NUM,NULL,0);
		return OK;
	}

	setValue(result,VAL_NUM,NULL,MAX_STREAMS + snum + 1);
	setValue(eof_var->value,VAL_NUM,NULL,0);
	setValue(syserror_var->value,VAL_NUM,NULL,0);
	return OK;
}




int funcGetDirStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	char buff[PATH_MAX+1];
	char *dir;

	if (!(dir = getcwd(buff,PATH_MAX)))
	{
		/* Note VAL_STR - we return an empty string on error */
		setValue(syserror_var->value,VAL_NUM,NULL,errno);
		setValue(result,VAL_STR,NULL,0);
	}
	else setValue(result,VAL_STR,dir,0);

	return OK;
}




/*** Change directory ***/
int funcChDirStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	char matchpath[PATH_MAX+1];
	char *path;
	int ret;

	setValue(syserror_var->value,VAL_NUM,NULL,0);

	MATCHPATH(S_IFDIR)

	if (chdir(matchpath) == -1)
	{
		setValue(syserror_var->value,VAL_NUM,NULL,errno);
		setValue(result,VAL_STR,"",0);
	}
	else setValue(result,VAL_STR,matchpath,0);
	return OK;
}




/*** Create a new directory ***/
int funcMkDirStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	char matchpath[PATH_MAX+1];
	mode_t perms;
	char *path;
	char *ptr;
	int ret;

	/* If we have permissions then sanity check */
	if (result->dval > 1)
	{
		perms = (mode_t)vallist[1].dval;
		if (perms < 0 || perms > MAX_PERMS)
			return ERR_INVALID_FILE_PERMS;
	}
	else perms = 0777;

	setValue(syserror_var->value,VAL_NUM,NULL,0);

	path = vallist[0].sval;

	if (hasWildCards(path))
	{
		/* Don't try and match last path section because it doesn't
		   exist yet */
		if ((ptr = strrchr(path,'/')))
		{
			if (ptr == path) return OK;
			*ptr = 0;
		}
		ret = matchPath(S_IFDIR,path,matchpath,TRUE);
		if (ptr) *ptr = '/';
		if (ret != OK) return OK;
		if (ptr)
		{
			/* Don't allow wildcards in last section */
			if (hasWildCards(ptr)) return OK;
			if (!appendPath(matchpath,ptr))
				return ERR_PATH_TOO_LONG;
		}
	}
	else if (!copyStr(matchpath,path,PATH_MAX)) return ERR_PATH_TOO_LONG;

	if (mkdir(matchpath,perms) == -1)
	{
		setValue(syserror_var->value,VAL_NUM,NULL,errno);
		setValue(result,VAL_STR,"",0);
	}
	else setValue(result,VAL_STR,matchpath,0);
	return OK;
}




int funcSeek(int func, st_var **var, st_value *vallist, st_value *result)
{
	off_t offset;
	int snum;
	int fd;

	if (vallist[0].dval >= MAX_STREAMS) return ERR_DIR_SEEK;

	/* lseek() doesn't work in STDIN but allow it anyway for the sake
	   of consistency */
	snum = vallist[0].dval - 1;
	if (snum == -1) fd = STDIN;
	else
	{
		CHECK_STREAM(snum);
		fd = stream[snum];
	}

	/* If not -1 then offset is the offset from the start of the file */
	if ((offset = lseek(fd,(off_t)vallist[1].dval,SEEK_SET)) == -1)
	{
		setValue(syserror_var->value,VAL_NUM,NULL,errno);
		setValue(result,VAL_NUM,NULL,-1);
	}
	else
	{
		setValue(syserror_var->value,VAL_NUM,NULL,0);
		setValue(result,VAL_NUM,NULL,offset);
	}

	return OK;
}




/*** Delete a file system object. Returns the name of the object deleted else
     returns empty string if it failed ***/
int funcRmFSOStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	char matchpath[PATH_MAX+1];
	char *path;
	int ret;

	setValue(syserror_var->value,VAL_NUM,NULL,0);

	MATCHPATH(S_IFANY);

	switch(func)
	{
	case FUNC_RMFILE:
		/* Deletes files or links */
		ret = unlink(matchpath);
		break;
	case FUNC_RMDIR:
		/* Deletes directories */
		ret = rmdir(matchpath);
		break;
	case FUNC_RMFSO:
		/* Deletes anything */
		ret = remove(matchpath);
		break;
	default:
		assert(0);
	}
	
	if (ret == -1)
	{
		setValue(syserror_var->value,VAL_NUM,NULL,errno);
		setValue(result,VAL_STR,"",0);
	}
	else setValue(result,VAL_STR,matchpath,0);

	return OK;
}




int funcChModStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	char matchpath[PATH_MAX+1];
	char *path;
	mode_t perms;
	int ret;

	setValue(syserror_var->value,VAL_NUM,NULL,0);
	perms = (mode_t)vallist[1].dval;
	if (perms < 0 || perms > MAX_PERMS) return ERR_INVALID_FILE_PERMS;

	MATCHPATH(S_IFDIR);

	if (chmod(matchpath,perms) == -1)
	{
		setValue(syserror_var->value,VAL_NUM,NULL,errno);
		setValue(result,VAL_STR,"",0);
	}
	else setValue(result,VAL_STR,matchpath,0);
	return OK;
}




/*** Returns the previous umask in the result ***/
int funcUmask(int func, st_var **var, st_value *vallist, st_value *result)
{
	mode_t perms = (mode_t)vallist[0].dval;
	if (perms < 0 || perms > MAX_PERMS) return ERR_INVALID_FILE_PERMS;

	setValue(result,VAL_NUM,"",umask(perms));
	return OK;
}




int funcStatStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	struct stat fs;
	struct passwd *pwd;
	struct group *grp;
	char matchpath[PATH_MAX+1];
	char str[PATH_MAX+100];
	char *path;
	char *ftype;
	int ret;

	setValue(syserror_var->value,VAL_NUM,NULL,0);

	MATCHPATH(S_IFDIR);

	if (func == FUNC_STAT)
		ret = stat(matchpath,&fs);
	else
		ret = lstat(matchpath,&fs);

	/* If we can't stat a file return an empty string and set the 
	   system errror */
	if (ret == -1)
	{
		setValue(syserror_var->value,VAL_NUM,NULL,errno);
		setValue(result,VAL_STR,"",0);
		return OK;
	}

	/* Set filetype string */
	switch(fs.st_mode & S_IFMT) 
	{
	case S_IFREG : ftype = "FILE";    break;
	case S_IFDIR : ftype = "DIR";     break;
	case S_IFLNK : ftype = "LINK";    break;
	case S_IFIFO : ftype = "FIFO";    break;
	case S_IFBLK : ftype = "BLOCK";   break;
	case S_IFCHR : ftype = "CHAR";    break;
	case S_IFSOCK: ftype = "SOCKET";  break;

	default: ftype = "?";
	}

	pwd = getpwuid(fs.st_uid);
	grp = getgrgid(fs.st_gid);

	snprintf(str,sizeof(str)-1,"%s %04o %u %s %u %s %lu %lu %lu %lu %lu %s",
		ftype,
		(u_short)fs.st_mode & 0xFFF,
		fs.st_uid,
		pwd ? pwd->pw_name : "?",
		fs.st_gid,
		grp ? grp->gr_name : "?",
		(long)fs.st_size,
		(long)fs.st_atime,
		(long)fs.st_mtime,
		(long)fs.st_ctime,
		(long)fs.st_nlink,
		matchpath);

	setValue(syserror_var->value,VAL_NUM,NULL,0);
	setValue(result,VAL_STR,str,0);
	return OK;
}




/*** Sets result to 1 if we can read/write data from/to the file descriptor
     else 0. Not reliable for files. ***/
int funcCanRW(int func, st_var **var, st_value *vallist, st_value *result)
{
	struct timeval tv;
	fd_set mask;
	int snum;
	int ret;
	int fd;

	snum = vallist[0].dval - 1;
	if (snum == -1) fd = (func == FUNC_CANREAD ? STDIN : STDOUT);
	else
	{
		CHECK_STREAM(snum);
		fd = stream[snum];
	}

	/* Do a select() but return instantly */
	tv.tv_sec = 0;
	tv.tv_usec = 0;
	FD_ZERO(&mask);
	FD_SET(fd,&mask);

	if (func == FUNC_CANREAD)
		ret = select(FD_SETSIZE,&mask,0,0,&tv);
	else
		ret = select(FD_SETSIZE,0,&mask,0,&tv);
	if (ret == -1)
	{
		setValue(syserror_var->value,VAL_NUM,NULL,errno);
		setValue(result,VAL_NUM,NULL,-1);
	}
	else
	{
		setValue(syserror_var->value,VAL_NUM,NULL,0);
		setValue(result,VAL_NUM,NULL,!!FD_ISSET(fd,&mask));
	}
	return OK;
}




/*** Like CANREAD & CANWRITE but can multiplex and timeout on multiple
     streams. Analogue of C select() function. Arguments are:
     (read array, write array, timeout) ***/
int funcSelect(int func, st_var **var, st_value *vallist, st_value *result)
{
	struct timeval tv;
	struct timeval *tvp;
	st_value *value;
	fd_set mask[2];
	int snum;
	int ret;
	int fd;
	int i;
	int j;

	if (!var[0]->index_cnt) return ERR_VAR_IS_NOT_ARRAY;

	/* Must be 1D arrays */
	if (var[0]->index_cnt != 1 || var[1]->index_cnt != 1)
		return ERR_INVALID_ARRAY_TYPE;

	/* Check and set read streams, then write streams */
	for(j=0;j < 2;++j)
	{
		FD_ZERO(&mask[j]);

		for(i=0;i < var[j]->arrsize;++i)
		{
			value = &var[j]->value[i];
			if (value->type != VAL_NUM) return ERR_INVALID_ARG;
			if (value->dval < 0) continue;

			/* If value is zero assume STDIN/STDOUT, else its a 
			   stream */
			if (value->dval)
			{
				snum = value->dval - 1;
				CHECK_STREAM(snum);
				fd = stream[snum];
			}
			else fd = (j ? STDOUT : STDIN);

			FD_SET(fd,&mask[j]);
		}
	}

	if (vallist[2].dval < 0) tvp = NULL;
	else
	{
		tvp = &tv;
		tv.tv_sec = (int)vallist[2].dval;
		tv.tv_usec = (vallist[2].dval - tv.tv_sec) * 1000000;
	}

	switch((ret = select(FD_SETSIZE,&mask[0],&mask[1],0,tvp)))
	{
	case -1: /* Error */
	case 0: /* Timeout. */
		break;

	default:
		/* Go through arrays and set based on whether bits are
		   set in mask */
		for(j=0;j < 2;++j)
		{
			for(i=0;i < var[j]->arrsize;++i)
			{
				value = &var[j]->value[i];

				/* If the value is negative leave it alone */
				if (value->dval < 0) continue;

				if (value->dval)
					fd = stream[(int)value->dval - 1];
				else
					fd = (j ? STDOUT : STDIN);

				/* Set array return value as 0 or 1 */
				setValue(value,VAL_NUM,NULL,!!FD_ISSET(fd,&mask[j]));
			}
		}
	}
	setValue(syserror_var->value,VAL_NUM,NULL,ret == -1 ? errno : 0);
	setValue(result,VAL_NUM,NULL,ret);
	return OK;
}




/*** Returns a path matching the pattern given ***/
int funcPathStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	char matchpath[PATH_MAX+1];

	if (matchPath(S_IFANY,vallist[0].sval,matchpath,TRUE) == OK)
		setValue(result,VAL_STR,matchpath,0);
	else
		setValue(result,VAL_STR,NULL,0);
	return OK;
}


/******************************* DATE & TIME *******************************/

int funcTime(int func, st_var **var, st_value *vallist, st_value *result)
{
	setValue(result,VAL_NUM,NULL,getCurrentTime());
	return OK;
}




int funcDateStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	char *format = vallist[1].sval;
	time_t tm = (time_t)vallist[0].dval;
	char str[200];
	int ret;

	/* If the first character in the format is a 'U' then return the time
	   in UTC */
	if (format[0] == 'U')
		ret = strftime(str,200,format+1,gmtime(&tm));
	else
		ret = strftime(str,200,format,localtime(&tm));
	if (!ret) return ERR_INVALID_ARG;

	setValue(result,VAL_STR,str,0);
	return OK;
}




int funcDateToSecs(int func, st_var **var, st_value *vallist, st_value *result)
{
	char *format = vallist[1].sval;
	char *ret;
	struct tm tms;

	/* Check for UTC as above */
	if (format[0] == 'U')
		ret = strptime(vallist[0].sval,format+1,&tms);
	else
	{
		ret = strptime(vallist[0].sval,format,&tms);
		tms.tm_isdst = 1;
	}

	if (!ret) return ERR_INVALID_ARG;

	setValue(result,VAL_NUM,NULL,mktime(&tms));
	return OK;
}


/******************************** PROCESSES ********************************/

/*** Open a process as a file stream ***/
int funcPopen(int func, st_var **var, st_value *vallist, st_value *result)
{
	FILE *fp;
	int snum;

	FIND_FREE_STREAM(snum);

	if (!(fp = popen(vallist[0].sval,vallist[1].sval)))
	{
		/* Set syserror and return 0. Don't return an error because
		   its not a BASIC error */
		setValue(syserror_var->value,VAL_NUM,NULL,errno);
		setValue(result,VAL_NUM,NULL,0);
		return OK;
	}

	/* BASIC works with descriptors not pointers but we can't close the
	   descriptor yet as that kills the process */
	stream[snum] = fileno(fp);
	popen_fp[snum] = fp;

	/* +1 because streams start at 1 */
	setValue(result,VAL_NUM,NULL,snum+1);
	setValue(eof_var->value,VAL_NUM,NULL,0);
	setValue(syserror_var->value,VAL_NUM,NULL,0);

	return OK;
}




int funcFork(int func, st_var **var, st_value *vallist, st_value *result)
{
	pid_t pid;

	switch((pid = fork()))
	{
	case -1:
		/* Error */
		break;

	case 0:
		/* Child */
		flags.child_process = TRUE;
		setValue(pid_var->value,VAL_NUM,NULL,getpid());
		setValue(ppid_var->value,VAL_NUM,NULL,getppid());
		break;

	default:
		/* Parent */
		addProcessToList(pid);
		break;
	}
	setValue(syserror_var->value,VAL_NUM,NULL,pid == -1 ? errno : 0);
	setValue(result,VAL_NUM,NULL,pid);
	return OK;
}




int funcExec(int func, st_var **var, st_value *vallist, st_value *result)
{
	st_var *pvar;
	char **b_argv;
	int b_argc;
	int snum;

	pvar = var[1];

	/* Must be a size 2 array */
	if (pvar->type != VAR_STD || !pvar->index_cnt)
		return ERR_VAR_IS_NOT_ARRAY;
	if (pvar->arrsize != 2) return ERR_ARR_SIZE;

	/* Get the command line arguments from the string */
	if (!(b_argc = splitStringIntoArgv(vallist[0].sval,&b_argv)))
		return ERR_INVALID_ARG;

	/* Always use the lower value side of the socket pair */
	snum = (int)pvar->value[0].dval - 1;
	if (snum < 0 || snum >= MAX_STREAMS)
	{
		freeArgv(b_argc,b_argv);
		return ERR_INVALID_STREAM;
	}
	if (!stream[snum])
	{
		freeArgv(b_argc,b_argv);
		return ERR_STREAM_NOT_OPEN;
	}
	if (dup2(stream[snum],STDIN) != -1 &&
	    dup2(stream[snum],STDOUT) != -1 &&
	    dup2(stream[snum],STDERR) != -1) 
	{
		/* This'll only return if it fails */
		execvp(b_argv[0],b_argv);
	}

	setValue(syserror_var->value,VAL_NUM,NULL,errno);
	setValue(result,VAL_NUM,NULL,-1);
	freeArgv(b_argc,b_argv);
	return OK;
}




int funcWaitCheckStr(
	int func, st_var **var, st_value *vallist, st_value *result)
{
	pid_t pid;
	int status;
	char str[200];

	status = 0;
	if ((pid = waitpid(
		(int)vallist[0].dval,
		&status,func == FUNC_CHECKPID ? WNOHANG : 0)) == -1)
	{
		setValue(syserror_var->value,VAL_NUM,NULL,errno);
		if (errno == ECHILD)
			setValue(result,VAL_STR,"0 NOCHILD",0);
		else
			setValue(result,VAL_STR,"0 ERROR",0);
		return OK;
	}
	if (!pid) strcpy(str,"0 NOEXIT");
	else if (WIFEXITED(status))
		sprintf(str,"%d EXIT %d",pid,WEXITSTATUS(status));
	else if (WIFSIGNALED(status))
	{
		sprintf(str,"%d SIGNAL %d %s",
			pid,
			WTERMSIG(status),WCOREDUMP(status) ? "CORE" : "NOCORE");
	}
	else if (WIFSTOPPED(status))
		sprintf(str,"%d STOP %d",pid,WSTOPSIG(status));
	else if (WIFCONTINUED(status))
		sprintf(str,"%d CONT %d",pid,SIGCONT);
	else sprintf(str,"%d NODATA",pid);

	setValue(result,VAL_STR,str,0);
	setValue(syserror_var->value,VAL_NUM,NULL,0);
	removeProcessFromList(pid);
	return OK;
}




int funcKill(int func, st_var **var, st_value *vallist, st_value *result)
{
	if (vallist[1].dval < 1) return ERR_INVALID_ARG;

	if (kill((pid_t)vallist[0].dval,(int)vallist[1].dval) == -1)
	{
		setValue(syserror_var->value,VAL_NUM,NULL,errno);
		setValue(result,VAL_NUM,NULL,0);
	}
	else
	{
		setValue(syserror_var->value,VAL_NUM,NULL,0);
		setValue(result,VAL_NUM,NULL,1);
	}
	return OK;
}




int funcPipe(int func, st_var **var, st_value *vallist, st_value *result)
{
	int fd[2];
	int snum1;
	int snum2;

	if (!var[0]->index_cnt) return ERR_VAR_IS_NOT_ARRAY;
	if (var[0]->index_cnt != 1 || var[0]->arrsize != 2)
		return ERR_INVALID_ARRAY_TYPE;

	/* Find 2 free streams */
	FIND_FREE_STREAM(snum1);
	for(snum2=snum1+1;snum2 < MAX_STREAMS && stream[snum2];++snum2);
	if (snum2 == MAX_STREAMS) return ERR_MAX_STREAMS;

	/* Check flag */
	if (result->dval > 1)
	{
		if (!strcasecmp(vallist[1].str,"no_wait_nl"))
		{
			stream_flags[snum1] = SFLAG_NO_WAIT_NL;
			stream_flags[snum2] = SFLAG_NO_WAIT_NL;
		}
		else return ERR_INVALID_ARG;
	}

	/* Use socketpair() because it creates 2 way descriptors. pipe() only
	   creates 1 way */
	if (socketpair(AF_LOCAL,SOCK_STREAM,0,fd) == -1)
	{
		setValue(syserror_var->value,VAL_NUM,NULL,errno);
		setValue(result,VAL_NUM,NULL,0);
		return OK;
	}
	stream[snum1] = fd[0];
	stream[snum2] = fd[1];

	/* +1 because streams start at 1 */
	setValue(&var[0]->value[0],VAL_NUM,NULL,snum1+1);
	setValue(&var[0]->value[1],VAL_NUM,NULL,snum2+1);

	setValue(syserror_var->value,VAL_NUM,NULL,0);
	setValue(result,VAL_NUM,NULL,1);
	return OK;
}


/******************************** NETWORK **********************************/

int funcConnect(int func, st_var **var, st_value *vallist, st_value *result)
{
	struct sockaddr_in con_addr;
	struct hostent *hp;
	int port;
	int sock;
	int snum;
	int on;
	char *addr;
	char *ptr;

	setValue(reserror_var->value,VAL_NUM,NULL,0);

	addr = vallist[0].sval;
	if ((ptr = strchr(addr,':')))
	{
		if ((port = atoi(ptr+1)) < 1) return ERR_INVALID_PORT;
		*ptr = 0;
	}
	else port = 23;

	/* Find free stream */
	FIND_FREE_STREAM(snum);

	if (result->dval > 1)
	{
		if (!strcasecmp(vallist[1].str,"no_wait_nl"))
			stream_flags[snum] = SFLAG_NO_WAIT_NL;
		else
			return ERR_INVALID_ARG;
	}

	if ((sock = socket(AF_INET,SOCK_STREAM,0)) == -1)
		return ERR_SOCKET;	

	/* Disble Nagle */
	on = 1;
	setsockopt(sock,SOL_SOCKET,SO_BROADCAST,&on,sizeof(on));

	/* Send keepalive */
	setsockopt(sock,SOL_SOCKET,SO_KEEPALIVE,&on,sizeof(on));

	bzero(&con_addr,sizeof(con_addr));
	con_addr.sin_family = AF_INET;
	con_addr.sin_port = htons(port);

	/* If -1 then have DNS name, not numeric address */
	if ((con_addr.sin_addr.s_addr = inet_addr(addr)) == -1)
	{
		/* Don't error if can't resolve, want to return 0 from BASIC
		   function */
		if (!(hp = gethostbyname(addr)))
		{
			close(sock);
			setValue(reserror_var->value,VAL_NUM,NULL,h_errno);
			setValue(result,VAL_NUM,NULL,0);
			return OK;
		}
		memcpy((char *)&con_addr.sin_addr.s_addr,
		       hp->h_addr_list[0],hp->h_length);
	}

	/* Connect to server */
	if (connect(sock,(struct sockaddr *)&con_addr,sizeof(con_addr)) == -1)
	{
		setValue(syserror_var->value,VAL_NUM,NULL,errno);
		setValue(result,VAL_NUM,NULL,0);
		close(sock);
		return OK;
	}

	stream[snum] = sock;

	setValue(result,VAL_NUM,NULL,snum+1);
	setValue(eof_var->value,VAL_NUM,NULL,0);
	setValue(syserror_var->value,VAL_NUM,NULL,0);

	return OK;
}




int funcListen(int func, st_var **var, st_value *vallist, st_value *result)
{
	struct sockaddr_in bind_addr;
	int snum;
	int sock;
	int port;
	int qlen;
	int on;

	if ((port = vallist[0].dval) < 1) return ERR_INVALID_PORT;
	if ((qlen = vallist[1].dval) < 1) return ERR_INVALID_ARG;

	FIND_FREE_STREAM(snum);

	if ((sock = socket(AF_INET,SOCK_STREAM,0)) == -1)
		return ERR_SOCKET;

	on = 1;
	setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&on,sizeof(on));

	bzero(&bind_addr,sizeof(bind_addr));
	bind_addr.sin_family = AF_INET;
	bind_addr.sin_port = htons(port);
	bind_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sock,(struct sockaddr *)&bind_addr,sizeof(bind_addr)) == -1 ||
	    listen(sock,qlen) == -1)
	{
		close(sock);
		return ERR_SOCKET;
	}

	stream[snum] = sock;
	setValue(result,VAL_NUM,NULL,snum+1);
	setValue(eof_var->value,VAL_NUM,NULL,0);

	return OK;
}




int funcAccept(int func, st_var **var, st_value *vallist, st_value *result)
{
	struct sockaddr_in addr;
	socklen_t size;
	int lsnum;
	int asnum;
	int accept_sock;

	/* Stream contains the listen socket */
	lsnum = vallist[0].dval - 1;
	CHECK_STREAM(lsnum);
	size = sizeof(addr);
	if ((accept_sock = accept(stream[lsnum],(struct sockaddr *)&addr,&size)) == -1)
		return ERR_SOCKET;

	FIND_FREE_STREAM(asnum);
	stream[asnum] = accept_sock;
	setValue(result,VAL_NUM,NULL,asnum+1);
	setValue(eof_var->value,VAL_NUM,NULL,0);

	return OK;
}




int funcGetIPStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	struct sockaddr_in addr;
	socklen_t size;
	int snum;

	snum = vallist[0].dval - 1;
	CHECK_STREAM(snum);

	size = sizeof(addr);
	if (getpeername(stream[snum],(struct sockaddr *)&addr,&size) == -1)
		return ERR_SOCKET;

	setValue(result,VAL_STR,inet_ntoa(addr.sin_addr),0);
	return OK;
}




int funcIP2HostStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	in_addr_t addr;
	struct hostent *host;

	if ((addr = inet_addr(vallist[0].sval)) == INADDR_NONE)
	{
		setValue(reserror_var->value,VAL_NUM,NULL,0);
		return ERR_INVALID_ARG;
	}

	host = gethostbyaddr(&addr,4,AF_INET);
	setValue(reserror_var->value,VAL_NUM,NULL,h_errno);
	setValue(result,VAL_STR,host ? host->h_name : "",0);
	return OK;
}




/*** Return all the IP addresses of the host in a space seperated string ***/
int funcHost2IPStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	struct hostent *he; 
	char *out;
	char *ip;
	char **ptr;
	int len;

	if ((he = gethostbyname(vallist[0].sval)) && 
	     he->h_addrtype == AF_INET)
	{
		len = 0;
		out = NULL;

		/* Concatenate all addresses */
		for(ptr=he->h_addr_list;*ptr;++ptr)
		{
			if (ptr != he->h_addr_list) strcat(out," ");

			ip = inet_ntoa(*((struct in_addr *)*ptr));
			len += strlen(ip) + 2;
			assert((out = (char *)realloc(out,len)));

			if (ptr == he->h_addr_list) 
				strcpy(out,ip);
			else
				strcat(out,ip);
		}
		setDirectStringValue(result,out);
	}
	else setValue(result,VAL_STR,"",0);

	setValue(reserror_var->value,VAL_NUM,NULL,h_errno);
	return OK;
}



/****************************** USER & GROUP ******************************/

int funcGetUserStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	struct passwd *pwd;
	char *str;

	if (func == FUNC_GETUSERBYID)
		pwd = getpwuid((uid_t)vallist[0].dval);
	else
		pwd = getpwnam(vallist[0].sval);

	if (pwd)
	{
		asprintf(&str,"%u %s %u %s %s %s",
			pwd->pw_uid,
			pwd->pw_name,
			pwd->pw_gid,
			pwd->pw_dir,
			pwd->pw_shell,
			pwd->pw_gecos);
		setValue(result,VAL_STR,str,0);
		free(str);
	}
	else setValue(result,VAL_STR,"",0);

	return OK;
}




int funcGetGroupStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	struct group *grp;
	char *str;
	char **ptr;
	int len;

	if (func == FUNC_GETGROUPBYID)
		grp = getgrgid((gid_t)vallist[0].dval);
	else
		grp = getgrnam(vallist[0].sval);

	if (grp)
	{
		len = asprintf(&str,"%u %s",grp->gr_gid,grp->gr_name);
		assert(len != -1);

		/* Add group members */
		for(ptr=grp->gr_mem;*ptr;++ptr)
		{
			len = len + strlen(*ptr) + 2;
			str = (char *)realloc(str,len);
			assert(str);
			strcat(str," ");
			strcat(str,*ptr);
		}
		setValue(result,VAL_STR,str,0);
		free(str);
	}
	else setValue(result,VAL_STR,"",0);

	return OK;
}


/***************************** ENVIROMENT VARS ******************************/

int funcGetEnvStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	setValue(result,VAL_STR,getenv(vallist[0].sval),0);
	return OK;
}




int funcSetEnv(int func, st_var **var, st_value *vallist, st_value *result)
{
	if (setenv(vallist[0].sval,vallist[1].sval,1) == -1)
	{
		setValue(syserror_var->value,VAL_NUM,NULL,errno);
		setValue(result,VAL_NUM,NULL,0);
	}
	else setValue(result,VAL_NUM,NULL,1);

	return OK;
}


/********************************* SYSTEM **********************************/

int funcSystem(int func, st_var **var, st_value *vallist, st_value *result)
{
	/* system() returns 0 for ok, -1 or 127 for fail */
	if (system(vallist[0].sval))
	{
		setValue(syserror_var->value,VAL_NUM,NULL,errno);
		setValue(result,VAL_NUM,NULL,0);
	}
	else setValue(result,VAL_NUM,NULL,1);
	return OK;
}




int funcSysInfoStr(int func, st_var **var, st_value *vallist, st_value *result)
{
	struct utsname uts;
	char *res;

	if (uname(&uts) == -1)
	{
		setValue(syserror_var->value,VAL_NUM,NULL,errno);
		res = NULL;
	}
	else if (!strcasecmp(vallist[0].sval,"OS"))
		res = uts.sysname;
	else if (!strcasecmp(vallist[0].sval,"RELEASE"))
		res = uts.release;
	else if (!strcasecmp(vallist[0].sval,"VERSION"))
		res = uts.version;
	else if (!strcasecmp(vallist[0].sval,"HOSTNAME"))
		res = uts.nodename;
	else if (!strcasecmp(vallist[0].sval,"CPU"))
		res = uts.machine;
	else return ERR_INVALID_ARG;

	setValue(result,VAL_STR,res,0);
	return OK;
}


/****************************** RANDOM NUMBERS ******************************/

/*** Return a floating from 0 to 1 ***/
int funcRand(int func, st_var **var, st_value *vallist, st_value *result)
{
	setValue(result,VAL_NUM,NULL,(double)random() / RAND_MAX);
	return OK;
}




/*** Return an integer from 0 to maxval inclusive ***/
int funcRandom(int func, st_var **var, st_value *vallist, st_value *result)
{
	long maxval;
	if ((maxval = (long)vallist[0].dval) < 1) return ERR_INVALID_ARG;
	setValue(result,VAL_NUM,NULL,random() % (maxval + 1));
	return OK;
}


/****************************** MISCELLANIOUS ******************************/

/*** First argument is string to encrypt, 2nd is salt, 3rd is encryption
     type. MacOS only supports DES for this function ***/
int funcCryptStr(int func, st_var **var, st_value *vallist, st_value *result)
{
#ifdef NO_CRYPT
	return ERR_UNAVAILABLE;
#else
	static char *etype[4] = { "DES","MD5","SHA256","SHA512" };
	char *salt;
	int add;
	int i;

	/* Must have a salt of 2 characters */
	if (strlen(vallist[1].sval) != 2) return ERR_INVALID_ARG;

	for(i=0;i < 4;++i) if (!strcasecmp(etype[i],vallist[2].sval)) break;

#ifdef DES_ONLY
	/* The non DES salt codes don't work in MacOS and will use DES so 
	   don't allow them */
	if (i && i < 4) return ERR_ENCRYPTION_NOT_SUPPORTED;
#endif
	add = 6;

	switch(i)
	{
	case 0: /* DES */
		assert((salt = strdup(vallist[1].sval)));
		add = 2;
		break;

	case 1: /* MD5 */
		assert(asprintf(&salt,"$1$%s$",vallist[1].sval) != -1);
		break;

	case 2: /* SHA256 */
		assert(asprintf(&salt,"$5$%s$",vallist[1].sval) != -1);
		break;

	case 3: /* SHA512 */
		assert(asprintf(&salt,"$6$%s$",vallist[1].sval) != -1);
		break;

	default:
		return ERR_INVALID_ARG;
	}

	setValue(result,VAL_STR,crypt(vallist[0].sval,salt)+add,0);
	free(salt);
	return OK;
#endif
}




/*** See if any DATA statements left to read. This function does not take 
     AUTORESTORE into account so will return 0 if at end even if AUTORESTORE
     is set. ***/
int funcHaveData(int func, st_var **var, st_value *vallist, st_value *result)
{
	st_runline *runline;
	int res = 0;

	if (data_runline) 
	{
		/* See if we're in the middle of DATA statement */
		if (data_pc < data_runline->num_tokens) res = 1;
		else for(runline=data_runline->next;
		         runline;runline=runline->next)
		{
			/* Find the next DATA command */
			if (runline->num_tokens > 1 &&
			    IS_COM_TYPE(&runline->tokens[0],COM_DATA))
			{
				res = 1;
				break;
			}
		}
	}
	setValue(result,VAL_NUM,NULL,res);
	return OK;
}

#include "globals.h"


void initValue(st_value *val)
{
	val->type = VAL_NUM;
	val->shmid = -1;
	val->slen= 0;
	val->str[0] = 0;
	val->sval = NULL;
	val->dval = 0;
}




bool initMemValue(st_value *val, int size)
{
	val->type = VAL_STR;
	val->dval = 0;
	val->str[0] = 0;
	val->slen = 0;
	val->sval = NULL;
	val->shmlen = size;

	/* Allocate shared memory. Use pointer as key. size+1 for \0 */
	if ((val->shmid = shmget((key_t)val,size+1,IPC_CREAT | 0666)) < 0)
		return FALSE;
	if ((val->sval = shmat(val->shmid,NULL,0)) == (char *)-1)
		return FALSE;

	val->sval[0] = 0;
	val->sval[size] = 0;

	/* Mark segment to be deleted once all processes exit if not done
	   explicitly */
	shmctl(val->shmid,IPC_RMID,0);

	return TRUE;
}




void clearValue(st_value *val)
{
	if (val->shmid != -1) val->sval[0] = 0;
	else
	{
		if (val->type == VAL_STR)
		{
			if (val->sval != val->str) free(val->sval);
		}
		initValue(val);
	}
	val->slen = 0;
}




void setValue(st_value *val, int type, char *sval, double dval)
{
	int len;

	clearValue(val);
	if (type == VAL_STR)
	{
		if (!sval)
		{
			sval = "";
			len = 0;
		}
		else len = strlen(sval);

		if (val->shmid != -1)
		{
			if (len > val->shmlen) len = val->shmlen;
			strncpy(val->sval,sval,len);
			val->sval[len] = 0;
		}
		/* If we can fit the string in the fixed array then do it 
		   otherwise strdup it */
		else if (len > VAL_FIXED_STR_LEN)
		{
			val->str[0] = 0;
			assert((val->sval = strdup(sval)));
		}
		else
		{
			strcpy(val->str,sval);
			val->sval = val->str;
		}
		val->slen = len;
	}
	else if (type == VAL_NUM)
	{
		/* If shared memory write the number as a string since we 
		   can't point dval at the memory (Ok, I could , but it would
		   make the rest of the interpreter far too complex) */
		if (val->shmid != -1) 
		{
			snprintf(val->sval,val->shmlen+1,"%f",dval);
			val->slen = strlen(val->sval);
			type = VAL_STR;
		}
		else val->dval = dval;
	}
	else assert(0);

	val->type = type;
}




/*** No copying, just use pointer passed. Ignore str[] */
void setDirectStringValue(st_value *val, char *sval)
{
	clearValue(val);
	assert(sval);

	if (val->shmid != -1)
	{
		/* Have to copy for shared mem */
		setValue(val,VAL_STR,sval,0);

		/* Calling function assumes we're using this memory so won't
		   free it itself */
		free(sval);
	}
	else
	{
		val->type = VAL_STR;
		val->sval = sval;
	}
	val->slen = strlen(sval);
}




void copyValue(st_value *to, st_value *from)
{
	setValue(to,from->type,from->sval,from->dval);
}




/*** Append val2.sval to val1.sval ***/
void appendStringValue(st_value *val1, st_value *val2)
{
	char *str1;
	char *str2;
	char *newstr;

	assert(val1->type == VAL_STR && val2->type == VAL_STR);
	if (!val2->slen) return;

	str1 = val1->sval;
	str2 = val2->sval;
	val1->dval = 0;

	if (str1)
	{
		/* Simpler but slower
		assert(asprintf(&newstr,"%s%s",str1,str2) != -1);
		*/
		assert((newstr = (char *)malloc(val1->slen + val2->slen + 1)));
		strcpy(newstr,str1);
		strcat(newstr,str2);

		setDirectStringValue(val1,newstr);
	}
	else setValue(val1,VAL_STR,str2,0);
}




/*** Remove all instances of val2.sval from val1.sval ***/
void subtractStringValue(st_value *val1, st_value *val2)
{
	char *ptr;

	assert(val1->type == VAL_STR && val2->type == VAL_STR);
	if (!val2->slen) return;

	while((ptr = strstr(val1->sval,val2->sval)))
	{
		/* Move chars down to overwrite current data */
		for(;*ptr;++ptr) *ptr = *(ptr + val2->slen);
		val1->slen -= val2->slen;
	}
	assert(val1->slen >= 0);
}




/*** Multiply the string by given count ***/
void multStringValue(st_value *val, int cnt)
{
	char *out;
	int i;

	if (!cnt || !val->slen)
	{
		setValue(val,VAL_STR,"",0);
		return;
	}
	assert((out = (char *)malloc(val->slen * cnt + 1)));
	out[0] = 0;

	for(i=0;i < cnt;++i) strcat(out,val->sval);

	setDirectStringValue(val,out);
}




bool trueValue(st_value *val)
{
	return ((val->type == VAL_NUM && val->dval) ||
	        (val->type == VAL_STR && val->sval && val->sval[0]));
}

#include "globals.h"

static char *createWord(char *s, char *e)
{
	char *word;
	size_t len;

	len = (size_t)e - (size_t)s + 1;
	word = (char *)malloc(len + 1);
	assert(word);
	memcpy(word,s,len);
	word[len] = 0;
	return word;
}



	
static int addWord(char *word, int b_argc, char ***b_argv)
{
	char **ba = *b_argv;
	ba = (char **)realloc(ba,sizeof(char *) * (b_argc + 2));
	assert(ba);
	ba[b_argc] = word;
	++b_argc;
	ba[b_argc] = NULL;
	*b_argv = ba;
	return b_argc;
}




int splitStringIntoArgv(char *str, char ***b_argv)
{
	char *quotes = "\"'`";
	char *s;
	char *e;
	int b_argc;

	*b_argv = NULL;
	b_argc = 0;

	for(e=s=str;*s && *e;)
	{
		if (isspace(*s))
		{
			++s;
			continue;
		}

		/* If we've hit quotes find the end of the quoted string */
		if (strchr(quotes,*s))
		{
			for(e=s+1;*e && *e != *s;++e);
			b_argc = addWord(createWord(s,e),b_argc,b_argv);
			s = e + 1;
			continue;
		}

		/* Find the end of the word */
		for(e=s+1;*e && !strchr(quotes,*e) && !isspace(*e);++e);
		b_argc = addWord(createWord(s,e-1),b_argc,b_argv);
		s = e;
		if (!strchr(quotes,*e)) ++s;
	}
	return b_argc;
}




void freeArgv(int b_argc, char **b_argv)
{
	int i;
	for(i=0;i < b_argc;++i) free(b_argv[i]);
	free(b_argv);
}

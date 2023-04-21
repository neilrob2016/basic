#include "globals.h"

void initDefMods(void)
{
	bzero(defmod,sizeof(defmod));
}




void clearDefMods(void)
{
	int i;
	for(i=0;i < DEFMOD_SIZE;++i) if (defmod[i]) free(defmod[i]);
	initDefMods();
}




void addDefMod(int c, char *str)
{
	if (defmod[c]) free(defmod[c]);
	defmod[c] = strdup(str);
}




void dumpDefMods(void)
{
	int i;
	int cnt;
	for(i=cnt=0;i < DEFMOD_SIZE;++i)
	{
		if (defmod[i])
		{
			if (i > 255)
				printf("F%d  = \"%s\"\n",i - 255,defmod[i]);
			else
				printf("'%c' = \"%s\"\n",(char)i,defmod[i]);
			++cnt;
		}
	}
	if (!cnt) puts("There are no keyboard mods defined.");
}

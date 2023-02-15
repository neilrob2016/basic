#include "globals.h"

static char *getUserDir(char *name, char *matchpath);

/*** Goes through a particular directory and looks for an object type that
     matches type and pattern. If type is zero then match any type. ***/
int matchPath(int type, char *pat, char *matchpath, bool toplevel)
{
	DIR *dir;
	struct dirent *de;
	struct stat fs;
	char path[PATH_MAX+1];
	char *ptr;
	char *home_dir;
	int stat_type;
	int err;

	/* Set up initial dir to read */
	if (toplevel)
	{
		matchpath[0] = 0;

		if (*pat == '/')
		{
			strcpy(matchpath,"/");
			++pat;
		}
		else if (*pat == '~')
		{
			++pat;
			home_dir = getpwuid(uid_var->value[0].dval)->pw_dir;
			if (!*pat) strcpy(matchpath,home_dir);
			else if (*pat == '/')
			{
				++pat;
				strcpy(matchpath,home_dir);
			}
			else if (!(pat = getUserDir(pat,matchpath)))
				return ERR_INVALID_PATH;

			strcat(matchpath,"/");
		}
		else if (!strncmp(pat,"../",3))
		{
			strcpy(matchpath,"../");
			pat += 3;
		}
		else
		{
			strcpy(matchpath,"./");
			if (pat[0] == '.' && pat[1] == '/') pat += 2;
		}

		/* If no pattern left then we've got path already */
		if (!*pat) return OK;
	}

	if (!(dir = opendir(matchpath))) return ERR_CANT_OPEN_DIR;

	/* Get section of pattern to match */
	if ((ptr = strchr(pat,'/'))) *ptr = 0;
	err = OK;

	while((de = readdir(dir)))
	{
		/* Only match on . and .. if specifically requested */
		if ((!strcmp(de->d_name,".") && strcmp(pat,".")) ||
		    (!strcmp(de->d_name,"..") && strcmp(pat,".."))) continue;

		/* Match directory entry to pattern section */
		if (!wildMatch(de->d_name,pat,TRUE)) continue;

		/* Name matches the pattern. Stat it and make sure its
		   the correct type */
		if (!copyStr(path,matchpath,PATH_MAX))
		{
			err = ERR_PATH_TOO_LONG;
			goto DONE;
		}
		if (path[strlen(path)-1] != '/') appendPath(path,"/");
		if (!appendPath(path,de->d_name))
		{
			err = ERR_PATH_TOO_LONG;
			goto DONE;
		}

		if (stat(path,&fs) == -1)
		{
			err = ERR_READ;
			goto DONE;
		}
		stat_type = fs.st_mode & S_IFMT;

		/* If more pattern then recurse. First check the matchpath
		   so far is a directory */
		if (ptr && *(ptr+1))
		{
			if (stat_type == S_IFDIR)
			{
				if (!copyStr(matchpath,path,PATH_MAX))
				{
					err = ERR_PATH_TOO_LONG;
					goto DONE;
				}

				if (matchPath(type,ptr+1,matchpath,FALSE) != OK)
				{
					/* Get rid of the bit added by the 
					   recursive call and go around again */
					*strrchr(matchpath,'/') = 0;
					continue;
				}

				/* If OK or another error return */
				goto DONE;
			}
			continue;
		}

		/* At end of pattern. Check final file for type. Treat links
		   as files. */
		if (type == S_IFANY || stat_type == type || 
		    (type == S_IFREG && stat_type == S_IFLNK))
		{
			if (!copyStr(matchpath,path,PATH_MAX))
				err = ERR_PATH_TOO_LONG;
			goto DONE;
		}
	}
	err = ERR_INVALID_PATH;

	DONE:
	closedir(dir);
	if (ptr) *ptr = '/';
	return err;
}




bool hasWildCards(char *path)
{       
	char *s;
	for(s=path;*s && *s != '~' && *s != '*' && *s != '?';++s);
	return (*s ? TRUE : FALSE);
}




int appendPath(char *to, char *add)
{
	int len = PATH_MAX - strlen(to);
	if (len <= 0) return FALSE;
	strncat(to,add,len);
	return TRUE;
}




char *getUserDir(char *name, char *matchpath)
{
	struct passwd *pwd;
	char *ptr;

	if ((ptr = strchr(name,'/')))
		*ptr = 0;
	else
		ptr = name + strlen(name) - 1;
	if (!(pwd = getpwnam(name))) return NULL;
	strcpy(matchpath,pwd->pw_dir);
	return ptr + 1;
}

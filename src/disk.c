#include "globals.h"

/*** Never go longer than MAX PATH but make sure string is terminated ***/
void mystrcpy(char *to, char *from)
{
	strncpy(to,from,PATH_MAX);
	to[PATH_MAX] = 0;
}




/*** Ditto above ***/
bool mystrcat(char *to, char *add)
{
	int len = PATH_MAX - strlen(to);
	if (len <= 0) return FALSE;
	strncat(to,add,len);
	return TRUE;
}




/*** Goes through a particular directory and looks for an object type that
     matches type and pattern. If type is zero then match any type. ***/
int getFirstFileMatch(int type, char *pat, char *matchpath, int depth)
{
	DIR *dir;
	struct dirent *de;
	struct stat fs;
	char path[PATH_MAX+1];
	char *ptr;
	char *home_dir;
	int err;
	int stat_type;

	/* Set up initial dir to read */
	if (!depth)
	{
		if (pat[0] == '/')
		{
			strcpy(matchpath,"/");
			++pat;
		}
		else if (pat[0] == '~')
		{
			++pat;
			if (pat[0] == '/') ++pat;
			home_dir = getpwuid(uid_var->value[0].dval)->pw_dir;
			if (home_dir)
			{
				strcpy(matchpath,home_dir);
				strcat(matchpath,"/");
			}
			else strcpy(matchpath,"./");
		}
		else if (!strncmp(pat,"../",3))
		{
			strcpy(matchpath,"../");
			pat += 3;
		}
		else
		{
			strcpy(matchpath,"./");
			if (pat[0] == '.') ++pat;
			if (pat[0] == '/') ++pat;
		}

		/* If no pattern left then we've got path already */
		if (!*pat) return OK;
	}

	if (!(dir = opendir(matchpath))) return ERR_CANT_OPEN_DIR;

	/* Get section of pattern to match */
	if ((ptr = strchr(pat,'/'))) *ptr = 0;

	while((de = readdir(dir)))
	{
		/* Only match on . and .. if specifically requested */
		if ((!strcmp(de->d_name,".") && strcmp(pat,".")) ||
		    (!strcmp(de->d_name,"..") && strcmp(pat,".."))) continue;

		/* Match directory entry to pattern section */
		if (!wildMatch(de->d_name,pat,TRUE)) continue;

		/* Name matches the pattern. Stat it and make sure its
		   the correct type */
		mystrcpy(path,matchpath);
		if (path[strlen(path)-1] != '/') mystrcat(path,"/");
		if (!mystrcat(path,de->d_name))
		{
			err = ERR_FILENAME_TOO_LONG;
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
				mystrcpy(matchpath,path);
				err = getFirstFileMatch(type,ptr+1,matchpath,depth+1);

				if (err == ERR_NO_SUCH_FILE) 
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
		if (!type || stat_type == type || 
		    (type == S_IFREG && stat_type == S_IFLNK))
		{
			mystrcpy(matchpath,path);
			err = OK;
			goto DONE;
		}
	}
	err = ERR_NO_SUCH_FILE;

	DONE:
	closedir(dir);
	if (ptr) *ptr = '/';
	return err;
}

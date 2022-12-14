#include "globals.h"

void initProcessList()
{
	process_list = NULL;
	processes_cnt = 0;
}




void addProcessToList(pid_t pid)
{
	process_list = (pid_t *)realloc(
		process_list,++processes_cnt * sizeof(pid_t));
	assert(process_list);
	process_list[processes_cnt-1] = pid;
	createProcessArray();
}




/*** It doesn't actually remove it, just sets it to zero ***/
void removeProcessFromList(pid_t pid)
{
	int i;
	if (pid > 0)
	{
		for(i=0;i < processes_cnt;++i)
		{
			if (process_list[i] == pid)
			{
				process_list[i] = 0;
				createProcessArray();
				break;
			}
		}
	}
}




/*** Kill and reap any child processes still running ***/
void killChildProcesses()
{
	pid_t pid;
	int i;

	for(i=0;i < processes_cnt;++i)
	{
		if ((pid = process_list[i]))
		{
			kill(pid,SIGKILL);
			waitpid(pid,NULL,0);
		}
	}
	FREE(process_list);
	initProcessList();
	createProcessArray();
}




/*** This is a bit of a hack as I didn't design arrays to be dynamic, but fork()
     won't be called often so it doesn't matter much ***/
void createProcessArray()
{
	int cnt;
	int i;

	/* Easier to delete and recreate than try to remove elements */
	if (processes_var) deleteVariable(processes_var,NULL);

	/* Get child processes */
	for(i=cnt=0;i < processes_cnt;++i) if (process_list[i]) ++cnt;

	processes_var = createVariable("$processes",VAR_STD,1,&cnt);
	for(i=cnt=0;i < processes_cnt;++i)
	{
		if (process_list[i])
		{
			setValue(
				&processes_var->value[cnt++],
				VAL_NUM,NULL,process_list[i]);
		}
	}
}

#include<stdio.h>
#include<unistd.h>
#include<fcntl.h>
#include<stdlib.h>
#include<string.h>

#define MAX_ARGS 20
#define MAX_PATHS 20
#define MAX_LINE 80
#define MAX_NAME 15
#define debug 1

typedef struct command_t
{
    char name[MAX_NAME];
    int argc;
    char *argv[MAX_ARGS];
} command_t;

command_t cmd;
/*
command_t * readCommand(command_t * cmd)
{
char ch;
char line[MAX_LINE];
int i;
fgets(line,sizeof(line),stdin);
//printf("%d\n",strlen(line));
int len=(int)strlen(line);
line[len-1]='\0';
char *pch;
pch=strtok(line," ");
i=0;
while(pch!=NULL)
{
cmd->argv[i++]=pch;
pch=strtok(NULL," ");
}
cmd->argc=i;
cmd->argv[i++]=NULL;
if(debug) {
		printf("Stub: parseCommand(char, struct);\n");
		printf("Array size: %i\n", sizeof(*cmd->argv));
		int j;
		for(j=0; j<i; j++) {
			printf("command->argv[%i] = %s\n", j, cmd->argv[j]);
		}
		printf("\ncommand->argc = %i\n", cmd->argc);

		if(cmd->argv[0] != NULL) {
			//printf("*command->argv[%i] = %c\n", j, *command->argv[0]);
			char **p;
			for(p = &cmd->argv[1]; *p != NULL; p++) {
				printf("%s\n", *p);
			}
		}
	}
return cmd;
}
*/
int getPathofCommand(command_t *cmd)
{
    char *path;
    path=getenv("PATH");
    char *pathv;
    char strpath[MAX_PATHS];
    pathv=strtok(path,":");
    while(pathv!=NULL)
    {
        sprintf(strpath,"%s/%s",pathv,cmd->argv[0]);
        //printf("%s\n",strpath);
        if(access(strpath,X_OK)==0)
        {
            strcpy(cmd->name,strpath);
            return 1;
        }
        pathv=strtok(NULL,":");
    }
    strcpy(cmd->name,cmd->argv[0]);
    return 0;
}
int processCommand()
{
    getPathofCommand(&cmd);
    execve(cmd.name,cmd.argv,0);
    return 0;
}

int main()
{
    int i,j;
    char ch;
    pid_t child_pid;
    char line[MAX_LINE];
    while(1)
    {
        printf("$");
        fgets(line,sizeof(line),stdin);
        int len=(int)strlen(line);
        line[len-1]='\0';
        char *pch;
        pch=strtok(line," ");
        i=0;
        while(pch!=NULL)
        {
            cmd.argv[i++]=pch;
            pch=strtok(NULL," ");
        }
        cmd.argc=i;
	printf("i = %d\n",i);
        cmd.argv[i++]=NULL;
        if(!strcmp(cmd.argv[0],"exit")||!strcmp(cmd.argv[0],"quit"))
        {
            printf("Exit\n");
            exit(0);
        }
        /*//cmd1=&cmd;
        if(debug) {
        		printf("Stub: parseCommand(char, struct);\n");
        		printf("Array size: %i\n", sizeof(*cmd.argv));
        		for(j=0; j<i; j++) {
        			printf("command->argv[%i] = %s\n", j, cmd.argv[j]);
        		}
        		printf("\ncommand->argc = %i\n", cmd.argc);

        		if(cmd.argv[0] != NULL) {
        			//printf("*command->argv[%i] = %c\n", j, *command->argv[0]);
        			char **p;
        			for(p = &cmd.argv[1]; *p != NULL; p++) {
        				printf("%s\n", *p);
        			}
        		}
        	}*/
	
        child_pid=fork();
        //printf("%d\n",child_pid);
        switch(child_pid)
        {
        case -1:
            perror("Fork Error\n");
            break;
        case 0:
            //processCommand();
            exit(0);
            break;
        default:
            sleep(0.5);
            waitpid(child_pid,NULL,0);
        }
    }
    return 0;
}

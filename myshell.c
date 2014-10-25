#include<stdio.h>
#include<unistd.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>
#include <readline/readline.h>
#include <readline/history.h>
#include<pwd.h>
#include<signal.h>
#include <termios.h>

#define MAX_ARGS 20
#define MAX_PATHS 20
#define MAX_LINE 80
#define MAX_NAME 15
#define TRUE 1
#define FALSE 0

#define FOREGROUND 'F'
#define BACKGROUND 'B'

#define STDIN 1
#define STDOUT 2

#define debug 0

typedef struct job
{
    int id;
    char *name;
    pid_t pid;
    pid_t ppid;
    pid_t pgid;
    int status;
    char *infile;
    char *outfile;
    struct job *next;
} job;

static job* JobList = NULL;

char name[MAX_NAME];
int cmdcount;
char **cmd;
char welcomestring[150] ;
char *line , hostname[50] , cwd[50] ;
struct passwd *passwd;
int status;

static pid_t SHELL_PID;
static pid_t SHELL_PGID;
static int SHELL_TERMINAL, IS_INTERACTIVE_SHELL;
static int CHILD_PGID;
static int numActiveJobs = 0;
int executionMode=FOREGROUND;

char * stripwhite (string)
char *string;
{
    register char *s, *t;

    for (s = string; whitespace (*s); s++)
        ;

    if (*s == 0)
        return (s);

    t = s + strlen (s) - 1;
    while (t > s && whitespace (*t))
        t--;
    *++t = '\0';

    return s;
}


static char *line_read = (char *)NULL;

char * Gets (char string[150])
{
    if (line_read)
    {
        free (line_read);
        line_read = (char *)NULL;
    }
    line_read = readline (string);
    //strcat(ch,line_read);
    return (line_read);
}
/*
int getPathofCommand()
{
    char *path;
    path=getenv("PATH");
    char *pathv;
    char strpath[MAX_PATHS];
    pathv=strtok(path,":");
    while(pathv!=NULL)
    {
        sprintf(strpath,"%s/%s",pathv,cmd[0]);
        //printf("%s\n",strpath);
        if(access(strpath,X_OK)==0)
        {
            strcpy(name,strpath);
            return 1;
        }
        pathv=strtok(NULL,":");
    }
    strcpy(name,cmd[0]);
    return 0;
}*/

/*int processCommand()
{
    execve(name,cmd,0);
    return 0;
}
*/
void clearScreen()
{
    printf("\033[2J\033[1;1H");
}

void changeDir(char * argv)
{
    if (argv == NULL)
    {
        chdir(getenv("HOME"));
    }
    else
    {
        if (chdir(argv) == -1)
        {
            printf(" %s: no such directory\n", argv);
        }
    }
    char cwd[50];
    getcwd(cwd,50);
    //printf("CWD is %s\n",cwd);
    if(!strncmp(cwd, "/home",5))
    {
        char * tempcwd=strchr(strchr(cwd+1,'/')+1,'/');
        sprintf(cwd,"~%s",tempcwd==NULL?"":tempcwd);
    }
    if(cwd==NULL)
    {
        strcpy(cwd,"");
    }
    sprintf(welcomestring,"%s@%s:%s$ ",passwd->pw_name,hostname,cwd);
    //printf("CWD is %s\n",cwd);
}

void makeDir()
{
    if (strncmp(cmd[0], "mkdir", 5) == 0)
    {
        if(cmd[1] != NULL )
        {
            status = mkdir(cmd[1], 0775);
            perror("mkdir ");
        }
    }
}

void rmDir()
{
    if (strncmp(cmd[0], "rmdir", 5) == 0)
    {
        if(cmd[1] != NULL )
        {
            status = rmdir(cmd[1]);
            perror("rmdir ");
        }
    }
}

void echoDollar()
{
    if (strncmp(cmd[0], "echo", 4) == 0)
    {
        if(cmd[1] != NULL )
        {
            char *echo_str = (char *)malloc(sizeof(strlen(cmd[1])));
            strcpy(echo_str, cmd[1]);
            //printf("%s\n",echo_str);
            if (strcmp(echo_str,"$$") == 0)
            {
                printf("PID: %d\n", getpid());
                free(echo_str);
                return;
            }

            if (strcmp(echo_str,"$?") == 0)
            {
                printf("%d\n", status);
                free(echo_str);
                return;
            }
            if(!strncmp(echo_str,"$",1))
            {
                char *path;
                //printf("yes\n");
                path=getenv(echo_str+1);
                printf("%s\n",path);
                return;
            }
        }
    }
}
void showhistory()
{
    register HIST_ENTRY **the_list;
    register int i;

    the_list = history_list ();
    if (the_list)
        for (i = 0; the_list[i]; i++)
            printf ("%d: %s\n", i + history_base, the_list[i]->line);
}

void executehistory(int offset)
{
    register HIST_ENTRY *hist;
    hist= history_get (offset-history_base+1);
    line=hist->line;
    char *pch;
    pch=strtok(line," ");
    int i=0;
    while(pch!=NULL)
    {
        cmd[i++]=pch;
        pch=strtok(NULL," ");
    }
    cmdcount=i;
    cmd[i++]=NULL;
}


void printStatus(job * job,int * termstatus)
{
    //printf("Status= %d\n",*termstatus);
    if (WIFEXITED(*termstatus))
    {
        if (job->status == BACKGROUND)
        {
            printf("\n[%d]+  Done\t   %s\n", job->id, job->name);
        }
    }
    else if (WIFSIGNALED(*termstatus))
    {
        printf("\n[%d]+  Killed\t   %s\n", job->id, job->name);
    }
    else if (WIFSTOPPED(*termstatus))
    {
        if(job->status=FOREGROUND)
        {
            tcsetpgrp(SHELL_TERMINAL, job->pgid);
            printf("\n[%d]+   stopped\t   %s\n", numActiveJobs, job->name);
        }
    }
}

int delJob(int pid,int * termstatus)
{
    numActiveJobs--;
    if(JobList->pid==pid && JobList->next==NULL)
    {
        printStatus(JobList,termstatus);
        JobList=NULL;
        return 1;
    }

    job * prevJob, * tempJob;

    if(JobList->pid==pid)
    {
        tempJob=JobList;
        JobList=JobList->next;
        printStatus(tempJob,termstatus);
        free(tempJob);
        return 1;
    }
    tempJob=JobList->next;
    prevJob=JobList;
    while (tempJob != NULL)
    {
        //printf("yes\n");
        if(tempJob->pid==pid)
        {
            prevJob->next=tempJob->next;
            printStatus(tempJob,termstatus);
            free(tempJob);
            return 1;
        }
        tempJob = tempJob->next;
    }
    return 0;
}

void catcher(int signum)
{
    int ppid=-1;
    static int termstatus;
    if((ppid=waitpid(-1,&termstatus,WNOHANG))>0)
        delJob(ppid,&termstatus);
    tcsetpgrp(SHELL_TERMINAL,SHELL_PGID);
//printf("int catcher PID %d Exit %x \n",ppid,termstatus);
}

void init()
{
    SHELL_PID = getpid();
    SHELL_TERMINAL = STDIN_FILENO;
    IS_INTERACTIVE_SHELL = isatty(SHELL_TERMINAL);
    if (IS_INTERACTIVE_SHELL)
    {
        while (tcgetpgrp(SHELL_TERMINAL) != (SHELL_PGID = getpgrp()))
            kill(SHELL_PID, SIGTTIN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGINT, SIG_IGN);
        signal(SIGCHLD,&catcher);
        setpgid(SHELL_PID, SHELL_PID);
        SHELL_PGID = getpgrp();
        if (SHELL_PID != SHELL_PGID)
        {
            printf("Error, the shell is not process group leader");
            exit(EXIT_FAILURE);
        }
        tcsetpgrp(SHELL_TERMINAL, SHELL_PGID);
    }
    else
    {
        printf("Could not make SHELL interactive. Exiting..\n");
        exit(EXIT_FAILURE);
    }
}

job* newJob(pid_t pid, pid_t ppid, pid_t pgid, char* name, char* infile,char* outfile ,int status)
{
    job *tempJob = malloc(sizeof(job));
    tempJob->name = (char*) malloc(sizeof(name));
    tempJob->name = strcpy(tempJob->name, name);
    tempJob->pid = pid;
    tempJob->ppid=ppid;
    tempJob->pgid = pgid;
    tempJob->status = status;
    tempJob->infile = (char*) malloc(sizeof(infile));
    tempJob->infile = strcpy(tempJob->infile, infile);
    tempJob->outfile = (char*) malloc(sizeof(outfile));
    tempJob->outfile = strcpy(tempJob->outfile, outfile);
    tempJob->next = NULL;
    return tempJob;
}

job* insertJob(pid_t pid, pid_t ppid, pid_t pgid, char* name, char* infile,char* outfile ,int status)
{
    // usleep(10000);

    job *tempJob;
    tempJob=newJob(pid,ppid,pgid,name,infile,outfile,status);
    if (JobList == NULL)
    {
        numActiveJobs++;
        if(status==BACKGROUND)
            printf("[%d] \t %d \n",numActiveJobs,tempJob->pid);
        tempJob->id = numActiveJobs;
        return tempJob;
    }
    else
    {
        job *tempNode = JobList;
        while (tempNode->next != NULL)
        {
            tempNode = tempNode->next;
        }
        tempJob->id = tempNode->id + 1;
        if(status==BACKGROUND)
            printf("[%d] \t %d \n",tempNode->id,tempNode->pid);
        tempNode->next = tempJob;
        numActiveJobs++;
        return JobList;
    }
}


void printJobs()
{
    job* Job = JobList;
    if (Job == NULL)
        return;
    printf("\nActive jobs:\n");
    printf("---------------------------------------------------------------------------\n");
    printf("| %7s  | %30s | %5s | %5s | %5s | %10s | %10s | %6s |\n", "JOB NO.", "NAME", "PPID" ,"PID", "PGID",
           "INFILE", "OUTFILE" , "STATUS");
    printf("---------------------------------------------------------------------------\n");
    while (Job != NULL)
    {
        //printf("yes\n");
        printf("|  %7d | %30s | %5d | %5d | %5d | %10s | %10s | %6c |\n", Job->id, Job->name, Job->ppid ,Job->pid,Job->pgid, Job->infile, Job->outfile, Job->status);
        Job = Job->next;
    }
    printf("---------------------------------------------------------------------------\n");
}

void freeJobs()
{
    job * tempjob;
    job* job = JobList;
    while (job != NULL)
    {
        tempjob=job;
        job = job->next;
        free(tempjob);
    }
}

int isBuiltIn()
{
    if(strcmp("cd", cmd[0]) == 0)
    {
        changeDir(cmd[1]);
        return 1;
    }
    if(strcmp("mkdir", cmd[0]) == 0)
    {
        makeDir(cmd[1]);
        return 1;
    }
    if(strcmp("rmdir", cmd[0]) == 0)
    {
        rmDir(cmd[1]);
        return 1;
    }
    if(strcmp("clear", cmd[0]) == 0)
    {
        clearScreen();
        return 1;
    }
    if(strcmp("jobs", cmd[0]) == 0)
    {
        printJobs();
        return 1;
    }
    if(!strcmp("echo", cmd[0])&&!strncmp("$",cmd[1],1))
    {
        echoDollar();
        return 1;
    }
    if(!strcmp("!!",cmd[0])||!strcmp("history", cmd[0]))
    {
        showhistory();
        return 1;
    }
    if(strncmp("!", cmd[0],1) == 0)
    {
        executehistory(atoi(cmd[0]+1));
        return 0;
    }
    return 0;
}


int main()
{
    int i,j,k;
    char ch;
    pid_t child_pid;
    init();
    cmd = (char **) malloc(32 * sizeof(char *));
    gethostname(hostname,50);
    passwd = getpwuid(getuid());
    printf("--------------------------------------Welcome to MYSHELL--------------------------------------------------- \n");
    getcwd(cwd,50);
    if(!strncmp(cwd, "/home",5))
    {
        char * tempcwd=strchr(strchr(cwd+1,'/')+1,'/');
        sprintf(cwd,"~%s$ ",tempcwd==NULL?"":tempcwd);
    }
    if(cwd==NULL)
    {
        strcpy(cwd,"");
    }
    sprintf(welcomestring,"%s@%s:%s$ ",passwd->pw_name,hostname,cwd);
    while(1)
    {
        int isbg=0;
        int fd1[2]  = { -1, -1 };
        int fd2[2] = { -1, -1 };
        //printf("$");
        //fgets(line,sizeof(line),stdin);
        //printf("%s",welcomestring);
        line = Gets (welcomestring);
        int len=(int)strlen(line);
        //if (len<1)
        //continue;
        line = stripwhite (line);
        if(!*line)
            continue;
        else
        {
            add_history (line);
        }
        char *pch;
        pch=strtok(line," ");
        i=0;
        while(pch!=NULL)
        {
            cmd[i++]=pch;
            pch=strtok(NULL," ");
        }
        cmdcount=i;
        cmd[i++]=NULL;
        //printf("i=%d\n",i);
        if(!strcmp(cmd[0],"exit")||!strcmp( cmd[0],"quit"))
        {
            printf("exiting..........\n");
            freeJobs();
            exit(0);
        }
        if(isBuiltIn())
        {
            continue;
        }
        //printf
        int pipeloc[32],infileloc[32],outfileloc[32];
        int numpipe=0,numout=0,numin=0;
        pipeloc[numpipe]=0;
        int haveoutfile[32],haveinfile[32];
        int processno=0;
        haveinfile[processno]=haveoutfile[processno]=0;
        for(k=0; k<cmdcount; k++)
        {
            if(!strcmp( cmd[k],"|"))
            {
                pipeloc[++numpipe]=k+1;
                cmd[k]=NULL;
                processno++;
                haveinfile[processno]=haveoutfile[processno]=0;
            }
            else if(!strcmp( cmd[k],"<"))
            {
                infileloc[numin++]=k+1;
                cmd[k]=NULL;
                haveinfile[processno]=k+1;
            }
            else if(!strcmp( cmd[k],">"))
            {
                outfileloc[numout++]=k+1;
                cmd[k]=NULL;
                haveoutfile[processno]=k+1;
            }
            else if(!strcmp( cmd[k],"&"))
            {
                if(cmd[k+1]!=NULL)
                {
                    printf("bash : Wrong Syntax '&' should not be in Middle\n");
                    isbg=1;//Wrong Syntax
                    break;
                }
                isbg=2;//Yes Background process
                cmd[k]=NULL;
            }
        }
        //printf("igbg= %d\n",isbg);
        if(isbg==1) continue;
        pipeloc[numpipe+1]=cmdcount;
        i=cmdcount+1;
        //cmd1=&cmd;
        if(debug)
        {
            printf("Array size: %i\n", sizeof(* cmd));
            for(j=0; j<i; j++)
            {
                printf("command->cmd[%i] = %s\n", j,  cmd[j]);
            }
            printf("\ncommand->cmdcount = %i\n",  cmdcount);
            printf("PIPE LOCATION\n");
            for(j=0; j<=numpipe; j++)
                printf("%d\n",pipeloc[j]);
            printf("INPUT LOC\n");
            for(j=0; j<numin; j++)
                printf("%d\n",infileloc[j]);
            printf("OUTPUT LOC\n");
            for(j=0; j<numout; j++)
                printf("%d\n",outfileloc[j]);
            for(j=0; j<=processno; j++)
            {
                printf("INFILE = %d OUTFILE= %d\n" , haveinfile[j],haveoutfile[j]);
            }
        }
        int l=0,m=0;
        for (j = 0; j <= numpipe; j++)
        {

            if (j != numpipe)
            {
                if (numpipe > 0)
                {
                    if (pipe(fd2) != 0)
                        perror("pipe error");
                }
            }
            if ((child_pid = fork()) < 0)
                perror("fork failed");
            else if (child_pid == 0)
            {
                //sleep(0.5);
                signal(SIGINT, SIG_DFL);
                signal(SIGQUIT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);
                signal(SIGTTIN, SIG_DFL);

                int process_start;
                int flag1=0,flag2=0;
                if(haveinfile[j])
                {
                    int fdin=open(cmd[haveinfile[j]],O_RDONLY);
                    dup2(fdin, 0);
                    close(fdin);
                }
                if(haveoutfile[j])
                {
                    int fdout=creat(cmd[haveoutfile[j]],0644);
                    dup2(fdout,1);
                    close(fdout);
                }
                if (j != numpipe && !haveoutfile[j])
                {
                    dup2(fd2[1], 1);
                    close(fd2[0]);
                    close(fd2[1]);
                }

                if (j != 0 && !haveinfile[j])
                {
                    dup2(fd1[0], 0);
                    close(fd1[0]);
                    close(fd1[1]);
                }
                //printf("11\n");
                process_start = pipeloc[j];
                //sleep(10);
                execvp(cmd[process_start], & cmd[process_start]);
                fprintf(stderr, "exec failed\n");
                exit(EXIT_SUCCESS);
            }
            if(isbg==2)
            {
                setpgid(child_pid,child_pid);
                tcsetpgrp(SHELL_TERMINAL,SHELL_PGID);
            }

            if (j != 0)
            {
                assert(fd1[0] != -1 && fd1[1] != -1);
                close(fd1[0]);
                close(fd1[1]);

            }

            if(debug) printf("PID %d launched\n", child_pid);
            fd1[0] = fd2[0];
            fd1[1] = fd2[1];
            char *inFile,*outFile;
            if(haveinfile[j])
            {
                inFile=cmd[haveinfile[j]];
            }
            else
            {
                inFile=malloc(6);
                strcpy(inFile,"STDIN");
            }
            if(haveoutfile[j])
            {
                outFile=cmd[haveoutfile[j]];
            }
            else
            {
                outFile=malloc(7);
                strcpy(outFile,"STDOUT");
            }
            if(isbg==0)
                if(j==0)
                {
                    setpgid(child_pid,child_pid);
                    tcsetpgrp(SHELL_TERMINAL,child_pid);
                    CHILD_PGID=child_pid;
                    //CHILD_SID=getsid();
                }
                else
                {
                    if(debug)
                    {
                        printf("CH-SID %d NEW %d \n",getsid(CHILD_PGID),getsid(child_pid));
                        //perror(strerror(errno));
                        printf("in CHILD_PGID= %d\n",CHILD_PGID);
                    }
                    //printf("setpgid = %d\n",setgid(CHILD_PGID));
                    setpgid(child_pid,CHILD_PGID);

                }
            JobList = insertJob(child_pid,getpid(), getpgid(child_pid), cmd[pipeloc[j]], inFile, outFile ,(isbg==2)?BACKGROUND:FOREGROUND);

            pid_t ppid;
            if(debug) printf("TCGETPGRP= %d\n",tcgetpgrp(SHELL_TERMINAL));
            if(isbg!=2)
                if((ppid=waitpid(child_pid,&status,0))!=-1)
                {
                    //printf("PID %d Exit %x\n",ppid,status);
                    delJob(child_pid,&status);
                    tcsetpgrp(SHELL_TERMINAL,SHELL_PGID);
                }
            //tcsetpgrp(SHELL_TERMINAL,SHELL_PGID);
            if(debug) printJobs();
            //freeJobs();
            //sleep(1000);
        }
    }
    return 0;
}

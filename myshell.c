
/* ****************************************************MYSHELL - a mini shell *****************************************************************/

/*                                          		 Created by:	

************************************************************************************************************************************************
****************************************		    AJAY YADAV  				****************************************
****************************************      ENROLLMENT NUMBER - 14535002 (2014-2016)	         	****************************************
****************************************      M.Tech. Ist Year ( Computer Science and Engineering)	****************************************
****************************************      IIT Roorkee , Roorkee , Uttarakhand , India , 247667	****************************************
************************************************************************************************************************************************
*/

/* Dependency : libreadline6-dev and libreadline6    .. please install these libraries before running this program.
    IN UBUNTU:
			type ..... sudo apt-cache search lreadline          (TO SEARCH LIBRARIES)
			// 	then install libreadline6 and libreadline6-dev	//
			type ...   sudo apt-get install libreadline6 libreadline6-dev
*/

/* HEADER FILEs */

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

/*MACROS DEFINITION */

#define MAX_ARGS 20
#define MAX_PATHS 20
#define MAX_LINE 80
#define MAX_NAME 15
#define TRUE 1
#define FALSE 0

#define FOREGROUND 'F'
#define BACKGROUND 'B'
#define STOP 'S'

#define STDIN 1
#define STDOUT 2


/*JOB STRUCTURE */

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

/*Command Variables */

char name[MAX_NAME];
int cmdcount;
char **cmd;
char welcomestring[150] ;
char *line , hostname[50] , cwd[50] ;
struct passwd *passwd;
int status;

/*Pipes and Redirection Variables*/

int inbuilt_function_flag=0;
int pipeloc[32],infileloc[32],outfileloc[32];
int numpipe=0,numout=0,numin=0;
int haveoutfile[32],haveinfile[32];
int processno=0;
int isbg;

/*JOB CONTROLLING VARIABLES */

static pid_t shell_pid;
static pid_t shell_pgid;
static int shell_terminal, is_shell_interactive;
static int child_pgid;
static int numActiveJobs = 0;
int executionMode=FOREGROUND;

/* PARSING COMPONENTS */

void init();
void parser();
char * stripwhite (char *);
void Pipe_and_Redirection();
void createWelcomeString();
void checkExit();

/* BUILTIN FUNCTIONS HANDLING COMPONENT */

int isBuiltIn();
void clearScreen();
void changeDir(char *);
void makeDir();
void rmDir();
void echoDollar();
void showhistory();
void executehistory(int );

/* JOB CONTROLLING COMPONENT */

int delJob(int ,int * );
void printStatus(job * ,int * );
job* newJob(pid_t , pid_t , pid_t , char* , char* ,char* ,int );
job* insertJob(pid_t , pid_t , pid_t , char* , char* ,char* ,int );
void printJobs();
void freeJobs();

/* SIGNAL HANDLERS */

void sigchld_handler(int );
void sigquit_handler(int );
void sigtstp_handler(int );

/* Function to remove whitespace from both side of a string */

char * stripwhite (char* string)
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

/* Function to get input through GNU Readline Library */

static char *line_read = (char *)NULL;

char * Gets (char string[150])
{
    if (line_read)
    {
        free (line_read);
        line_read = (char *)NULL;
    }
    line_read = readline (string);
    return (line_read);
}

/*BUILTIN FUNCTION - used to clear screen */

void clearScreen()
{
    printf("\033[2J\033[1;1H");
}

/* Function to change Directory */

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
	createWelcomeString();
}

/* Function to Make a Directory */

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

/* Function to Remove a Directory */

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
/* Function for BUILTIN echo i.e for echo $variable or echo $$ */

void echoDollar()
{
    if (strncmp(cmd[0], "echo", 4) == 0)
    {
        if(cmd[1] != NULL )
        {
            char *echo_str = (char *)malloc(sizeof(strlen(cmd[1])));
            strcpy(echo_str, cmd[1]);
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
                path=getenv(echo_str+1);
                printf("%s\n",path);
                return;
            }
        }
    }
}

/*Funtion to Add Command to History */

void Add_history()
{
	if(!strncmp("!",line,1))
	{
		if(!strncmp("!",line+1,1))
		{
			register HIST_ENTRY * hist;
			hist=history_get(history_length);
			add_history(hist->line);
			return;
		}
		register HIST_ENTRY * hist;
		int offset=atoi(line+1);
		hist=history_get(offset-history_base+1);
		add_history(hist->line);
		return;
	}
	add_history(line);
	return;
}

/* Function to show History */

void showhistory()
{
    register HIST_ENTRY **the_list;
    register int i;

    the_list = history_list ();
    if (the_list)
        for (i = 0; the_list[i]; i++)
            printf ("%d: %s\n", i + history_base, the_list[i]->line);
}

/* Function to Execute command of type !n */

void executehistory(int offset)
{
    register HIST_ENTRY *hist;
    hist= history_get (offset-history_base+1);
    line=hist->line;
    parser();
    if(isBuiltIn())
		inbuilt_function_flag=1;
}

/* Function to print a Job Status after Exiting */

void printStatus(job * job,int * termstatus)
{
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
            tcsetpgrp(shell_terminal, job->pgid);
            printf("\n[%d]+   stopped\t   %s\n", numActiveJobs, job->name);
        }
    }
}

/* Function to delete a job from JOB LIST */

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

/* SIGCHLD HANDLER for background Processes */

void sigchld_handler(int signum)
{
    int ppid=-1;
    static int termstatus;
    if((ppid=waitpid(-1,&termstatus,WNOHANG))>0)
        delJob(ppid,&termstatus);
    tcsetpgrp(shell_terminal,shell_pgid);
}

/*Function to init SHELL PROGRAM */

void init()
{
    shell_pid = getpid();
    shell_terminal = STDIN_FILENO;
    is_shell_interactive = isatty(shell_terminal);
    if (is_shell_interactive)
    {
        while (tcgetpgrp(shell_terminal) != (shell_pgid = getpgrp()))
            kill(shell_pid, SIGTTIN);
        signal(SIGQUIT, SIG_IGN);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTSTP, SIG_IGN);
        signal(SIGINT, SIG_IGN);
        signal(SIGCHLD,&sigchld_handler);
        setpgid(shell_pid, shell_pid);
        shell_pgid = getpgrp();
        if (shell_pid != shell_pgid)
        {
            printf("Error, the shell is not process group leader");
            exit(EXIT_FAILURE);
        }
        tcsetpgrp(shell_terminal, shell_pgid);
    }
    else
    {
        printf("Could not make SHELL interactive. Exiting..\n");
        exit(EXIT_FAILURE);
    }
}

/* Function to create a new job with given values */

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

/*Function to insert a new job to JOB list */

job* insertJob(pid_t pid, pid_t ppid, pid_t pgid, char* name, char* infile,char* outfile ,int status)
{

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


/* Function to print JOBS when "jobs" command will be executed */

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
        printf("|  %7d | %30s | %5d | %5d | %5d | %10s | %10s | %6c |\n", Job->id, Job->name, Job->ppid ,Job->pid,Job->pgid, Job->infile, Job->outfile, Job->status);
        Job = Job->next;
    }
    printf("---------------------------------------------------------------------------\n");
}

/* Funtion to free JOB LIST when program will be exiting */

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


/* Function to check and Execute BUILTIN commands */

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
    if(!strcmp("history", cmd[0]))
    {
        showhistory();
        return 1;
    }
    if(strncmp("!", cmd[0],1) == 0)
    {
	if(!strncmp("!",cmd[0]+1,1))
	{
		executehistory(history_length-1);
		return 0;
	}
        executehistory(atoi(cmd[0]+1));
        return 0;
    }
    return 0;
}

/* Function to parse the given command and generate tokens */

void parser(){
	char *pch;
	char *saveptr;
        pch=strtok_r(line," ",&saveptr);
        int i=0;
        while(pch!=NULL)
        {
            cmd[i++]=pch;
            pch=strtok_r(NULL," ",&saveptr);
        }
        cmdcount=i;
        cmd[i++]=NULL;
}

/* Function to Mark Pipes and redirection in Command */

void Pipe_and_Redirection()
{
  int k;
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
}

/* Function for checking EXIT/QUIT command */
void checkExit()
{
        if(!strcmp(cmd[0],"exit")||!strcmp( cmd[0],"quit"))
        {
            printf("exiting..........\n");
            freeJobs();
            exit(0);
        }
}

/*Function to create Welcome String i.e prompt */

void createWelcomeString()
{
    gethostname(hostname,50);
    passwd = getpwuid(getuid());
    getcwd(cwd,50);
    if(!strncmp(cwd, strcat(strdup("/home/"),passwd->pw_name) ,6+strlen(passwd->pw_name)))
    {
        char * tempcwd=strchr(strchr(cwd+1,'/')+1,'/');
        sprintf(cwd,"~%s$ ",tempcwd==NULL?"":tempcwd);
    }
    if(cwd==NULL)
    {
        strcpy(cwd,"");
    }
    sprintf(welcomestring,"%s@%s:%s$ ",passwd->pw_name,hostname,cwd);
}

/* MAIN FUNCTION */

int main()
{
    int i,j,k;
    char ch;
    pid_t child_pid;
    init();
    cmd = (char **) malloc(32 * sizeof(char *));

    printf("--------------------------------Welcome to MYSHELL-a mini shell---------------------------------------------\n");

    printf("--------------------------------------AJAY YADAV , IITR-----------------------------------------------------\n");
    
    createWelcomeString();
    
    while(1)
    {
        isbg=0;										/*checking for backgorund i.e & */
	inbuilt_function_flag=0;
        int fd1[2]  = { -1, -1 }; 							/*for memorizing previos pipes */
        int fd2[2] = { -1, -1 };							/*for creating pipe for every process */
        line = Gets (welcomestring);							/* reading the command */
        int len=(int)strlen(line);							
        line = stripwhite (line);							/* removing Whitespaces both side */
        if(!*line)
            continue;									/*if NULL string then continue again */
        else
        {
            Add_history ();								/*ADD to history */
        }

        parser();									/*parsing command and tokenizing */
	
	checkExit();									/*checking for Exit command */	

        if(isBuiltIn())						/*if BUILTIN command then continue and process them in isBuiltIn Funtion */
        {
            continue;
        }
	numpipe=0,numout=0,numin=0;				/*initializing Number of pipe , output redirection and input redirection to 0*/
	pipeloc[numpipe]=0;
	processno=0;
	haveinfile[processno]=haveoutfile[processno]=0;		/*to account for all the pipe command that if they have inFILE or outFile */

	Pipe_and_Redirection();					/* to Store Pipelocation and Input and Output Redirection Places */

        if(isbg==1) continue;					/*if Wrong syntax i.e. & placed between commands then continue */
	if(inbuilt_function_flag) continue;			/*if inbuilt function then continue */
        pipeloc[numpipe+1]=cmdcount;
        i=cmdcount+1;
        int l=0,m=0;
        for (j = 0; j <= numpipe; j++)				/*create number_of_pipe +1 process */
        {

            if (j != numpipe)					/*if not last process */
            {
                if (numpipe > 0)
                {
                    if (pipe(fd2) != 0)
                        perror("pipe error");
                }
            }
            if ((child_pid = fork()) < 0)			/*forking to create child */
                perror("fork failed");
            else if (child_pid == 0)				/*if it is child */
            {
                signal(SIGINT, SIG_DFL);                        /*Handling Signals */
                signal(SIGQUIT, SIG_DFL);
                signal(SIGTSTP, SIG_DFL);
                signal(SIGTTIN, SIG_DFL);
		signal(SIGCHLD,&sigchld_handler);
                int process_start;
                int flag1=0,flag2=0;
                if(haveinfile[j])  					/*if HAVE INPUT FILE Redirection then redirect from file */
                {
                    int fdin=open(cmd[haveinfile[j]],O_RDONLY);
                    dup2(fdin, 0);
                    close(fdin);
                }
                if(haveoutfile[j])					/*if HAVE OUTPUT FILE Redirection then redirect to file */
                {
                    int fdout=creat(cmd[haveoutfile[j]],0644);
                    dup2(fdout,1);
                    close(fdout);
                }
                if (j != numpipe && !haveoutfile[j])		/*if not last process and have no output file then redirect to next pipe */
                {
                    dup2(fd2[1], 1);
                    close(fd2[0]);
                    close(fd2[1]);
                }

                if (j != 0 && !haveinfile[j])			/*if not first process and have no input file then redirect from previous pipe */
                {
                    dup2(fd1[0], 0);
                    close(fd1[0]);
                    close(fd1[1]);
                }
                    process_start = pipeloc[j];						/*next pipe command location */
                execvp(cmd[process_start], & cmd[process_start]); 			    /*execute current command */
                fprintf(stderr, "exec failed\n");
                exit(EXIT_SUCCESS);                               			  /*if Execution failed */
            }

	/* PARENT PROCESS */

            if(isbg==2)							/*if background process ,create its own group */
            {
                setpgid(child_pid,child_pid);
                tcsetpgrp(shell_terminal,shell_pgid);
            }

            if (j != 0)							/*if not first process close previous pipe to decrease refrence count */
            {
                assert(fd1[0] != -1 && fd1[1] != -1);
                close(fd1[0]);
                close(fd1[1]);

            }

            fd1[0] = fd2[0];						/*copy current pipe to memrize for next command */
            fd1[1] = fd2[1];
            char *inFile,*outFile;
            if(haveinfile[j])						/*if Have input file */
            {
                inFile=cmd[haveinfile[j]];
            }
            else							/*else input file is STDIN */
            {
                inFile=malloc(6);
                strcpy(inFile,"STDIN");
            }
            if(haveoutfile[j])						/*if Have Output file */
            {
                outFile=cmd[haveoutfile[j]];
            }
            else							/*else  output file is STDOUT */
            {
                outFile=malloc(7);
                strcpy(outFile,"STDOUT");
            }
            if(isbg==0)							/*if foreground process */		
                if(j==0)						/*if first program */
                {
                    setpgid(child_pid,child_pid);
                    tcsetpgrp(shell_terminal,child_pid);
                    child_pgid=child_pid;
                 }
                else							/*else than first program */
                {
                                      setpgid(child_pid,child_pgid);

                }
		/*insert the job in JOB LIST */
            JobList = insertJob(child_pid,getpid(), getpgid(child_pid), cmd[pipeloc[j]], inFile, outFile ,(isbg==2)?BACKGROUND:FOREGROUND);

            pid_t ppid;
		/* if foreground process wait for child*/
            if(isbg!=2)
                if((ppid=waitpid(child_pid,&status,0))!=-1)			 /* Wait for child */
                {
                    delJob(child_pid,&status);					 /*delete from JOB LIST */
                    tcsetpgrp(shell_terminal,shell_pgid);			 /*set terminal to shell program*/
                }			
        }
    }
    return 0;
}

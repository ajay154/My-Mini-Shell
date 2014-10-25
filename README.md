MYSHELL-a mini shell


Prerequisites:
Dependency: libreadline6-dev and libreadline6. please install these libraries before running this program.
in UBUNTU: to install libreadline6 and libreadline6-dev type: 
sudo apt-get install libreadline6 libreadline6-dev
To compile and run program :
Open terminal and type: 
gcc –o mshell ajay_myshell.c –lreadline –lhistory
Now run the program by typing:
./mshell
Note: Execute all the command by providing space between each argument. 
List of Commands:
Built-in commands:
1.	clear
2.	cd
3.	mkdir
4.	rmdir
5.	echo $Variable
6.	history
7.	!n
8.	!!
External Commands:
All commands that are in path like ls ,ps ,grep , sort etc.

Supported Features:
1.	Multipipe commands:
It supports multipipe command like ls –l | grep a | sort –r .
2.	Input and Output redirection:
It supports input and output redirection . commands like 
ls –l > output.out and grep abc < output.out are supported.
Also Redirection with multipipe command is also supported.
3.	JOB Controlling :
It supports Job Controlling .when a new process runs in background it creates its own process group.
we can use commands like 
jobs , and & should be use at last of command to run the process in background.

4.	History :
It supports history feature. we can see history by typing “history” command .
!n = executes nth command in history.
!! = executes last executed command.
5.	TAB auto completion:
It supports TAB auto completion. We can press TAB to auto complete with matching files.
6.	UP and DOWN to navigate between previous commands :

/*AJAY YADAV M.Tech. CSE Ist Year IIT Roorkee*/

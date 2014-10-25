
<h1>MYSHELL-a mini shell</h1>


<b>Prerequisites:</b><br>
<b>Dependency:</b> please install these libraries before running this program => 
<b>libreadline6-dev</b> and <b>libreadline6</b>. <br>
in <b>UBUNTU:</b> to install libreadline6 and libreadline6-dev type: <br>
sudo apt-get install libreadline6 libreadline6-dev<br>
To compile and run program :<br>
Open terminal and type: <br>
<i>gcc –o mshell ajay_myshell.c –lreadline –lhistory</i>
<br>Now run the program by typing:
<br><i>./mshell</i>
<br><b>Note: Execute all the command by providing space between each argument. </b><br>
<b>List of Commands:</b><br>
<b>Built-in commands:</b><br>
1.	clear<br>
2.	cd<br>
3.	mkdir<br>
4.	rmdir<br>
5.	echo $Variable<br>
6.	history<br>
7.	!n<br>
8.	!!<br>
<b>External Commands:</b><br>
All commands that are in path like ls ,ps ,grep , sort etc.
<br>
<b>Supported Features:</b><br>
1.	Multipipe commands:<br>
It supports multipipe command like ls –l | grep a | sort –r .<br>
2.	Input and Output redirection:<br>
It supports input and output redirection . commands like 
ls –l > output.out and grep abc < output.out are supported.
Also Redirection with multipipe command is also supported.<br>
3.	JOB Controlling :<br>
It supports Job Controlling .when a new process runs in background it creates its own process group.
we can use commands like 
jobs , and & should be use at last of command to run the process in background.
<br>
4.	History :<br>
It supports history feature. we can see history by typing “history” command .<br>
!n = executes nth command in history.<br>
!! = executes last executed command.<br>
5.	TAB auto completion:<br>
It supports TAB auto completion. We can press TAB to auto complete with matching files.<br>
6.	UP and DOWN to navigate between previous commands :<br>

/*AJAY YADAV M.Tech. CSE Ist Year IIT Roorkee*/

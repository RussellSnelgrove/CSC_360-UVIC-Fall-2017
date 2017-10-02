Process manager

Building the program:

Enter into the command line:
		make  			--> This compiles the code pman.c into PMan
		./PMan			--> This runs the process manager
		
Compile whatever other code you want at this point.
		
Commands in pman.c:
		bg command 		--> Runs a program that is in the input please do not forget the "./" after bg, ex. < bg ./inf hello 5 >
		bgkill pid		--> kills a process that is running, ex . < bgkill 21665 >
		bgstart pid		--> restarts a stopped proccess, ex. < bgstart 21665 >
		bgstop pid		--> stops a running proccess, ex . < bgstop 21665 >
		bglist			--> Shows a list of processes in the linked list, ex. < bglist() >
		pstat pid		--> list the information about the pid, ex. < pstat 21665 >
		exit			--> Own personal function to exit the program, ex. < exit >		
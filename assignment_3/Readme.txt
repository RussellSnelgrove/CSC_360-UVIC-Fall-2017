CSC 360 Assignment 360
Russell Snelgrove



Building the program:

Enter into the command line:
		make  			--> This compiles the code diskinfo.c disklist.c diskget.c diskput.c into diskinfo disklist diskget and diskput respectivly
		./diskinfo	<disk.IMA>							--> This checks the information that is invloved in the disk
		./disklist	<disk.IMA>							--> This lists the files that are in the disk and information about the files
		./diskget	<disk.IMA> <file you want>			--> This grabs the file that you want from the disk.IMA
		./diskput	<disk.IMA> <file you want to input>	--> This inputs the file that you want into the disk.IMA

		
Diskinfo (Part 1) should output:
		OS Name:
		Label of the disk:
		Total size of the disk:	
		Free size of the disk:
		==============
		The number of files in the root directory (not including subdirectories):
		=============
		Number of FAT copies:
		Sectors per FAT:

Disklist (Part 2) should output:
		1. The first column will contain:
			(a) F for regular files, or
			(b) D for directories;
				followed by a single space
		2. then 10 characters to show the file size in bytes, followed by a single space
		3. then 20 characters for the file name, followed by a single space
		4. then the file creation date and creation time.

Diskget (Part 3) should output:
		If your code runs correctly, ANS1.PDF should be copied to your current Linux directory, and you should be able
		to read the content of ANS1.PDF.

Diskput (Part 4)should output:
		Note that a correct execution should update FAT and related allocation information in disk.IMA accordingly.
		To validate, you can use diskget implemented in Part III to check if you can correctly read foo.txt from the file
		system

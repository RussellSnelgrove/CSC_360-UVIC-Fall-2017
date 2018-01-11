/**
CSC 360 Assignment 3 part 2

Russell Snelgrove

*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include<sys/types.h>




//returns the size of the file that will be inserted to the IMA file
int find_file_size(char* fileName, char* disk_pointer) {
	while (disk_pointer[0] != 0x00) {
		if ((disk_pointer[11] & 0b00000010) == 0 && (disk_pointer[11] & 0b00001000) == 0) {
			char* currentFileName = malloc(sizeof(char));
			char* currentFileExtension = malloc(sizeof(char));
			int k;
			for (k = 0; k < 8; k++) {
				if (disk_pointer[k] == ' ') {
					break;
				}
				currentFileName[k] = disk_pointer[k];
			}
			for (k = 0; k < 3; k++) {
				currentFileExtension[k] = disk_pointer[k+8];
			}
			strcat(currentFileName, ".");
			strcat(currentFileName, currentFileExtension);

			if (strcmp(fileName, currentFileName) == 0) {
				int fileSize = (disk_pointer[28] & 0xFF) + ((disk_pointer[29] & 0xFF) << 8) + ((disk_pointer[30] & 0xFF) << 16) + ((disk_pointer[31] & 0xFF) << 24);
				return fileSize;
			}
		}
		disk_pointer= disk_pointer + 32;
	}
	return -1;
}



//this function prints the same into the file directory 
void directory_listing(char* disk_pointer) {
	while (disk_pointer[0] != 0x00) {
		char type_of_file;
		if (0b00010000 == (disk_pointer[11] & 0b00010000)) {
			type_of_file = 'D';
		} else {
			type_of_file = 'F';
		}
		char* fileName = malloc(sizeof(char));
		int k;
		for (k = 0; k < 8; k++) {
			if (disk_pointer[k] == ' ') {
				break;
			}
			fileName[k] = disk_pointer[k];
			//printf("%c ", fileName[k]);
		}

		char* file_entend = malloc(sizeof(char));
		for (k = 0; k < 3; k++) {
			file_entend[k] = disk_pointer[k+8];
			//printf("%c ", file_entend[k]);
		}

		strcat(fileName, ".");
		strcat(fileName, file_entend);
		
		int fileSize = find_file_size(fileName, disk_pointer);
		//printf("%d", fileSize);
		int year = (((disk_pointer[17] & 0b11111110)) >> 1) + 1980;
		int month = ((disk_pointer[16] & 0b11100000) >> 5) + (((disk_pointer[17] & 0b00000001)) << 3);
		int day = (disk_pointer[16] & 0b00011111);
		int hour = (disk_pointer[15] & 0b11111000) >> 3;
		int minute = ((disk_pointer[14] & 0b11100000) >> 5) + ((disk_pointer[15] & 0b00000111) << 3);
		if ((disk_pointer[11] & 0b00000010) == 0 && (disk_pointer[11] & 0b00001000) == 0) {
			printf("%c %10d %20s %d-%d-%d %02d:%02d\n", type_of_file, fileSize, fileName, year, month, day, hour, minute);
		}

		disk_pointer = disk_pointer + 32;
	}
}


int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Error: Please put ./diskinfo <file>\n");
		exit(1);
	}

	int file_disk = open(argv[1], O_RDWR);
	if (file_disk < 0) {
		printf("Error: unable to read disk image\n");
		exit(1);
	}
	struct stat buffer;
	fstat(file_disk, &buffer);
	char* disk_pointer = mmap(NULL, buffer.st_size, PROT_READ, MAP_SHARED, file_disk, 0);
	if (disk_pointer == MAP_FAILED) {
		printf("Error: unable to map memory\n");
		exit(1);
	}

	directory_listing(disk_pointer + 512 * 19);

	munmap(disk_pointer, buffer.st_size);
	close(file_disk);

	return 0;
}

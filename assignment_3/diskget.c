/**
CSC 360 Assignment 3 part 3

Russell Snelgrove

*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include<sys/types.h>




//returns the fat entry
int find_fat_entry(int n, char* disk_pointer) {
	int result;
	int byte_1;
	int byte_2;

	if ((n % 2) == 0) {
		byte_1 = disk_pointer[512 + ((3*n) / 2) + 1] & 0x0F;
		byte_2 = disk_pointer[512 + ((3*n) / 2)] & 0xFF;
		result = (byte_1 << 8) + byte_2;
	} else {
		byte_1 = disk_pointer[512 + (int)((3*n) / 2)] & 0xF0;
		byte_2 = disk_pointer[512 + (int)((3*n) / 2) + 1] & 0xFF;
		result = (byte_1 >> 4) + (byte_2 << 4);
	}

	return result;
}






//returns the size of the file that will be inserted to the IMA file
int find_file_size(char* name_of_file, char* disk_pointer) {
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

			if (strcmp(name_of_file, currentFileName) == 0) {
				int fileSize = (disk_pointer[28] & 0xFF) + ((disk_pointer[29] & 0xFF) << 8) + ((disk_pointer[30] & 0xFF) << 16) + ((disk_pointer[31] & 0xFF) << 24);
				return fileSize;
			}
		}
		disk_pointer= disk_pointer + 32;
	}
	return -1;
}


//this will return the first sector for the file
int find_first_sector(char* name_of_file, char* disk_pointer) {
	while (disk_pointer[0] != 0x00) {
		if ((disk_pointer[11] & 0b00000010) == 0 && (disk_pointer[11] & 0b00001000) == 0) {
			char* currentFileName = malloc(sizeof(char));
			char* currentFileExtension = malloc(sizeof(char));
			int i;
			for (i = 0; i < 8; i++) {
				if (disk_pointer[i] == ' ') {
					break;
				}
				currentFileName[i] = disk_pointer[i];
			}
			for (i = 0; i < 3; i++) {
				currentFileExtension[i] = disk_pointer[i+8];
			}

			strcat(currentFileName, ".");
			strcat(currentFileName, currentFileExtension);

			if (strcmp(name_of_file, currentFileName) == 0) {
				return (disk_pointer[26]) + (disk_pointer[27] << 8);
			}

		}
		disk_pointer = disk_pointer + 32;
	}
	return -1;
}

//puts the file from the IMA into the directory
void file_to_local_directory(char* disk_pointer, char* pointer_2, char* name_of_file) {
	int firstLogicalSector = find_first_sector(name_of_file, disk_pointer + 512 * 19);
	int n = firstLogicalSector;
	int fileSize = find_file_size(name_of_file, disk_pointer + 512 * 19);
	int bytes_left = find_file_size(name_of_file, disk_pointer + 512 * 19);
	int physicalAddress = 512 * (31 + n);

	do {
		n = (bytes_left == fileSize) ? firstLogicalSector : find_fat_entry(n, disk_pointer);
		physicalAddress = 512 * (31 + n);
		//printf("%d", physicalAddress);

		int i;
		for (i = 0; i < 512; i++) {
			if (bytes_left == 0) {
				break;
			}
			pointer_2[fileSize - bytes_left] = disk_pointer[i + physicalAddress];
			 bytes_left = bytes_left - 1; 
			 //printf("%d", bytes_left);
		}
	} while (find_fat_entry(n, disk_pointer) != 0xFFF);
}




int main(int argc, char* argv[]) {
	if (argc < 3) {
		printf("Error: Please put ./diskget <disk.IMA> <file>\n");
		exit(1);
	}

	int file_disk_int = open(argv[1], O_RDWR);
	if (file_disk_int < 0) {
		printf("Error: unable to read .IMA file\n");
		exit(1);
	}
	
	
	struct stat disk_buffer;
	fstat(file_disk_int, &disk_buffer);
	char* disk_pointer = mmap(NULL, disk_buffer.st_size, PROT_READ, MAP_SHARED, file_disk_int, 0);
	if (disk_pointer == MAP_FAILED) {
		printf("Error: Cannot find the mapped memory location\n");
		close(file_disk_int);
		exit(1);
	}

	int fileSize = find_file_size(argv[2], disk_pointer + 512 * 19);
	if (fileSize > 0) {
		int file_disk_int2 = open(argv[2], O_RDWR | O_CREAT, 0666);
		if (file_disk_int2 < 0) {
			printf("Error: Cannot open the file\n");
			close(file_disk_int);
			exit(1);
		}

		int result = lseek(file_disk_int2, fileSize-1, SEEK_SET);
		if (result == -1) {
			munmap(disk_pointer, disk_buffer.st_size);
			close(file_disk_int);
			close(file_disk_int2);
			printf("Error: unable to seek to end of file\n");
			exit(1);
		}
		
		result = write(file_disk_int2, "", 1);
		if (result != 1) {
			munmap(disk_pointer, disk_buffer.st_size);
			close(file_disk_int);
			close(file_disk_int2);
			printf("Error: unable to write last byte\n");
			exit(1);
		}

		char* disk_pointer_2 = mmap(NULL, fileSize, PROT_WRITE, MAP_SHARED, file_disk_int2, 0);
		if (disk_pointer_2 == MAP_FAILED) {
			printf("Error: unable to map file memory\n");
			exit(1);
		}

		file_to_local_directory(disk_pointer, disk_pointer_2, argv[2]);

		munmap(disk_pointer_2, fileSize);
		close(file_disk_int2);
	} else {
		printf("File was not found.\n");
	}

	munmap(disk_pointer, disk_buffer.st_size);
	close(file_disk_int);

	return 0;
}

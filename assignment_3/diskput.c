/**
CSC 360 Assignment 3 part 4

Russell Snelgrove

*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include<sys/types.h>

//Finds the  total disk size
int find_disk_size(char* pointer_1) {
	int bytesPerSector = pointer_1[11] + (pointer_1[12] << 8);
	int totalSectorCount = pointer_1[19] + (pointer_1[20] << 8);
	return bytesPerSector * totalSectorCount;
}



//returns the fat entry
int find_fat_entry(int n, char* pointer_1) {
	int result;
	int byte_1;
	int byte_2;

	if ((n % 2) == 0) {
		byte_1 = pointer_1[512 + ((3*n) / 2) + 1] & 0x0F;
		byte_2 = pointer_1[512 + ((3*n) / 2)] & 0xFF;
		result = (byte_1 << 8) + byte_2;
	} else {
		byte_1 = pointer_1[512 + (int)((3*n) / 2)] & 0xF0;
		byte_2 = pointer_1[512 + (int)((3*n) / 2) + 1] & 0xFF;
		result = (byte_1 >> 4) + (byte_2 << 4);
	}

	return result;
}

//returns the amount of free space on the disk
int find_amount_free_space_on_disk(int diskSize, char* disk_pointer) {
	int freeSector = 0;

	int i;
	for (i = 2; i < (diskSize / 512); i++) {
		if (find_fat_entry(i, disk_pointer) == 0x000) {
			freeSector= 1 + freeSector;
		}
	}

	return (512 * freeSector);
}

//checking to see if the file already exists in the directory
int file_exists(char* name_of_file, char* pointer_1) {
	while (pointer_1[0] != 0x00) {
		if ((pointer_1[11] & 0b00000010) == 0 && (pointer_1[11] & 0b00001000) == 0) {
			char* curr_file_name = malloc(sizeof(char));
			char* file_extention = malloc(sizeof(char));
			int i;
			for (i = 0; i < 8; i++) {
				if (pointer_1[i] == ' ') {
					break;
				}
				curr_file_name[i] = pointer_1[i];
			}
			for (i = 0; i < 3; i++) {
				file_extention[i] = pointer_1[i+8];
			}

			strcat(curr_file_name, ".");
			strcat(curr_file_name, file_extention);

			if (strcmp(name_of_file, curr_file_name) == 0) {
				return 1;
			}else{
				//printf("fail in the file_exists comparing two file names");
			}
		}
		pointer_1 = pointer_1 + 32;
	}
	return 0;
}


//setting the entry address of the file
void set_entry(int n, int value, char* pointer_1) {
	pointer_1 = pointer_1 + 512;

	if ((n % 2) == 0) {
		pointer_1[512 + ((3*n) / 2) + 1] = (value >> 8) & 0x0F;
		pointer_1[512 + ((3*n) / 2)] = value & 0xFF;
	} else {
		pointer_1[512 + (int)((3*n) / 2)] = (value << 4) & 0xF0;
		pointer_1[512 + (int)((3*n) / 2) + 1] = (value >> 4) & 0xFF;
	}
}

//adds the file to the root directory
void add_to_root(char* name_of_file_to_enter, int fileSize, int firstLogicalSector, char* pointer_1) {

	pointer_1 = pointer_1 + (512 * 19);
	while (pointer_1[0] != 0x00) {
		pointer_1 = pointer_1 + 32;
	}

	int finished = -1;
	int i;
	for (i = 0; i < 8; i++) {
		char character = name_of_file_to_enter[i];
		if (character == '.') {
			finished = i;
		}
		pointer_1[i] = (finished == -1) ? character : ' ';
	}
	for (i = 0; i < 3; i++) {
		pointer_1[i+8] = name_of_file_to_enter[i+finished+1];
	}

	pointer_1[11] = 0x00;

	time_t t = time(NULL);
	struct tm *now = localtime(&t);
	int year = now->tm_year + 1900;
	int month = (now->tm_mon + 1);
	int day = now->tm_mday;
	int hour = now->tm_hour;
	int minute = now->tm_min;
	pointer_1[14] = 0;
	pointer_1[15] = 0;
	pointer_1[16] = 0;
	pointer_1[17] = 0;
	pointer_1[17] |= (year - 1980) << 1;
	pointer_1[17] |= (month - ((pointer_1[16] & 0b11100000) >> 5)) >> 3;
	pointer_1[16] |= (month - (((pointer_1[17] & 0b00000001)) << 3)) << 5;
	pointer_1[16] |= (day & 0b00011111);
	pointer_1[15] |= (hour << 3) & 0b11111000;
	pointer_1[15] |= (minute - ((pointer_1[14] & 0b11100000) >> 5)) >> 3;
	pointer_1[14] |= (minute - ((pointer_1[15] & 0b00000111) << 3)) << 5;
	pointer_1[26] = (firstLogicalSector - (pointer_1[27] << 8)) & 0xFF;
	pointer_1[27] = (firstLogicalSector - pointer_1[26]) >> 8;
	pointer_1[28] = (fileSize & 0x000000FF);
	pointer_1[29] = (fileSize & 0x0000FF00) >> 8;
	pointer_1[30] = (fileSize & 0x00FF0000) >> 16;
	pointer_1[31] = (fileSize & 0xFF000000) >> 24;
}


//finding a free index to use
int find_free_index(char* pointer_1) {
	int k = 2;
	pointer_1 = pointer_1 + 512;

	while (find_fat_entry(k, pointer_1) != 0x000) {
		k++;
	}
	return k;
}


//copies the file into the IMA
void copyFile(char* pointer_1, char* mapped_to, char* name_of_file, int size_of_file) {
	if (!file_exists(name_of_file, pointer_1 + 512 * 19)) {
		int remain_bytes = size_of_file;
		int current = find_free_index(pointer_1);
		add_to_root(name_of_file, size_of_file, current, pointer_1);

		while (remain_bytes > 0) {
			int k;
			int address = 512 * (31 + current);
			for (k = 0; k < 512; k++) {
				if (remain_bytes == 0) {
					set_entry(current, 0xFFF, pointer_1);
					return;
				}
				pointer_1[k + address] = mapped_to[size_of_file - remain_bytes];
				remain_bytes = remain_bytes-1;
			}
			set_entry(current, 0x69, pointer_1);
			int next = find_free_index(pointer_1);
			set_entry(current, next, pointer_1);
			current = next;
		}
	} 
}



int main(int argc, char* argv[]) {
	if (argc < 3) {
		printf("Error: Please put ./diskput <disk.IMA> <file>\n");
		exit(1);
	}

	int file_disk = open(argv[1], O_RDWR);
	if (file_disk < 0) {
		printf("Error: Unable to read disk image\n");
		exit(1);
	}
	
	struct stat buffer;
	fstat(file_disk, &buffer);
	char* disk_pointer = mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, file_disk, 0);
	if (disk_pointer == MAP_FAILED) {
		printf("Error: Unable to map disk image memory\n");
		close(file_disk);
		exit(1);
	}
	
	int file_disk_2 = open(argv[2], O_RDWR);
	if (file_disk_2 < 0) {
		printf("Cannot find file.\n");
		close(file_disk);
		exit(1);
	}
	struct stat buffer_2;
	fstat(file_disk_2, &buffer_2);
	int size_of_file = buffer_2.st_size;
	char* pointer_2 = mmap(NULL, size_of_file, PROT_READ, MAP_SHARED, file_disk_2, 0);
	if (pointer_2 == MAP_FAILED) {
		printf("Error: Unable to map file memory\n");
		exit(1);
	}
	
	int totalDiskSize = find_disk_size(disk_pointer);
	int disk_free_space = find_amount_free_space_on_disk(totalDiskSize, disk_pointer);
	if (disk_free_space > (size_of_file-1)) {
		//printf("enough room to copy file over");
		copyFile(disk_pointer, pointer_2, argv[2], size_of_file);
	} else {
		printf("%d %d\n", disk_free_space, size_of_file);
		printf("Not enough space in the disk image.\n");
	}

	munmap(disk_pointer, buffer.st_size);
	munmap(pointer_2, size_of_file);
	close(file_disk_2);
	close(file_disk);

	return 0;
}

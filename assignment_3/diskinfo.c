/**
CSC 360 Assignment 3 part 1

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

//returns the number of sectors
int number_sectors_FAT(char* disk_pointer) {
	return disk_pointer[22] + (disk_pointer[23] << 8);
}

//returns the number of FAT copies
int num_FAT_copys(char* disk_pointer) {
	return disk_pointer[16];
}


//returns the number of files in the root
int num_root_files(char* disk_pointer) {
	int counter = 0;
	disk_pointer += 512 * 19;

	while (disk_pointer[0] != 0x00) {
		if ((disk_pointer[11] & 0b00000010) == 0 && (disk_pointer[11] & 0b00001000) == 0 && (disk_pointer[11] & 0b00010000) == 0) {
			counter++;
		}
		disk_pointer= disk_pointer + 32;
	}

	return counter;
}

// returns the label of the disk
void find_disk_label(char* disk_Label, char* disk_pointer) {
	int i;
	for (i = 0; i < 8; i++) {
		disk_Label[i] = disk_pointer[i+43];
	}

	if (disk_Label[0] == ' ') {
		disk_pointer += 512 * 19;
		while (disk_pointer[0] != 0x00) {
			if (disk_pointer[11] == 8) {
				for (i = 0; i < 8; i++) {
					disk_Label[i] = disk_pointer[i];
				}
			}
			disk_pointer = disk_pointer + 32;
		}
	}
}

//returns the string of the name of the OS
void get_OS(char* OS_name, char* disk_pointer) {
	int i;
	for (i = 0; i < 8; i++) {
		OS_name[i] = disk_pointer[i+3];
	}
}

//returns the total size of the disk
int total_size_of_disk(char* disk_pointer) {
	int bytesPerSector = disk_pointer[11] + (disk_pointer[12] << 8);
	int totalSectorCount = disk_pointer[19] + (disk_pointer[20] << 8);
	return bytesPerSector * totalSectorCount;
}



int main(int argc, char* argv[]) {
	if (argc < 2) {
		printf("Error: Please put ./diskinfo <disk.IMA> \n");
		exit(1);
	}

	int fd = open(argv[1], O_RDWR);
	if (fd < 0) {
		printf("Error: failed to read disk image\n");
		exit(1);
	}
	
	struct stat disk_buffer;
	fstat(fd, &disk_buffer);
	char* disk_pointer = mmap(NULL, disk_buffer.st_size, PROT_READ, MAP_SHARED, fd, 0);
	if (disk_pointer == MAP_FAILED) {
		printf("Error: Unable to map memory\n");
		exit(1);
	}
	//printf("Was able to map to memory");

	char* OS_name = malloc(sizeof(char));
	get_OS(OS_name, disk_pointer);
	//printf("GOT os_name");
	char* label_1 = malloc(sizeof(char));
	find_disk_label(label_1, disk_pointer);
	//printf("GOT label_1");
	
	int size_of_disk = total_size_of_disk(disk_pointer);
	//printf("GOT size_of_disk: %d", size_of_disk);
	
	int free_size_of_disk = find_amount_free_space_on_disk(size_of_disk, disk_pointer);
	//printf("GOT free_size_of_disk: %d", free_size_of_disk);
	
	int number_of_root_files = num_root_files(disk_pointer);
	//printf("GOT number_of_root_files: %d", number_of_root_files);
	
	int num_FAT_copies = num_FAT_copys(disk_pointer);
	//printf("GOT num_FAT_copies: %d", num_FAT_copies);
	
	int sector_per_a_FAT = number_sectors_FAT(disk_pointer);
	//printf("GOT secter_per_a_FAT: %d", secter_per_a_FAT);
	

	printf("OS Name: %s\n", OS_name);
	printf("Label of the disk: %s\n", label_1);
	printf("Total size of the disk: %d bytes\n", size_of_disk);
	printf("Free size of the disk: %d bytes\n\n", free_size_of_disk);
	printf("==============\n");
	printf("The number of files in the root directory (not including subdirectories): %d\n\n", number_of_root_files);
	printf("=============\n");
	printf("Number of FAT copies: %d\n", num_FAT_copies);
	printf("Sectors per FAT: %d\n", sector_per_a_FAT);

	munmap(disk_pointer, disk_buffer.st_size);
	close(fd);
	free(OS_name);
	free(label_1);

	return 0;
}

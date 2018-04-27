#define FUSE_USE_VERSION 29

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include <errno.h>
#include "compression.h"
#include "client.h"

#define SLAVE_COUNT 3
#define BUFFER_SIZE 16
#define DF_DATA ((struct df_data*) fuse_get_context()->private_data)
#define SERVER_PORT "8000"

typedef struct df_data {
	char* rootFile;
	char* slave_loc[SLAVE_COUNT];
} df_data;

typedef struct file_data {
	char* filename;
	size_t filesize;
} file_data;

/*
 * itoa implementation I found online
 */
char* itoa(int value, char* result, int base) {
	// check that the base if valid
	if (base < 2 || base > 36) { *result = '\0'; return result; }

	char* ptr = result, *ptr1 = result, tmp_char;
	int tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
	} while ( value );

	// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = '\0';
	while(ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr--= *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

/* 
 * THIS IS HEAP ALLOCATED
 */
file_data* get_file_data(char* file_info) {
	file_data* fd = malloc(sizeof(file_data));
	char* fn = strtok(file_info, ",");
	fd->filename = malloc(strlen(fn) + 1);
	strcpy(fd->filename, fn);
	fd->filesize = strtol(strtok(file_info, ","), NULL, 10);
	return fd;
}

void destroy_file_data(file_data* fd) {
	free(fd->filename);
	free(fd);
}

size_t get_slave_size(char* slave_info) {
	return strtol(slave_info, NULL, 10);
}

char* get_slave_fn(int slave_idx, char* slave_name) {
		strcpy(slave_name, "ddfs/slave");
		char slave_num[5];
		itoa(slave_idx, slave_num, 10);
		strcat(slave_name, slave_num);
		strcat(slave_name, ".csv");
		return slave_name;
}

/*
 * Toy getattr function just to test if filesystem properly mounts
 * Code taken from tutorial:
 * http://www.maastaar.net/fuse/linux/filesystem/c/2016/05/21/writing-a-simple-filesystem-using-fuse/
 */
static int df_getattr(const char* path, struct stat* st) {
	fprintf(stderr, "getattr called on path : %s\n", path);

	st->st_uid = getuid();
	st->st_gid = getgid();
	st->st_atime = time(NULL);
	if (strcmp(path, "/") == 0) {
		st->st_mode = S_IFDIR;
		st->st_nlink = 2;
	}
	else {
		st->st_mode = S_IFREG;
		st->st_nlink = 1;
		st->st_size = 1;
	}

	return 0;
}

/*
 * Allows for ls like operation on slave
 */
static int df_readdir(const char* path, void* buf, fuse_fill_dir_t filler, 
		off_t offset, struct fuse_file_info* fi) {
	(void) offset;
	(void) fi;

	fprintf(stderr, "Attempting to readdir, slave path: %s\n", path);

	FILE* server_file = fopen(path, "r");
	if (!server_file) {
		fprintf(stderr, "Slave not found!\n");
		return 1;
	}

	char fi_buff[1024];
	while (fgets(fi_buff, 1024, server_file) != NULL) {
		if (!strcmp(fi_buff, "\n")) {
			continue;
		}
		file_data* cur_fi = get_file_data(fi_buff);
		filler(buf, cur_fi->filename, NULL, 0);
		destroy_file_data(cur_fi);
	}

	return 0;
}

int slave_contains(const char* slave, const char* target_file) {
	FILE* slave_file = fopen(slave, "r");
	if (slave_file == NULL) {
		fprintf(stderr, "Slave at %s not found\n", slave);
		return -1;
	}
	char fi_buff[1024];
	while (fgets(fi_buff, 1024, slave_file) != NULL) {
		if (!strcmp(fi_buff, "\n")) {
			continue;
		}
		file_data* cur_fi = get_file_data(fi_buff);
		if (strcmp(cur_fi->filename, target_file) == 0) {
			fprintf(stderr, "cur_fi: %s\ntarget: %s\n", cur_fi->filename, target_file);
			return cur_fi->filesize;
		}
		destroy_file_data(cur_fi);
	}
	return -1;
}

static int df_create(const char* path, mode_t mode, struct fuse_file_info* fi) {
	(void) fi;
	fprintf(stderr, "Create was called on path %s with mode %d\n", path, mode);
	return 0;
}

/*
 * Opens a stream that will read in a string
 */
static int df_open(const char* path, struct fuse_file_info* fi) {
	fprintf(stderr, "Attempting to open: %s\n", path);
	(void) fi;
	char slave_fn[128];
	for (int s = 0; s < SLAVE_COUNT; s++) {
		get_slave_fn(s, slave_fn);
		if (slave_contains(slave_fn, path) != -1) {
			return 0;
		}
	}

	fprintf(stderr, "File not found!\n");
	return 0;
}

int find_file_on_server(char* path) {
	char slave_fn[128];
	int slave_idx = -1;
	for (int s = 0; s < SLAVE_COUNT; s++) {
		get_slave_fn(s, slave_fn);
		if (slave_contains(slave_fn, path) != -1) {
			slave_idx = s;
			break;
		}
	}

	fprintf(stderr, "Returning slave_idx: %d\n", slave_idx);
	return slave_idx;
}
/*
 * This method is so dumb but it works
 */
int update_slave_metadata(char* target_file, int file_size, int slave_idx) {
	fprintf(stderr, "Updating slave metadata for %s to size %d on slave %d\n",
			target_file, file_size, slave_idx);
	char slave_fn[128];
	get_slave_fn(slave_idx, slave_fn);
	FILE* slave_file = fopen(slave_fn, "r+");
	if (slave_file == NULL) {
		return -1;
	}
	FILE* new_slave = fopen("ddfs/new_file.csv", "w+");
	char new_metadata [1024];
	sprintf(new_metadata, "%s,%d", target_file, file_size);
	fwrite(new_metadata, 1, strlen(new_metadata) + 1, new_slave);
	char fi_buff[1024];
	while (fgets(fi_buff, 1024, slave_file) == NULL) {
		if (!strcmp(fi_buff, "\n")) {
			continue;
		}
		file_data* cur_fi = get_file_data(fi_buff);
		if (strcmp(cur_fi->filename, target_file) == 0) {
			destroy_file_data(cur_fi);
			continue;
		}
		sprintf(new_metadata, "%s,%d\n", target_file, file_size);
		fwrite(new_metadata, 1, strlen(new_metadata) + 1, new_slave);
		destroy_file_data(cur_fi);
	}
	remove(slave_fn);
	rename("ddfs/new_file.csv", slave_fn);
	return 0;
}

int add_slave_metadata(char* target_file, int file_size, int slave_idx) {
	fprintf(stderr, "Adding file %s of size %d to slave %d\n", target_file, file_size, slave_idx);
	char slave_fn[128];
	get_slave_fn(slave_idx, slave_fn);
	FILE* slave_file = fopen(slave_fn, "a");
	if (slave_file == NULL) {
		return -1;
	}
	char new_metadata [1024];
	sprintf(new_metadata, "%s,%d\n", target_file, file_size);
	fwrite(new_metadata, 1, strlen(new_metadata) + 1, slave_file);
	return 0;
}

int update_all_metadata(int slave_idx, int del_size) {
	FILE* all_md = fopen("ddfs/all_servers.csv", "r");
	FILE* temp_md = fopen("ddfs/temp_md.csv", "w+");
	char fi_buff[128];
	for (int i = 0; i < SLAVE_COUNT; i++) {
		fgets(fi_buff, 128, all_md);
		if (i == slave_idx) {
			int slave_size = get_slave_size(fi_buff);
			int new_size = slave_size + del_size;
			char new_size_buff[128];
			sprintf(new_size_buff, "%d\n", new_size);
			fwrite(fi_buff, 1, strlen(new_size_buff) + 1, temp_md);
		}
		else {
			fwrite(fi_buff, 1, strlen(fi_buff) + 1, temp_md);
		}
	}
	remove("ddfs/all_servers.csv");
	rename("ddfs/temp_md.csv", "ddfs/all_servers.csv");
	return 0;
}

static int df_read(const char* path, char* buf, size_t size, 
		off_t offset, struct fuse_file_info* fi) {
	fprintf(stderr, "Attempting to read: %s\n", path);
	(void) size;
	(void) offset;
	(void) fi;

	int slave_idx = find_file_on_server((char*)path);
	if (slave_idx == -1) {
		fprintf(stderr, "File was not found on any servers!\n");
		return 1;
	}

	fprintf(stderr, "File was found on server idx: %d\n", slave_idx);
	int file_len;
	char* received_data = network_receive(path, SERVER_PORT, DF_DATA->slave_loc[slave_idx], &file_len);
	memcpy(buf, received_data, file_len);
	return 0;
}

static int df_write_slave(const char* path, size_t slave_idx, const char* buf, size_t size) {
	(void) size;
	(void) slave_idx;
	//Sends request to slave
	int data_size;
	char* comp_enc = compress((char*)buf, &data_size);
	int net_stat = network_send(comp_enc, path, SERVER_PORT, DF_DATA->slave_loc[slave_idx], data_size);
	free(comp_enc);
	return net_stat;
}

static int df_write(const char* path, const char* buf, size_t size, 
		off_t offset, struct fuse_file_info* fi) {
	(void) offset;
	(void) fi;

	fprintf(stderr, "Attempting to write: %s\n", path);
	FILE* server_file = fopen("ddfs/all_servers.csv", "r");
	if (!server_file) {
		fprintf(stderr, "All server metadata not found!\n");
		return 1;
	}

	int slave_idx = -1;
	int find_slave_idx = find_file_on_server((char*)path);
	fprintf(stderr, "fsi returned %d\n", find_slave_idx);
	if (find_slave_idx != -1) {
		slave_idx = find_slave_idx;
		update_slave_metadata((char*)path, size, slave_idx);
	}
	else {
		size_t min_size = 0;
		size_t min_slave = 0;
		size_t cur_slave = 0;
		char fi_buff[1024];
		while (fgets(fi_buff, 1024, server_file) != NULL) {
			size_t cur_size = get_slave_size(fi_buff);
			if (cur_size < min_size) {
				min_size = cur_size;
				min_slave = cur_slave;
			}
			cur_slave++;
		}
		slave_idx = min_slave;
		add_slave_metadata((char*)path, size, slave_idx);
		update_all_metadata(slave_idx, -min_size);
	}

	return df_write_slave(path, slave_idx, buf, size);
}

/*
 * Check that all metadata files exist, if not create them
 * Does not create directories, handled in makefile
 */
static void* df_init(struct fuse_conn_info* conn) {
	(void) conn;
	FILE* as_ptr = fopen("ddfs/all_servers.csv", "r");
	if (as_ptr == NULL) {
		fprintf(stderr, "Creating all_servers file\n");
		as_ptr = fopen("ddfs/all_servers.csv", "w+");
		char md_buff[3 * SLAVE_COUNT];
		md_buff[0] = '\0';
		for (int i = 0; i < SLAVE_COUNT; i++) {
			strcat(md_buff, "0\n");
		}
		fwrite(md_buff, 3, SLAVE_COUNT, as_ptr);
	}
	for (int f = 0; f < SLAVE_COUNT; f++) {
		char slave_name[128];
		get_slave_fn(f, slave_name);
		FILE* cur_fptr = fopen(slave_name, "r");
		if (cur_fptr == NULL)
			fopen(slave_name, "w+");
	}
	return NULL;
}

static struct fuse_operations df_oper = {
	.getattr	= df_getattr,
	.readdir	= df_readdir,
	.open		= df_open,
	.create		= df_create,
	.read		= df_read,
	.write		= df_write,
	.init		= df_init,
};

/*
void set_slave_locations() {

}
*/

int main(int argc, char* argv[]) {
	//This data is malloc'd but will be used for the lifetime of the filesystem
	df_data* dfd = malloc(sizeof(df_data));

	dfd->slave_loc[0] = "localhost";
	dfd->slave_loc[1] = "localhost";
	dfd->slave_loc[2] = "localhost";

	argv[argc - 2] = argv[argc - 1];
	argv[argc - 1] = NULL;
	argc--;
	return fuse_main(argc, argv, &df_oper, dfd);
}

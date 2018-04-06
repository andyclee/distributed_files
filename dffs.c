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
#include "compression.h"

#define SLAVE_COUNT 3
#define BUFFER_SIZE 16
#define DF_DATA ((struct df_data*) fuse_get_context()->private_data)

typedef struct df_data {
	char* rootFile;
	char* slaveLoc[SLAVE_COUNT];
} df_data;

typedef struct file_data {
	char* filename;
	size_t filesize;
} file_data;

static void df_getpath(char* fpath, const char* path) {
	strcpy(fpath, DF_DATA->rootFile);
	strcat(fpath, path + 1);
}

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
		strcpy(slave_name, "slave");
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
	char realPath[PATH_MAX];
	df_getpath(realPath, path);

	st->st_uid = getuid();
	st->st_gid = getgid();
	st->st_atime = time(NULL);
	st->st_mtime = time(NULL);

	if (strcmp(realPath, "/") == 0) {
		st->st_mode = S_IFDIR | 0755;
		st->st_nlink = 2;
	}
	else {
		st->st_mode = S_IFREG | 0644;
		st->st_nlink = 1;
		st->st_size = 1024;
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
		file_data* cur_fi = get_file_data(fi_buff);
		filler(buf, cur_fi->filename, NULL, 0);
		destroy_file_data(cur_fi);
	}

	return 0;
}

bool slave_contains(const char* slave, const char* target_file) {
	FILE* slave_file = fopen(slave, "r");
	char fi_buff[1024];
	while (fgets(fi_buff, 1024, slave_file) == NULL) {
		file_data* cur_fi = get_file_data(fi_buff);
		if (strcmp(cur_fi->filename, target_file) == 0)
			return true;
		destroy_file_data(cur_fi);
	}
	return false;
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
		if (slave_contains(slave_fn, path)) {
			return 0;
		}
	}

	fprintf(stderr, "File not found!\n");
	return 1;
}

static int df_read(const char* path, char* buf, size_t size, 
		off_t offset, struct fuse_file_info* fi) {
	fprintf(stderr, "Attempting to read: %s\n", path);
	(void) buf;
	(void) size;
	(void) offset;
	(void) fi;

	char slave_fn[128];
	for (int s = 0; s < SLAVE_COUNT; s++) {
		get_slave_fn(s, slave_fn);
		if (slave_contains(slave_fn, path)) {
			return 0;
		}
	}
	return 1;
}

static int df_write_slave(const char* path, size_t slave_idx, const char* buf, size_t size) {
	(void) path;
	(void) buf;
	(void) size;
	(void) slave_idx;
	//Sends request to slave
	return 0;
}

static int df_write(const char* path, const char* buf, size_t size, 
		off_t offset, struct fuse_file_info* fi) {
	(void) offset;
	(void) fi;

	fprintf(stderr, "Attempting to write: %s\n", path);
	FILE* server_file = fopen("all_servers.csv", "r");
	if (!server_file) {
		fprintf(stderr, "All servers not found!\n");
		return 1;
	}

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

	df_write_slave(path, min_slave, buf, size);
	return 0;
}

/*
 * Check that all metadata files exist, if not create them
 * Does not create directories, handled in makefile
 */
static void* df_init(struct fuse_conn_info* conn) {
	(void) conn;
	FILE* as_ptr = fopen("all_servers.csv", "r");
	if (as_ptr == NULL) {
		as_ptr = fopen("all_servers.csv", "w+");
		fwrite("0\n", 3, SLAVE_COUNT, as_ptr);
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
	.read		= df_read,
	.write		= df_write,
	.init		= df_init,
};

int main(int argc, char* argv[]) {
	//This data is malloc'd but will be used for the lifetime of the filesystem
	df_data* dfd = malloc(sizeof(df_data));
	//char* df_fp = realpath(argv[argc - 1], NULL);

	argv[argc - 2] = argv[argc - 1];
	argv[argc - 1] = NULL;
	argc--;
	return fuse_main(argc, argv, &df_oper, dfd);
}

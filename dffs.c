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
#include "client.h"

#define SLAVE_COUNT 3
#define BUFFER_SIZE 16
#define DF_DATA ((struct df_data*) fuse_get_context()->private_data)
#define SERVER_PORT "8000"
#define FN_SIZE 1024

typedef struct df_data {
	char* rootdir;
	char slave_loc[SLAVE_COUNT][FN_SIZE];
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

char* get_real_file(char* path) {
	char* realfile = malloc(strlen(path) + strlen(DF_DATA->rootdir) + 1 + 1);
	strcpy(realfile, DF_DATA->rootdir);
	strcat(realfile, "/");
	strcat(realfile, path);
	return realfile;
}

/* 
 * THIS IS HEAP ALLOCATED
 * Returns a metadata line parsed into a convenient struct
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

/*
 * Similar to get full path type helpers
 */
char* get_slave_fn(int slave_idx, char* slave_name) {
	char* slave_base = get_real_file("slave");
	strcpy(slave_name, slave_base);
	char slave_num[5];
	itoa(slave_idx, slave_num, 10);
	strcat(slave_name, slave_num);
	strcat(slave_name, ".csv");
	free(slave_base);
	return slave_name;
}

/*
 * Toy getattr function just to test if filesystem properly mounts
 * May be useful in future for quick lookups
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

	size_t list_size = 0;
	char* list_buff = malloc(1);
	list_buff[0] = '\0';
	char fi_buff[1024];
	while (fgets(fi_buff, 1024, server_file) != NULL) {
		if (!strcmp(fi_buff, "\n")) {
			continue;
		}
		file_data* cur_fi = get_file_data(fi_buff);
		filler(buf, cur_fi->filename, NULL, 0);
		size_t old_size = list_size;
		list_size = list_size + strlen(cur_fi->filename) + 2;
		list_buff = realloc(list_buff, list_size);
		if (old_size == 0)
			strcpy(list_buff, cur_fi->filename);
		else
			strcat(list_buff, cur_fi->filename);
		strcat(list_buff, "\n");
		destroy_file_data(cur_fi);
	}
	
	//TODO: Add network LIST call

	return 0;
}

/*
 * Checks if slave contains a file by parsing CSV file
 */
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
 * Updates CSV file by terrible, terrible file manipulation
 */
int update_slave_metadata(char* target_file, int file_size, int slave_idx) {
	fprintf(stderr, "Updating slave metadata for %s to size %d on slave %d\n",
			target_file, file_size, slave_idx);
	char slave_fn[128];
	slave_fn[0] = '\0';
	get_slave_fn(slave_idx, slave_fn);
	FILE* slave_file = fopen(slave_fn, "r+");
	if (slave_file == NULL) {
		return -1;
	}
	char* new_slave_path = get_real_file("new_file.csv");
	FILE* new_slave = fopen(new_slave_path, "w+");
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
	fclose(slave_file);
	fclose(new_slave);
	remove(slave_fn);
	rename(new_slave_path, slave_fn);
	free(new_slave_path);
	return 0;
}

int add_slave_metadata(char* target_file, int file_size, int slave_idx) {
	fprintf(stderr, "Adding file %s of size %d to slave %d\n", target_file, file_size, slave_idx);
	char slave_fn[128];
	slave_fn[0] = '\0';
	get_slave_fn(slave_idx, slave_fn);
	FILE* slave_file = fopen(slave_fn, "a");
	if (slave_file == NULL) {
		return -1;
	}
	char new_metadata [1024];
	sprintf(new_metadata, "%s,%d\n", target_file, file_size);
	fwrite(new_metadata, 1, strlen(new_metadata), slave_file);
	fclose(slave_file);
	return 0;
}

int update_all_metadata(int slave_idx, int del_size) {
	fprintf(stderr, "Updating all metadata\n");
	char* all_path = get_real_file("all_servers.csv");
	char* temp_path = get_real_file("temp_md.csv");
	FILE* all_md = fopen(all_path, "r");
	FILE* temp_md = fopen(temp_path, "w+");
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
	fclose(all_md);
	fclose(temp_md);
	remove(all_path);
	rename(temp_path, all_path);
	free(all_path);
	free(temp_path);
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
		return -1;
	}

	fprintf(stderr, "File was found on server idx: %d\n", slave_idx);
	size_t file_len = 0;
	char* received_data = network_receive(path, SERVER_PORT, DF_DATA->slave_loc[slave_idx], &file_len);
	fprintf(stderr, "Returned from network_receive call with file_len: %zu\n", file_len);
	if (!file_len)
		return 0;
	buf = malloc(file_len);
	fprintf(stderr, "Past malloc\n");
	memcpy(buf, received_data, file_len);
	return (int)file_len;
}

static int df_write_slave(const char* path, int slave_idx, const char* buf, size_t size) {
	//Sends request to slave
	fprintf(stderr, "About to network send to: %s\n", DF_DATA->slave_loc[slave_idx]);
	int net_stat = network_send((char*)buf, path, SERVER_PORT, DF_DATA->slave_loc[slave_idx], size);
	fprintf(stderr, "Network status: %d\n", net_stat);
	return size;

	/*
	if (net_stat)
		return size;
	else
		return net_stat;
	*/
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

void set_slave_locations(df_data* dfd) {
	char* slave_loc0 = "localhost";
	char* slave_loc1 = "localhost";
	char* slave_loc2 = "localhost";
	strcpy(dfd->slave_loc[0], slave_loc0);
	strcpy(dfd->slave_loc[1], slave_loc1);
	strcpy(dfd->slave_loc[2], slave_loc2);
}

/*
 * Check that all metadata files exist, if not create them
 * Does not create directories, handled in makefile
 */
static void* df_init(struct fuse_conn_info* conn) {
	char* all_file = get_real_file("all_servers.csv");
	FILE* as_ptr = fopen(all_file, "r");
	if (as_ptr == NULL) {
		fprintf(stderr, "Creating all_servers file: %s\n", all_file);
		as_ptr = fopen(all_file, "w+");
		char write_buff[2];
		strcpy(write_buff, "0\n");
		for (int i = 0; i < SLAVE_COUNT; i++) {
			fwrite(write_buff, 1, 2, as_ptr);
		}
		fclose(as_ptr);
	}
	for (int f = 0; f < SLAVE_COUNT; f++) {
		char slave_name[128];
		get_slave_fn(f, slave_name);
		FILE* cur_fptr = fopen(slave_name, "r");
		if (cur_fptr == NULL) {
			fopen(slave_name, "w+");
			fclose(cur_fptr);
		}
	}

	free(all_file);
	df_data* dfd = DF_DATA;
	return dfd;
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

int main(int argc, char* argv[]) {
	argv[argc - 2] = argv[argc - 1];
	argv[argc - 1] = NULL;
	argc--;

	df_data* dfd = malloc(sizeof(df_data));
	set_slave_locations(dfd);
	dfd->rootdir = realpath("ddfs", NULL);

	return fuse_main(argc, argv, &df_oper, dfd);
}

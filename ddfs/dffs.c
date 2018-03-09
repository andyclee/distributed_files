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
#include <limits.h>

#define SLAVE_COUNT 3
#define BUFFER_SIZE 16
#define DF_DATA ((struct df_data*) fuse_get_context()->private_data)

typedef struct df_data {
	char* rootFile;
	char* slaveLoc[SLAVE_COUNT];
} df_data;

static void df_getpath(char* fpath, const char* path) {
	strcpy(fpath, DF_DATA->rootFile);
	strcat(fpath, path + 1);
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

	char realPath[PATH_MAX];
	df_getpath(realPath, path);
	fprintf(stderr, "Attempting to readdir, path: %s\n", realPath);

	DIR* dirPtr = opendir(realPath);
	if (dirPtr == NULL)
		fprintf(stderr, "Directory unabled to be opened\n");

	struct dirent* de = readdir(dirPtr);
	if (de == 0) {
		fprintf(stderr, "An error occured while reading directory with fh: %zu\n", fi->fh);
		return 1;
	}

	while ((de = readdir(dirPtr)) != NULL) {
		if (filler(buf, de->d_name, NULL, 0) != 0)
			return 2;
	}

	fprintf(stderr, "Returning from readdir\n");
	return 0;
}

/*
 * Opens a stream that will read in a string
 */
static int df_open(const char* path, struct fuse_file_info* fi) {
	fprintf(stderr, "Attempting to open: %s\n", path);
	(void) fi;
	return 0;
}

/*
 * Take a string and distribute it amongst the slaves
 */
static int df_read(const char* path, char* buf, size_t size, 
		off_t offset, struct fuse_file_info* fi) {
	fprintf(stderr, "Attempting to read: %s\n", path);
	(void) buf;
	(void) size;
	(void) offset;
	(void) fi;
	return 0;
}

static int df_write_slave(const char* path, const char* slave, const char* buf, size_t size) {
	(void) path;
	(void) buf;
	(void) size;
	(void) slave;
	//Sends request to slave
	return 0;
}

static int df_write(const char* path, const char* buf, size_t size, 
		off_t offset, struct fuse_file_info* fi) {
	(void) offset;
	(void) fi;

	fprintf(stderr, "Attempting to write: %s\n", path);
	char* smallestSlave = NULL;
	size_t smallestSize = MAX_INT;
	for (int i = 0; i < SLAVE_COUNT; i++) {
		struct stat* thisSlave = NULL;
		df_getattr(DF_DATA->slaveLoc[i], thisSlave);
		if (thisSlave->size < smallestSize) {
			smallestSize = thisSlave;
			smallestSlave = DF_DATA->slaveLoc[i];
		}
	}

	df_write_slave(path, smallestSlave, buf, size);
	return 0;
}

static void df_init(struct fuse_conn_info* conn) {
	//Place locations of slaves here
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
	char* df_fp = realpath(argv[argc - 1], NULL);
	dfd->root_file = df_fp;

	argv[argc - 2] = argv[argc - 1];
	argv[argc - 1] = NULL;
	argc--;
	return fuse_main(argc, argv, &df_oper, dfd);
}

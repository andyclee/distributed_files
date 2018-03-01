#define FUSE_USE_VERSION 29

#include <fuse.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#define SLAVE_COUNT

/*
 * Toy getattr function just to test if filesystem properly mounts
 * Code taken from tutorial:
 * http://www.maastaar.net/fuse/linux/filesystem/c/2016/05/21/writing-a-simple-filesystem-using-fuse/
 */
static int df_getattr(const char* path, struct stat* st) {
	st->st_uid = getuid();
	st->st_gid = getgid();
	st->st_atime = time(NULL);
	st->st_mtime = time(NULL);

	if (strcmp(path, "/") == 0) {
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
 * Opens a stream that will read in a string
 */
static int df_open(const char* path) {

}

/*
 * Take a string and distribute it amongst the slaves
 */
static int df_read(const char* path, char* buf, size_t size, off_t offset) {

}

static int df_write_slave(const char* path, const char* buf, size_t size) {

}

static int df_write(const char* path, const char* buf, size_t size, off_t offset) {

}

static struct fuse_operations operations = {
	.getattr	= df_getattr,
	.open		= df_open,
	.read		= df_read,
	.write		= df_write,
};

int main(int argc, char* argv[]) {
	return fuse_main(argc, argv, &operations, NULL);
}

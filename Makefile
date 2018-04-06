COMPILER = gcc
FILESYSTEM_FILES = ddfs/dffs.c
FUSE_FLAGS = -D_FILE_OFFSET_BITS=64
COMPILE_FLAGS = -Wall -Wextra

all: ddfs client

ddfs:
	$(COMPILER) $(COMPILE_FLAGS) $(FUSE_FLAGS) -o dffs $(FILESYSTEM_FILES) `pkg-config fuse --cflags --libs`
	@echo 'Mount by: ./dffs -o auto_unmount rootdir mountdir'
	@echo 'Use flag '-d' before '-o' for debug mode'
	@echo 'Manually unomount by: fusermount -u mountdir'

client:
	$(COMPILER) $(COMPILER_FLAGS) -o client_app client_app.c

clean:
	@-rm dffs

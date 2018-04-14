COMPILER = gcc
FUSE_FLAGS = -D_FILE_OFFSET_BITS=64
COMPILE_FLAGS = -Wall -Wextra
ROOT_DIR = ddfs/rootdir
MOUNT_DIR = ddfs/mountdir
DFFS_DEP = compression.o encryption.o

.PHONY: ddfs

all: master client

ddfs: $(DFFS_DEP)
	@mkdir -p $(ROOT_DIR)
	@mkdir -p $(MOUNT_DIR)
	$(COMPILER) $(COMPILE_FLAGS) $(FUSE_FLAGS) $(DFFS_DEP) -o dffs dffs.c `pkg-config fuse --cflags --libs`
	@echo '----------------------------------------------------'
	@echo 'Mount by: ./dffs -o auto_unmount rootdir mountdir'
	@echo 'Use flag '-d' before '-o' for debug mode'
	@echo 'Manually unomount by: fusermount -u mountdir'
	@echo '----------------------------------------------------'

%.o: %.c
	@echo 'Object file creation called'
	$(COMPILER) -c $(COMPILE_FLAGS) $*.c
	$(COMPILER) -MM $(COMPILE_FLAGS) $*.c > $*.d

client:
	$(COMPILER) $(COMPILE_FLAGS) -o client_app client_app.c

master: ddfs
	$(COMPILER) $(COMPILE_FLAGS) -o master_app master_app.c

clean:
	@-rm -f dffs
	@-rm -f master_app
	@-rm -f client_app
	@-rm -f compression.o
	@-rm -f encryption.o
	@-rm -f o

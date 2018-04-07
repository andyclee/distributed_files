COMPILER = gcc
FUSE_FLAGS = -D_FILE_OFFSET_BITS=64
COMPILE_FLAGS = -Wall -Wextra
ROOT_DIR = ddfs/rootdir
MOUNT_DIR = ddfs/mountdir

.PHONY: ddfs

all: master client

ddfs:
	@mkdir -p $(ROOT_DIR)
	@mkdir -p $(MOUNT_DIR)
	$(COMPILER) $(COMPILE_FLAGS) $(FUSE_FLAGS) -o dffs dffs.c `pkg-config fuse --cflags --libs`
	@echo '----------------------------------------------------'
	@echo 'Mount by: ./dffs -o auto_unmount rootdir mountdir'
	@echo 'Use flag '-d' before '-o' for debug mode'
	@echo 'Manually unomount by: fusermount -u mountdir'
	@echo '----------------------------------------------------'

client:
	$(COMPILER) $(COMPILE_FLAGS) -o client_app client_app.c

master: ddfs
	$(COMPILER) $(COMPILE_FLAGS) -o master_app master_app.c

clean:
	@-rm -f dffs
	@-rm -f master_app
	@-rm -f client_app

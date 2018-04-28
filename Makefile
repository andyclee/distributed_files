COMPILER = gcc
FUSE_FLAGS = -D_FILE_OFFSET_BITS=64
COMPILE_FLAGS = -Wall -Wextra
ROOT_DIR = ddfs/rootdir
MOUNT_DIR = ddfs/mountdir
DFFS_DEP = compression.o encryption.o client.o

.PHONY: ddfs

all: master client slave

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

client: network_client
	$(COMPILER) $(COMPILE_FLAGS) client.o -o client_app client_app.c

network_client:
	$(COMPILER) -c $(COMPILE_FLAGS) client.c

master: ddfs network_client
	$(COMPILER) $(COMPILE_FLAGS) client.o -o master_app master_app.c

slave:
	$(COMPILER) $(COMPILE_FLAGS) -o slave_app slave_app.c

clean:
	@-rm -f dffs
	@-rm -f master_app
	@-rm -f client_app
	@-rm -f slave_app
	@-rm -f compression.o
	@-rm -f encryption.o
	@-rm -f client.o
	@-rm -f o
	@-rm -f compression.d
	@-rm -f encryption.d

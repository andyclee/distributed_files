# distributed_files
A command line tool that distributes a compressed and encrypted file across a network

DEPENDENCIES:

* libfuse-dev (v2.9.4-lubuntu3.1) (On Ubuntu, apt-get install libfuse-dev)
* pkg-config (Should be default installed but is sometimes not, on Ubuntu can also apt-get install pkg-config)

USAGE:

* client_app is run from the client machine, can push, pull, and list files
* master_app is run on the master server, can run in debug mode with the 'debug' flag
* slave_app is run on the various slave servers, waits on master requests

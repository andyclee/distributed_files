# distributed_files
A command line tool that distributes a compressed and encrypted file across a network

DEPENDENCIES:

* libfuse-dev (v2.9.4-lubuntu3.1) (On Ubuntu, apt-get install libfuse-dev)
* pkg-config (Should be default installed but is sometimes not, on Ubuntu can also apt-get install pkg-config)

USAGE:
* Please change SERVER_NAME in client_app.c line 13 to the master ip address and slave_loc in dffs.c line 395-397 to the slave ip addresses
* It is ok for the client_app and master_app run on the same server, but master_app and slave_app should run on different server
* Running only client_app and slave_app is working successfully just like a normal client-server connection 

* client_app is run from the client machine, can upload, download, and list files
  Usage: ./client_app <COMMAND> <FILENAME> (Valid <COMMAND>s: upload, download, list)
* master_app is run on the master server, can run in debug mode with the 'debug' flag
  Usage: ./master_app 
* slave_app is run on the various slave servers, waits on master requests
  Usage: ./slave_app

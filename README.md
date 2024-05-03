# archidog file storage

<p align="center">
  <img src="https://github.com/islomkhodja/archidog/assets/4748226/d16169ff-10ed-4d54-b0c5-a2815e7c63b5" alt="archidog file storage" width="400" height="400">
</p>

## Description
This is an asynchronous archive server with a thread pool.

The server will archive the stream received from the client and store some files:

## Install
https://www.ics.uci.edu/~pattis/common/handouts/mingweclipse/mingw.html

https://gist.github.com/sim642/29caef3cc8afaa273ce6

https://stackoverflow.com/questions/36519453/setup-boost-in-clion

## Commands

+ "**zip** `<name> <bytes_size>`" - then the client sends a byte stream, the server archives it and saves the file.

+ "**get** `<name>`" – the client requests an archived file by name,

+ "**zip-and-get** `<name> <bytes_size>`" – the server simultaneously receives, archives, and sends back the compressed data to the client.

+ "**unzip** `<name> <byte_size>`" - then the client sends a byte stream, the server unarchives it and saves the file.

+ "**unzip-and-get** `<name> <bytes_size>`" – the server simultaneously receives, unarchives, and sends back the compressed data to the client.
### Server Configuration
Start ./server
  
| Arguments | Description |
| --- | --- |
| \-d \-\-dir=\<path\> | path to the server folder where to save files (future) |
| \-i \-\-ip=\<IP\> | IP address on which the server listener works |
| \-p \-\-port=\<uint\> | port on which the server listener works |
| \-n \-\-maxnfiles=\<uint\> | maximum number of files a client can save |
| \-\-r \-\-reqspermin=\<uint\> | maximum number of archiving requests from 1 client in 1 minute |
| \-w \-\-workers=\<uint\> | number of threads |
| \-v \-\-verbose | print detailed operations being performed by the server to stdout (stderr), without this flag print only errors |
| \-h \-\-help | print to the console arguments, how to use the program |


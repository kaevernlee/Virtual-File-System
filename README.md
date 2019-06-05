# Virtual-File-System
I've tried to implement my own virtual file system in C -- It's still a working progress but the majority of the functions work as expected! It's suppose to replicate how a file system would work on an operating system -- works with mainly binary files.

INTRO: The program takes in 3 files specifcying: directory_table, file_data and hash_data
1) When you input your own directory file: The first 64 bytes are allocated to the name of the file, if it starts with a "NULL" character, the system will assume that the entire file is deleted and the data in the file data allocated with the file name is free to overwrite. The next 8 bytes refer to the offset and length respectively of the file in the file data.
2) You can assume that the directory table file is written 72 bytes at a time with the specifications above.
3) The file data must be directory specified in the directory table -- can be analogised as random space only defined by the directory table.
4) The hash data is automatically computed as you do functions with the files within the program.f

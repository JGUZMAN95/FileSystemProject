# CSC415 Group Term Assignment - File System

To help proffesor wrote the low level LBA based read and write.  The routines are in fsLow.o, the necessary header for you to include file is fsLow.h. There are 2 key functions:



`uint64_t LBAwrite (void * buffer, uint64_t lbaCount, uint64_t lbaPosition);`

`uint64_t LBAread (void * buffer, uint64_t lbaCount, uint64_t lbaPosition);`

LBAread and LBAwrite take a buffer, a count of LBA blocks and the starting LBA block number (0 based).  

On return, these function returns the number of **blocks** read or written.

In addition, proffesor had written a hexdump utility that will allow to analyze volume file in the Hexdump subdirectory.

**Write a file system!** 

We were able to format our volume, create and maintain a free space management system, initialize a root directory and maintain directory information, create, read, write, and delete files, and display info.  See below for specifics.

Professor provided an initial “main” (fsShell.c) that will be the driver to test file system. The driver will be interactive (with all built in commands) to list directories, create directories, add and remove files, copy files, move files, and two “special commands” one to copy from the normal filesystem to our filesystem and the other from our filesystem to the normal filesystem.


The shell also calls two function in the file fsInit.c `initFileSystem` and `exitFileSystem` which are routines for us to fill in with whatever initialization and exit code we need for our file system.  

Some specifics - following interfaces:

```
int b_open (char * filename, int flags);
int b_read (int fd, char * buffer, int count);
int b_write (int fd, char * buffer, int count);
int b_seek (int fd, off_t offset, int whence);
void b_close (int fd);

```

Methods are able of locating files, and knowing which logical block addresses are associated with the file.

Directory Functions - see [https://www.thegeekstuff.com/2012/06/c-directory/](https://www.thegeekstuff.com/2012/06/c-directory/) for reference.

```
int fs_mkdir(const char *pathname, mode_t mode);
int fs_rmdir(const char *pathname);
fdDir * fs_opendir(const char *name);
struct fs_diriteminfo *fs_readdir(fdDir *dirp);
int fs_closedir(fdDir *dirp);

char * fs_getcwd(char *buf, size_t size);
int fs_setcwd(char *buf);   //linux chdir
int fs_isFile(char * path);	//return 1 if file, 0 otherwise
int fs_isDir(char * path);		//return 1 if directory, 0 otherwise
int fs_delete(char* filename);	//removes a file

struct fs_diriteminfo
	{
    unsigned short d_reclen;    /* length of this record */
    unsigned char fileType;    
    char d_name[256]; 			/* filename max filename is 255 characters */
	};
```
Finally file stats - not all the fields in the structure are needed for this assingment

```
int fs_stat(const char *path, struct fs_stat *buf);

struct fs_stat
	{
	off_t     st_size;    		/* total size, in bytes */
	blksize_t st_blksize; 		/* blocksize for file system I/O */
	blkcnt_t  st_blocks;  		/* number of 512B blocks allocated */
	time_t    st_accesstime;   	/* time of last access */
	time_t    st_modtime;   	/* time of last modification */
	time_t    st_createtime;   	/* time of last status change */
	
	/* add additional attributes here for your file system */
	};

```

A shell program designed to demonstrate our file system called fsshell.c was proviced.  It has a number of built in functions that will work 
```
ls - Lists the file in a directory
cp - Copies a file - source [dest]
mv - Moves a file - source dest
md - Make a new directory
rm - Removes a file or directory
cp2l - Copies a file from the test file system to the linux file system
cp2fs - Copies a file from the Linux file system to the test file system
cd - Changes directory
pwd - Prints the working directory
history - Prints out the history
help - Prints out help
```

Used 10,000,000 or less (minimum 500,000) bytes for the volume size and 512 bytes per sector.  These are the values to pass into startPartitionSystem.






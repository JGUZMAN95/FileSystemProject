/**************************************************************
* Class:  CSC-415
* Name: Professor Bierman
*Student ID: Chris Humphrey - 922256813 "ON THE DEADLOCK"
* Project: Basic File System
*
* File: fsLow.h
*
* Description: 
*	This is the file system interface.
*	This is the interface needed by the driver to interact with
*	your filesystem.
*
**************************************************************/
#ifndef _MFS_H
#define _MFS_H
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include "b_io.h"

#include <dirent.h>
#define FT_REGFILE	DT_REG
#define FT_DIRECTORY DT_DIR
#define FT_LINK	DT_LNK

#ifndef uint64_t
typedef u_int64_t uint64_t;
#endif
#ifndef uint32_t
typedef u_int32_t uint32_t;
#endif


struct fs_diriteminfo
	{
    unsigned short d_reclen;    /* length of this record */
    unsigned char fileType;    	/* 1 Indicates Dir, 0 Otherwise */
    char d_name[256]; 			/* filename max filename is 255 characters */
	};

typedef struct
	{
	unsigned short  d_reclen;			/*length of this record */
	unsigned short	dirEntryPosition;	/*which directory entry position, like file pos */
	uint64_t	directoryStartLocation;	/*Starting LBA of directory */
	} fdDir;

struct VCB_Struct
	{
	int Block_Size;
	int Num_Blocks;
	int Root_Location;
	int Bitmap_Location;
	};

struct File_Entry_Struct
	{
		char pathname[256];		// Really Just Name
		int Type;				// 1 for Dir, Otherwise 0
		int Num_Entries;		// Num Entries (at least 2, for "." and ".."")
		int Size;				// Size (if used)
		int Data;				// If Type = 0; Where Is The Data Stored (Block Int)
		int Entries_Block_Locations[];	// Array Holding Psuedo Pointers for Dir
	} ;							//	Arr[0] is Self Location; Arr[1] is Parent Location


int fs_mkdir(const char *pathname, mode_t mode);
int fs_rmdir(const char *pathname);
fdDir * fs_opendir(const char *name);
struct fs_diriteminfo *fs_readdir(fdDir *dirp);
int fs_closedir(fdDir *dirp);

char CWD[256];
char * fs_getcwd(char *buf, size_t size);
int fs_setcwd(char *buf);   //linux chdir
int fs_isFile(char * path);	//return 1 if file, 0 otherwise
int fs_isDir(char * path);		//return 1 if directory, 0 otherwise
int fs_delete(char* filename);	//removes a file


// Functions I Made
int findFreeBlocks(int howmanyblocks);
void fs_mkdirROOT(const char * pathname, mode_t mode);
int parser(char * pathname, int block, int count, int instance);
const char* getName(const char * pathname);
int move(char* src, char* dest);

//VCB Functions
int VCB_get_Block_Size();
int VCB_get_Num_Blocks();
int VCB_get_Root_Location();
int VCB_get_Bitmap_Array(int index);
void VCB_Flip_Bitmap_Index (int index);



#endif


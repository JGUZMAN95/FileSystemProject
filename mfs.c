/**************************************************************
 *Class:  CSC-415
 *Name: Professor Bierman
 *Student ID: Chris Humphrey - 922256813 "ON THE DEADLOCK"
 *Project: Basic File System
 *
 *File: mfs.c
 *
 *Description: Main driver for file system assignment.
 *
 *  This is the bulk of our projects code, and where we implmenet mfs.h, and deal directly with using most commands 
 *
 **************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <getopt.h>
#include <string.h>
#include "fsLow.h"
#include "mfs.h"
#include "b_io.h"

#define DIRMAX_LEN 4096
#define DIR_SIZE 2
#define FILE_SIZE 10

// What does parser do?
// It takes in input name
// based on the CWD
// It returns the parent block if file does not exist
// Else, it returns the files starting block loc index
int parser(char *pathname, int block, int count, int instance) {

    char path[256];

    strcpy(path, pathname);

    if (instance == 0) {
        instance = 1;
        char path2[256];
        char *dir_buf = malloc(DIRMAX_LEN + 1);
        char *ptr;
        ptr = fs_getcwd(dir_buf, DIRMAX_LEN);
        block = parser(ptr, VCB_get_Root_Location(), 0, 1);
    }

    char *token = strtok(path, "/");

    for (int i = 0; i < count; i++) {
        token = strtok(NULL, "/");
    }

    if (token == NULL) {
        return block;
    }

    struct File_Entry_Struct *entry;
    entry = malloc(DIR_SIZE * VCB_get_Block_Size());
    LBAread(entry, DIR_SIZE, block);

    if (strcmp(token, "..") == 0) {
        block = entry->Entries_Block_Locations[1];
    } else if (strcmp(token, ".") == 0) {
        block = entry->Entries_Block_Locations[0];
    } else if (strcmp(token, "root") == 0) {
        block = VCB_get_Root_Location();
    } else {
        struct File_Entry_Struct *entry;
        entry = malloc(DIR_SIZE * VCB_get_Block_Size());
        LBAread(entry, DIR_SIZE, block);
        for (int i = 0; i < entry->Num_Entries; i++) {
            int entry_loc = entry->Entries_Block_Locations[i];
            struct File_Entry_Struct *entry2;
            entry2 = malloc(DIR_SIZE * VCB_get_Block_Size());
            LBAread(entry2, DIR_SIZE, entry_loc);
            if (strcmp(entry2->pathname, token) == 0) {
                block = entry_loc;
            }
        }
    }

    return parser(pathname, block, count + 1, 1);

}

const char *getName(const char *pathname) {
    char path[256];
    strcpy(path, pathname);
    char *token = strtok(path, "/");
    char *name = token;
    while (token != NULL) {
        token = strtok(NULL, "/");
        if (token != NULL) {
            name = token;
        }
        return name;
    }
}

int fs_mkdir(const char *pathname, mode_t mode) {
    int Parent_Block = parser((char *) pathname, 1, 0, 0);

    int Data_Location = findFreeBlocks(DIR_SIZE);

    struct File_Entry_Struct *entry;
    entry = malloc(DIR_SIZE * VCB_get_Block_Size());

    strcpy(entry->pathname, getName(pathname));    //CHANGE THIS
    entry->Type = 1;
    entry->Num_Entries = 2;
    entry->Entries_Block_Locations[2];
    entry->Entries_Block_Locations[0] = Data_Location;
    entry->Entries_Block_Locations[1] = Parent_Block;
    LBAwrite(entry, DIR_SIZE, Data_Location);
    free(entry);

    //Add Data Block To Parent Block Data Array
    struct File_Entry_Struct *parent;

    parent = malloc(DIR_SIZE * VCB_get_Block_Size());
    LBAread(parent, DIR_SIZE, Parent_Block);
    parent->Num_Entries = parent->Num_Entries + 1;

    int array[parent->Num_Entries];
    for (int i = 0; i < parent->Num_Entries - 1; i++) {
        array[i] = parent->Entries_Block_Locations[i];
    }
    array[parent->Num_Entries - 1] = Data_Location;
    memcpy(parent->Entries_Block_Locations, array, sizeof(array));

    LBAwrite(parent, DIR_SIZE, Parent_Block);
    free(parent);
}

void fs_mkdirROOT(const char *pathname, mode_t mode) {
    int Data_Location = findFreeBlocks(DIR_SIZE);

    struct File_Entry_Struct *entry;
    entry = malloc(DIR_SIZE * VCB_get_Block_Size());
    strcpy(entry->pathname, pathname);
    entry->Type = 1;
    entry->Num_Entries = 2;
    entry->Entries_Block_Locations[2];
    entry->Entries_Block_Locations[0] = Data_Location;
    entry->Entries_Block_Locations[1] = Data_Location;
    LBAwrite(entry, DIR_SIZE, Data_Location);
    free(entry);

    struct VCB_Struct *VCBptr;
    VCBptr = malloc(4096);
    LBAread(VCBptr, 1, 0);
    VCBptr->Root_Location = Data_Location;
    LBAwrite(VCBptr, 1, 0);
    free(VCBptr);
}

//Name = CWD
fdDir *fs_opendir(const char *name) {

    int fileloc = parser((char *) name, 1, 0, 0);

    struct File_Entry_Struct *file;
    file = malloc(DIR_SIZE * VCB_get_Block_Size());
    LBAread(file, DIR_SIZE, fileloc);

    fdDir *myDir;
    myDir = malloc(DIR_SIZE * VCB_get_Block_Size());

    myDir->d_reclen = file->Num_Entries - 1;
    myDir->dirEntryPosition = 2;
    myDir->directoryStartLocation = fileloc;

    free(file);
    return myDir;
}

struct fs_diriteminfo *fs_readdir(fdDir *dirp) {
    struct File_Entry_Struct *entry;
    entry = malloc(DIR_SIZE * VCB_get_Block_Size());
    LBAread(entry, DIR_SIZE, dirp->directoryStartLocation);

    int i = dirp->dirEntryPosition;
    int Read_Loc = entry->Entries_Block_Locations[i];

    struct File_Entry_Struct *data;
    data = malloc(DIR_SIZE * VCB_get_Block_Size());
    LBAread(data, DIR_SIZE, Read_Loc);

    struct fs_diriteminfo *item;
    item = malloc(DIR_SIZE * VCB_get_Block_Size());

    strcpy(item->d_name, data->pathname);
    item->d_reclen = dirp->d_reclen;
    item->fileType = data->Type;
    if (dirp->dirEntryPosition > dirp->d_reclen) {
        free(dirp);
        free(item);
        return NULL;
    }
    dirp->dirEntryPosition = dirp->dirEntryPosition + 1;

    return item;

}

int fs_closedir(fdDir *dirp) {
    return 0;
}

int fs_rmdir(const char *pathname) {

    int Dir_Block_Location = parser((char *) pathname, 1, 0, 0);
    struct File_Entry_Struct *entry;
    entry = malloc(DIR_SIZE * VCB_get_Block_Size());
    LBAread(entry, DIR_SIZE, Dir_Block_Location);

    //Delete all Files
    for (int i = 2; i < entry->Num_Entries; i++) {

        struct File_Entry_Struct *entry2;
        entry2 = malloc(DIR_SIZE * VCB_get_Block_Size());
        LBAread(entry2, DIR_SIZE, entry->Entries_Block_Locations[i]);
        char path[256];
        strcpy(path, pathname);
        strncat(path, "/", sizeof(char) + 1);
        strncat(path, entry2->pathname, 256);

        fs_delete(path);

        VCB_Flip_Bitmap_Index(entry->Entries_Block_Locations[i]);
    }

    // Remove From Parent
    struct File_Entry_Struct *parent;
    parent = malloc(DIR_SIZE * VCB_get_Block_Size());
    LBAread(parent, DIR_SIZE, entry->Entries_Block_Locations[1]);
    parent->Num_Entries = parent->Num_Entries - 1;
    int array[parent->Num_Entries];
    int bool = 0;
    for (int i = 0; i < parent->Num_Entries; i++) {
        if (parent->Entries_Block_Locations[i] == entry->Entries_Block_Locations[0]) {
            bool = 1;
        }
        if (bool == 0) {
            array[i] = parent->Entries_Block_Locations[i];
        }
        if (bool == 1) {
            array[i] = parent->Entries_Block_Locations[i + 1];
        }
    }
    memcpy(parent->Entries_Block_Locations, array, sizeof(array));
    LBAwrite(parent, DIR_SIZE, entry->Entries_Block_Locations[1]);

    // Remove The Block Itself
    for (int i = 0; i < DIR_SIZE; i++) {
        VCB_Flip_Bitmap_Index(entry->Entries_Block_Locations[0] + i);
    }

    return 0;

}

int fs_delete(char *filename) {
    int Dir_Block_Location = parser((char *) filename, 1, 0, 0);
    struct File_Entry_Struct *entry;
    entry = malloc(DIR_SIZE * VCB_get_Block_Size());
    LBAread(entry, DIR_SIZE, Dir_Block_Location);
    if (entry->Type == 1) {
        fs_rmdir(filename);
        return 0;
    }
    if (entry->Type == 0) {
        // Not a Dir
        // Find Where Data Is Stored and Delete
        for (int i = 0; i < FILE_SIZE; i++) {
            VCB_Flip_Bitmap_Index(entry->Data + i);
        }
        // Remove From Parent
        struct File_Entry_Struct *parent;
        parent = malloc(DIR_SIZE * VCB_get_Block_Size());
        LBAread(parent, DIR_SIZE, entry->Entries_Block_Locations[1]);
        parent->Num_Entries = parent->Num_Entries - 1;
        int array[parent->Num_Entries];
        int bool = 0;
        for (int i = 0; i < parent->Num_Entries; i++) {
            if (parent->Entries_Block_Locations[i] == entry->Entries_Block_Locations[0]) {
                bool = 1;
            }
            if (bool == 0) {
                array[i] = parent->Entries_Block_Locations[i];
            }
            if (bool == 1) {
                array[i] = parent->Entries_Block_Locations[i + 1];
            }
        }
        memcpy(parent->Entries_Block_Locations, array, sizeof(array));
        LBAwrite(parent, DIR_SIZE, entry->Entries_Block_Locations[1]);

        // Remove The Block Itself
        for (int i = 0; i < DIR_SIZE; i++) {
            VCB_Flip_Bitmap_Index(entry->Entries_Block_Locations[0] + i);
        }

        return 0;
    }
}

int move(char *src, char *dest) {
    // We are not moving the data, just changing the psueod-pointer arrays

    int Src_Block_Location = parser((char *) src, 1, 0, 0);
    struct File_Entry_Struct *entry;
    entry = malloc(DIR_SIZE * VCB_get_Block_Size());
    LBAread(entry, DIR_SIZE, Src_Block_Location);

    // Remove From Parent
    struct File_Entry_Struct *parent;
    parent = malloc(DIR_SIZE * VCB_get_Block_Size());
    LBAread(parent, DIR_SIZE, entry->Entries_Block_Locations[1]);
    parent->Num_Entries = parent->Num_Entries - 1;
    int array[parent->Num_Entries];
    int bool = 0;
    for (int i = 0; i < parent->Num_Entries; i++) {
        if (parent->Entries_Block_Locations[i] == entry->Entries_Block_Locations[0]) {
            bool = 1;
        }
        if (bool == 0) {
            array[i] = parent->Entries_Block_Locations[i];
        }
        if (bool == 1) {
            array[i] = parent->Entries_Block_Locations[i + 1];
        }
    }
    memcpy(parent->Entries_Block_Locations, array, sizeof(array));
    LBAwrite(parent, DIR_SIZE, entry->Entries_Block_Locations[1]);
    free(parent);

    //Add To Dest
    int Dest_Parent_Block_Location = parser((char *) dest, 1, 0, 0);
    entry->Entries_Block_Locations[1] = Dest_Parent_Block_Location;
    struct File_Entry_Struct *ParentB;
    ParentB = malloc(DIR_SIZE * VCB_get_Block_Size());
    LBAread(ParentB, DIR_SIZE, Dest_Parent_Block_Location);

    ParentB->Num_Entries = ParentB->Num_Entries + 1;

    int array2[ParentB->Num_Entries];
    for (int i = 0; i < ParentB->Num_Entries - 1; i++) {
        array2[i] = ParentB->Entries_Block_Locations[i];
    }
    array2[ParentB->Num_Entries - 1] = Src_Block_Location;
    memcpy(ParentB->Entries_Block_Locations, array2, sizeof(array2));

    LBAwrite(ParentB, DIR_SIZE, Dest_Parent_Block_Location);
    free(ParentB);

    LBAwrite(entry, DIR_SIZE, Src_Block_Location);

    return 0;

}

int fs_isFile(char *path) {
    int Dir_Block_Location = parser((char *) path, 1, 0, 0);
    struct File_Entry_Struct *entry;
    entry = malloc(DIR_SIZE * VCB_get_Block_Size());
    LBAread(entry, DIR_SIZE, Dir_Block_Location);

    if (strcmp(getName(path), entry->pathname) == 0) { //This is a file (of any type)
        free(entry);
        return 1;
    }
    free(entry);
    return 0;
}

int fs_isDir(char *path) {

    int Dir_Block_Location = parser((char *) path, 1, 0, 0);
    struct File_Entry_Struct *entry;
    entry = malloc(DIR_SIZE * VCB_get_Block_Size());
    LBAread(entry, DIR_SIZE, Dir_Block_Location);
    if (strcmp(getName(path), entry->pathname) == 0) {
        if (entry->Type == 0) {
            //We Do Not Have A Directory File
            free(entry);
            return 0;
        } else { //We Have a Dir
            free(entry);
            return 1;
        }
    } else {
        free(entry);
        return 0;
    }
}

// 46 is the same as "." ASCII
// 47 is "/" ASCII
// We parse the input and set CWD
// After parsing, we append input to create absolute path
int fs_setcwd(char *buf) {

    if (buf[0] == 46) {
        if (buf[1] == 46) {
            //Equivalent to .. So We go Up One

            char *dir_buf = malloc(DIRMAX_LEN + 1);
            char *ptr;
            ptr = fs_getcwd(dir_buf, DIRMAX_LEN);
            int lastslash;
            for (int i = 0; i < 256; i++) {
                if (ptr[i] == 47) {
                    lastslash = i;
                }
            }
            char *ptr2 = malloc(DIRMAX_LEN + 1);
            strncpy(ptr2, ptr, lastslash);
            memcpy(CWD, ptr2, 256);

            strncat(ptr2, buf + 2, 256);
            memcpy(CWD, ptr2, 256);
        } else if (buf[1] == 47) {
            //Equivalent to ././ Simply Remove the ./
            char *dir_buf = malloc(DIRMAX_LEN + 1);
            char *ptr;
            ptr = fs_getcwd(dir_buf, DIRMAX_LEN);

            strncat(ptr, "/", sizeof(char) + 1);
            strncat(ptr, buf + 2, 256);
            memcpy(CWD, ptr, 256);
        }
    } else if (buf[0] == 47) {
        //Root
        memcpy(CWD, buf, 256);
    } else {
        char *dir_buf = malloc(DIRMAX_LEN + 1);
        char *ptr;
        ptr = fs_getcwd(dir_buf, DIRMAX_LEN);

        strncat(ptr, "/", sizeof(char) + 1);
        strncat(ptr, buf, 256);
        memcpy(CWD, ptr, 256);
    }
    return 0;
}

char *fs_getcwd(char *buf, size_t size) {
    memcpy(buf, CWD, 256);
    return buf;
}

// Returns Block Size
int VCB_get_Block_Size() {
    struct VCB_Struct *VCBptr;
    VCBptr = malloc(4096);
    LBAread(VCBptr, 1, 0);
    int Block_Size = VCBptr->Block_Size;
    free(VCBptr);
    return Block_Size;
}

//Returns Number of Blocks
int VCB_get_Num_Blocks() {
    struct VCB_Struct *VCBptr;
    VCBptr = malloc(4096);
    LBAread(VCBptr, 1, 0);
    int Num_Blocks = VCBptr->Num_Blocks;
    free(VCBptr);
    return Num_Blocks;
}

//Returns Location (Block) of Root
int VCB_get_Root_Location() {
    struct VCB_Struct *VCBptr;
    VCBptr = malloc(4096);
    LBAread(VCBptr, 1, 0);
    int Root_Loc = VCBptr->Root_Location;
    free(VCBptr);
    return Root_Loc;
}

//Input a Block Index, Array Returns 0 if Block Free, or 1 if Block Not free
int VCB_get_Bitmap_Array(int index) {
    struct VCB_Struct *VCBptr;
    VCBptr = malloc(4096);
    LBAread(VCBptr, 1, 0);
    int BS = VCB_get_Block_Size();
    int NB = VCB_get_Num_Blocks();
    int BitmapSizeBlocks = ((NB / BS) + 1);
    int Tmp[BitmapSizeBlocks * BS];
    LBAread(Tmp, BitmapSizeBlocks, VCBptr->Bitmap_Location);
    free(VCBptr);
    return Tmp[index];
}

//Input a Block Index, the Index Will Be flipped from 0 to 1 or 1 to 0
void VCB_Flip_Bitmap_Index(int index) {
    struct VCB_Struct *VCBptr;
    VCBptr = malloc(4096);
    LBAread(VCBptr, 1, 0);
    int BS = VCB_get_Block_Size();
    int NB = VCB_get_Num_Blocks();
    int BitmapSizeBlocks = ((NB / BS) + 1);
    int Tmp[BitmapSizeBlocks * BS];
    LBAread(Tmp, BitmapSizeBlocks, VCBptr->Bitmap_Location);
    Tmp[index] = 1 - Tmp[index];
    LBAwrite(Tmp, BitmapSizeBlocks, VCBptr->Bitmap_Location);
    free(VCBptr);
    return;
}

// Pass howmanyblocks you need, it returns first block index, and flips 
// a number of bitmap ints
int findFreeBlocks(int howmanyblocks) {
    int counter = 0;
    int NumBlocks = VCB_get_Num_Blocks();
    for (int i = 0; i < NumBlocks; i++) {
        if (VCB_get_Bitmap_Array(i) == 0) {
            for (int j = 0; j < howmanyblocks; j++) {
                if (VCB_get_Bitmap_Array(i + j) == 0) {
                    if ((j == howmanyblocks - 1) && ((NumBlocks - i) > howmanyblocks)) {
                        for (int q = 0; q < howmanyblocks; q++) {
                            VCB_Flip_Bitmap_Index(i + q);
                        }
                        return i;
                    }
                } else break;
            }
        }
    }
    printf("No Space - Not Enough Sequential Free Blocks \n \n");
}
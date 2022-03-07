/**************************************************************
 *Class:  CSC-415-0# Spring 2021
 *Name: 
 *Student ID: Chris Humphrey - 922256813 "ON THE DEADLOCK"
 *GitHub Name:
 *Project: Buffered I/O
 *
 *File: b_io.c
 *
 *Description: Buffered io module - Now with b_write
 *
 **************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>            // for malloc
#include <string.h>            // for memcpy
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include "b_io.h"
#include "mfs.h"
#include "fsLow.h"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512
#define DIR_SIZE 2
#define Default_File_Size 10

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct b_fcb {
    int linuxFd;//holds the systems file descriptor
    char *buf;    //holds the open file buffer
    int index;    //holds the current position in the buffer
    int buflen;    //holds how many valid bytes are in the buffer
    int size;    //Size of File
    int size2;    //Size Temp
}
        b_fcb;

b_fcb fcbArray[MAXFCBS];

int startup = 0;

void b_init()    // Made by Professor
{
    for (int i = 0; i < MAXFCBS; i++) {
        fcbArray[i].linuxFd = -1;
    }

    startup = 1;
}

int b_getFCB()    // Made by Professor
{
    for (int i = 0; i < MAXFCBS; i++) {
        if (fcbArray[i].linuxFd == -1) {
            fcbArray[i].linuxFd = -2;
            return i;
        }
    }
    return (-1);
}

int b_open(char *filename, int flags) {
    int fd;
    int returnFd;
    int size;

    if (startup == 0) b_init();

    returnFd = b_getFCB();

    char path[256];
    strcpy(path, filename);
    char *token = strtok(path, "/");
    char *name = token;
    while (token != NULL) {
        token = strtok(NULL, "/");
        if (token != NULL) {
            name = token;
        }
    }

    int Dir_Block_Location = parser((char *) filename, 1, 0, 0);

    struct File_Entry_Struct *entry;
    entry = malloc(DIR_SIZE * VCB_get_Block_Size());
    LBAread(entry, DIR_SIZE, Dir_Block_Location);

    if (strcmp(name, entry->pathname) == 0) {    //	File Already Exists
        //	Read MetaData
        if (entry->Type == 0) {
            //We Do Not Have A Directory File
            fd = entry->Data;
            size = entry->Size;
            fcbArray[returnFd].size = size;
        }

        if (entry->Type == 1) {
            //We Have A Directory File
            fd = Dir_Block_Location;
            size = DIR_SIZE * VCB_get_Block_Size();
            fcbArray[returnFd].size = size;
        }
    } else {    // File Does Not Exit
        // This is Creating The File MetaData
        int Parent_Block = parser((char *) filename, 1, 0, 0);

        int Data_Location = findFreeBlocks(10 + DIR_SIZE);

        struct File_Entry_Struct *entry;
        entry = malloc(DIR_SIZE * VCB_get_Block_Size());

        strcpy(entry->pathname, name);
        entry->Type = 0;
        entry->Size = 0;
        entry->Num_Entries = 2;
        entry->Data = Data_Location + 2;
        entry->Entries_Block_Locations[2];
        entry->Entries_Block_Locations[0] = Data_Location;
        entry->Entries_Block_Locations[1] = Parent_Block;

        fd = Data_Location + 2;
        fcbArray[returnFd].size = 0;

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

    if (fd == -1)
        return (-1);    //error opening filename

    pthread_mutex_lock(&mutex);
    fcbArray[returnFd].linuxFd = fd;    // Save the linux file descriptor
    pthread_mutex_unlock(&mutex);

    //allocate our buffer
    fcbArray[returnFd].buf = malloc(B_CHUNK_SIZE);
    if (fcbArray[returnFd].buf == NULL) {
        // very bad, we can not allocate our buffer
        close(fd);    // close linux file
        fcbArray[returnFd].linuxFd = -1;    //Free FCB
        return -1;
    }

    fcbArray[returnFd].buflen = 0;    // have not read anything yet
    fcbArray[returnFd].index = 0;    // have not read anything yet

    return (returnFd);    // all set
}

// Interface to write a buffer	
int b_write(int fd, char *buffer, int count) {
    if (startup == 0) b_init();    //Initialize our system

    // check that fd is between 0 and (MAXFCBS-1)
    if ((fd < 0) || (fd >= MAXFCBS)) {
        return (-1);    //invalid file descriptor
    }

    if (fcbArray[fd].linuxFd == -1)    //File not open for this descriptor
    {
        return -1;
    }

    // Here We Are Writing Our Buffer, of Size 512
    LBAwrite(buffer, 1, fcbArray[fd].linuxFd + fcbArray[fd].index);
    fcbArray[fd].size = fcbArray[fd].size + count;
    fcbArray[fd].index = fcbArray[fd].index + 1;
    if (count < 512) {
        fcbArray[fd].index = 0;
    }

    // We Modify MetaData as We Learn The File Size From Writing
    struct File_Entry_Struct *entry;
    entry = malloc(DIR_SIZE * VCB_get_Block_Size());
    LBAread(entry, DIR_SIZE, fcbArray[fd].linuxFd - DIR_SIZE);
    if (entry->Type == 0) {
        entry->Size = fcbArray[fd].size;
        LBAwrite(entry, DIR_SIZE, fcbArray[fd].linuxFd - DIR_SIZE);
    }
    free(entry);

}


int b_read(int fd, char *buffer, int count) {

    int bytesRead;    // for our reads
    int bytesReturned;    // what we will return
    int part1, part2, part3;    // holds the three potential copy lengths
    int numberOfBlocksToCopy;    // holds the number of whole blocks that are needed
    int remainingBytesInMyBuffer;    // holds how many bytes are left in my buffer

    if (startup == 0) b_init();    //Initialize our system

    // check that fd is between 0 and (MAXFCBS-1)
    if ((fd < 0) || (fd >= MAXFCBS)) {
        return (-1);    //invalid file descriptor
    }

    if (fcbArray[fd].linuxFd == -1)    //File not open for this descriptor
    {
        return -1;
    }

    // Buffer Size 512, Here we Read in 512 chunks and utilize size2 as a temp field
    if (fcbArray[fd].index == 0) {
        fcbArray[fd].size2 = fcbArray[fd].size;
    }
    char temp_buf[B_CHUNK_SIZE];
    LBAread(temp_buf, 1, fcbArray[fd].linuxFd + fcbArray[fd].index);
    if (fcbArray[fd].size2 >= B_CHUNK_SIZE) {
        memcpy(buffer, temp_buf, B_CHUNK_SIZE);
        fcbArray[fd].size2 = fcbArray[fd].size2 - 512;
        fcbArray[fd].index = fcbArray[fd].index + 1;
        return 512;
    } else {
        memcpy(buffer, temp_buf, fcbArray[fd].size2);
        fcbArray[fd].index = 0;
        return fcbArray[fd].size2;
    }
}

// Interface to Close the file	
void b_close(int fd) {
    close(fcbArray[fd].linuxFd);    // close the linux file handle
    free(fcbArray[fd].buf);    // free the associated buffer
    fcbArray[fd].buf = NULL;    // Safety First
    fcbArray[fd].linuxFd = -1;    // return this FCB to list of available FCB's
}
/**************************************************************
 *Class:  CSC-415
 *Name: Professor Bierman
 *Student ID: Chris Humphrey - 922256813 "ON THE DEADLOCK"
 *Project: Basic File System
 *
 *File: fsInit.c
 *
 *Description: Main driver for file system assignment.
 *
 *We initialize the VCB if not already done so, and call to make Root if not done so, and set CWD
 *
 **************************************************************/
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include "fsLow.h"
#include "mfs.h"

int initFileSystem(char *filename, uint64_t numberOfBlocks, uint64_t blockSize) {

    printf("Initializing File System with %ld blocks with a block size of %ld\n", numberOfBlocks, blockSize);

    //Setting up VCB
    struct VCB_Struct *VCBptr;
    VCBptr = malloc(blockSize);
    LBAread(VCBptr, 1, 0);
    if (VCBptr->Num_Blocks == numberOfBlocks) {
        printf("\nVCB Already Initialized Previously\n");
    } else {
        // VCB will occupy 1 block at block 0
        struct VCB_Struct VCB;
        VCB.Block_Size = blockSize;
        VCB.Num_Blocks = numberOfBlocks;
        int Total_Size = sizeof(VCB) + (sizeof(int) * VCB.Num_Blocks);

        struct VCB_Struct *VCBptr2;
        VCBptr2 = malloc(blockSize);
        VCBptr2->Block_Size = VCB.Block_Size;
        VCBptr2->Num_Blocks = VCB.Num_Blocks;

        int Bitmap_Size_Tmp = (numberOfBlocks / blockSize) + 1;
        int Bitmap_Location_Tmp = numberOfBlocks - Bitmap_Size_Tmp;

        int Bitmap[numberOfBlocks];
        for (int i = 0; i < VCB.Num_Blocks; i++) {
            Bitmap[i] = 0;
        }
        Bitmap[0] = 1;
        Bitmap[Bitmap_Location_Tmp] = 1;
        VCBptr2->Bitmap_Location = Bitmap_Location_Tmp;

        LBAwrite(Bitmap, Bitmap_Size_Tmp, Bitmap_Location_Tmp);
        LBAwrite(VCBptr2, 1, 0);

        printf("\n");
        printf("New VCB Initialized");
        printf(" \n ");
        printf("\n");

        free(VCBptr2);

        // Now We Create Root, which should auto be at block 1
        char *pathname = "/root";
        fs_mkdirROOT(pathname, 0777);
    }
    // By default let's cd into /root
    free(VCBptr);
    char *pathname = "root";
    fs_setcwd(pathname);

    printf("\nPlease watch this Youtube tutorial: ");
    printf("\nhttps://www.youtube.com/watch?v=7QcdAxCbxH4\n\n");


    return 0;
}

void exitFileSystem() {
    printf("System exiting\n");
}
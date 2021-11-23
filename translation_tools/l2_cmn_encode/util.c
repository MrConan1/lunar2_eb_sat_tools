/**********************************************************************/
/* util.c - Utility Functions                                         */
/**********************************************************************/
#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

/************/
/* Includes */
/************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"

/***********/
/* Defines */
/***********/
#define MAX_STORED_CHARACTERS 4096


/***********************/
/* Function Prototypes */
/***********************/
void swap16(void* pWd);
void swap32(void* pWd);
int numBytesInUtf8Char(unsigned char val);
int loadUTF8Table(char* fname);
int getUTF8character(int index, char* utf8Value);
int getUTF8code_Byte(char* utf8Value, unsigned char* utf8Code);
int getUTF8code_Short(char* utf8Value, unsigned short* utf8Code);

/* Global Array of UTF8 Data */
static char utf8Array[MAX_STORED_CHARACTERS][5];
static int numEntries = 0;



/****************************/
/* swap16                   */
/* Endian swap 16-bit data  */
/****************************/
void swap16(void* pWd){
    char* ptr;
    char temp;
    ptr = pWd;
    temp = ptr[1];
    ptr[1] = ptr[0];
    ptr[0] = temp;
    return;
}



/****************************/
/* swap32                   */
/* Endian swap 32-bit data  */
/****************************/
void swap32(void* pWd){
    char* ptr;
    char temp[4];
    ptr = pWd;
    temp[3] = ptr[0];
    temp[2] = ptr[1];
    temp[1] = ptr[2];
    temp[0] = ptr[3];
    memcpy(ptr, temp, 4);

    return;
}



/*****************************************************/
/* numBytesInUtf8Char                                */
/* Returns the number of bytes in the UTF8 character */
/*****************************************************/
int numBytesInUtf8Char(unsigned char val){
    if(val < 128){
        return 1;
    }
    else if(val < 224){
        return 2;
    }
    else if(val < 240){
        return 3;
    }
    else
        return 4;
}




/*******************************************************************/
/* loadUTF8Table                                                   */
/* Loads in a table file consisting of index values and UTF-8 Data */
/* Format for each line should be "index,utf-8_value"              */
/* Lines should end in line feed, tabs and spaces may be used for  */
/* separation.                                                     */
/* (No real safety checks if the input file is formatted wrong)    */
/*******************************************************************/
int loadUTF8Table(char* fname){

    static char linebuf[300];
    int index, numBytes, x;
    FILE* infile = NULL;
    char* ptr_line = NULL;
    memset(linebuf, 0, 300);

    /* Open the input file */
    infile = fopen(fname,"r");
    if(infile == NULL){
        printf("Error opening %s for reading.\n",fname);
        return -1;
    }

    /* Read until the end is detected */
    while(!feof(infile)){

        /* Get the index location */
        /* Skip lines that dont start with numbers */
        /* Assume they are comments, '#' is supposed to be a comment marker */
        fgets(linebuf, 299, infile);
        if (linebuf[0] == '#')
            continue;
        if (sscanf(linebuf, "%d", &index) != 1){
            fgets(linebuf, 299, infile);
            continue;
        }
        ptr_line = linebuf;

        /* Increment past numerical digits */
        while ((*ptr_line >= '0') && (*ptr_line <= '9'))
            ptr_line++;

        /* Index Bounds Check */
        if(index >= MAX_STORED_CHARACTERS){
            printf("Error, static array cannot support an index location of %d!\n",index);
            printf("Static array fixed to locations 0-%d\n",MAX_STORED_CHARACTERS-1);
            return -1;
        }

        /* Init the array for the index */
        memset(&(utf8Array[index][0]),0,5);

        /* Read until a comma is found */
        while (*ptr_line != ','){
            ptr_line++;
        }
        ptr_line++;

        /* Read until not a space or tab */
        while(1){
            if ((*ptr_line == ' ') || (*ptr_line == '\t')){
                ptr_line++;
                continue;
            }
            break;
        }

        /* Have the first byte of the utf8 character, get # bytes */
        numBytes = numBytesInUtf8Char((unsigned char)*ptr_line);
        utf8Array[index][0] = *ptr_line;

        /* Read the rest of the utf8 character */
        for(x=1; x < numBytes; x++){
            ptr_line++;
            utf8Array[index][x] = *ptr_line;
        }
        numEntries++;
    }

	/* Fixed Entries */
	utf8Array[32][0] = 0x20; //Space
	utf8Array[10][0] = 0x0A; //Enter
	numEntries++;
	numEntries++;

    fclose(infile);

    return 0;
}




/*******************************************************************/
/* getUTF8character                                                */
/* Copies in the UTF-8 Data for a given index location.  Assumes   */
/* that the location to be copied to has at least 5 bytes of space */
/* allocated to it.                                                */
/*******************************************************************/
int getUTF8character(int index, char* utf8Value){

    if(index >= MAX_STORED_CHARACTERS){
        printf("Error, static array index location of %d does not exist!\n",index);
        printf("Static array fixed to locations 0-%d\n",MAX_STORED_CHARACTERS-1);
        return -1;
    }
    memcpy(utf8Value,&utf8Array[index][0],5);
    return 0;
}


/*******************************************************************/
/* getUTF8code_Byte                                                */
/* Copies in the UTF-8 Code for a given UTF-8 character            */
/*******************************************************************/
int getUTF8code_Byte(char* utf8Value, unsigned char* utf8Code){

    int x,y,numBytes;
    numBytes = numBytesInUtf8Char((unsigned char)*utf8Value);

    for (x = 0; x < numEntries; x++){
        for (y = 0; y < numBytes; y++){
            if (utf8Array[x][y] != utf8Value[y])
                break;
            if (y == (numBytes - 1)){
                *utf8Code = (unsigned char)x;
                return 0;
            }
        }
    }

    return -1;
}


/*******************************************************************/
/* getUTF8code_Short                                               */
/* Copies in the UTF-8 Code for a given UTF-8 character            */
/*******************************************************************/
int getUTF8code_Short(char* utf8Value, unsigned short* utf8Code){

    int x, y, numBytes;
    numBytes = numBytesInUtf8Char((unsigned char)*utf8Value);

    for (x = 0; x < numEntries; x++){
        for (y = 0; y < numBytes; y++){
            if (utf8Array[x][y] != utf8Value[y])
                break;
            if (y == (numBytes - 1)){
                *utf8Code = (unsigned short)x;
                return 0;
            }
        }
    }

    return -1;
}

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
#include "script_node_types.h"
#include "util.h"

/***********/
/* Defines */
/***********/
#define MAX_STORED_CHARACTERS 4096


/***********************/
/* Function Prototypes */
/***********************/
void setSSSEncode();
int getSSSEncode();
void swap16(void* pWd);
void swap32(void* pWd);
int numBytesInUtf8Char(unsigned char val);
int loadUTF8Table(char* fname);
int getUTF8character(int index, char* utf8Value);
int getUTF8code_Byte(char* utf8Value, unsigned char* utf8Code);
int getUTF8code_Short(char* utf8Value, unsigned short* utf8Code);

void setBinOutputMode(int mode);
int getBinOutputMode();
void setBinMaxSize(unsigned int maxSize);
unsigned int getBinMaxSize();
void setMetaScriptInputMode(int mode);
int getMetaScriptInputMode();
void setTableOutputMode(int mode);
int getTableOutputMode();

/**************************/
/* Meta Script Read Fctns */
/**************************/
int read_ID(unsigned char* pInput);
int readLW(unsigned char* pInput, unsigned int* data);
int readSW(unsigned char* pInput, unsigned short* datas);
int readBYTE(unsigned char* pInput, unsigned char* datab);

/* Decode Related Functions */
int getTextDecodeMethod();
void setTextDecodeMethod(int method);
int checkSSSItemHack();

/* Global Array of UTF8 Data */
static char utf8Array[MAX_STORED_CHARACTERS][5];
static int enable_SSS_mode = 0;
static int numEntries = 0;

/* I/O Globals*/
//static int textDecodeMode = TEXT_DECODE_TWO_BYTES_PER_CHAR;  //Binary Input File Encoding for Text
//static int outputMode = LUNAR_BIG_ENDIAN;  //Script Text File Value Encoding
//static int inputMode = RADIX_HEX;    //Script Text File Value Representation
//static int tableMode = TWO_BYTE_ENC; //Table File Encoding Method
//static unsigned int maxBinFsize = 0; //MAX allowed size of a binary script file



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

        /* Hack for SSS */
        if ((enable_SSS_mode) && (index >= 769)){
            index++;
        }

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

    fclose(infile);

    /* Hack for SSS -- Fill in '.' at offset 769 */
    if (enable_SSS_mode){
        memset(&(utf8Array[769][0]), 0, 5);
        utf8Array[769][0] = '.';
    }

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

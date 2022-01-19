/******************************************************************************/
/* main.c - Main execution file for preliminary Lunar 2 Save/Load Text Decoder*/
/*          l2_cmntxt_decode.exe                                              */
/*          For use with Sega Saturn Lunar 2 File only.                       */
/******************************************************************************/

/***********************************************************************/
/* Lunar 2 Save/Load Text Decoder (l2_save_load_decode.exe) Usage      */
/* =========================================================           */
/* l2_save_load_decode.exe decode InputFname                           */
/*                                                                     */
/* InputFname should be File #2440                                     */
/*                                                                     */
/*                                                                     */
/* Note: Expects table file to be within same directory as exe.        */
/*       Table file should be named lunar2_font_table.txt              */
/***********************************************************************/


/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"


#define VER_MAJ    0
#define VER_MIN    2

#define MAX_BUF_SIZE (1024*1024)  /* 1MB, overkill, file is 96kB */



/******************************************************************************/
/* printUsage() - Display command line usage of this program.                 */
/******************************************************************************/
void printUsage(){
    printf("Error in input arguments.\n");
    printf("Usage:\n===============================\n");
    printf("l2_cmntxt_decode.exe OutputFname\n");
    printf("\n\n");
    return;
}


/******************************************************************************/
/* main()                                                                     */
/******************************************************************************/
int main(int argc, char** argv){

    FILE *inFile, *outFile;
	char* buffer;
    static char inFileName[300];
    static char outFileName[300];
	char data[32];
    int rval,numRead, x, y;
	unsigned int currentOffset, initialOffset, beginOffset, endOffset;

    printf("Lunar2 Save/Load Text Decoder v%d.%02d\n", VER_MAJ, VER_MIN);

    /**************************/
    /* Check input parameters */
    /**************************/

    /* Check for valid # of args */
    if (argc != 2){
        printUsage();
        return -1;
    }

    //Handle Generic Input Parameters
    memset(inFileName, 0, 300);
	memset(outFileName, 0, 300);
	strcpy(outFileName,argv[1]);
	strcat(outFileName,"_out.txt");

    
    /***************************************************/
    /* Load in the Table File for Decoding 2-Byte Text */
    /***************************************************/
	rval = loadUTF8Table("lunar2_font_table.txt");
    if (rval < 0){
        printf("Error loading UTF8 Table for Text Decoding.\n");
        return -1;
    }

    /*******************************/
    /* Open the input/output files */
    /*******************************/
    inFile = outFile = NULL;
    outFile = fopen(outFileName, "wb");
    if (outFile == NULL){
        printf("Error occurred while opening output file %s for writing\n", outFileName);
        fclose(inFile);
        return -1;
    }

	/* Read the whole thing into memory */
	buffer = (char*)malloc(MAX_BUF_SIZE);
	if(buffer == NULL){
		printf("Error allocating memory for file.\n");
		return -1;
	}


    /**************************************************************************/
    /* Parse the Input Script File (Req'd for Encoding to Binary or Updating) */
    /**************************************************************************/
    printf("Parsing input files.\n");
	
	for(y = 1268; y <= 1366; y++){
		char tmpbuf[50];

		memset(inFileName,0,300);
		memset(tmpbuf,0,50);
		itoa(y,tmpbuf,10);
		strcpy(inFileName,"input\\");
		strcat(inFileName,tmpbuf);

		inFile = fopen(inFileName, "rb");
		if (inFile == NULL){
			printf("Error occurred while opening input script %s for reading\n", inFileName);
			return -1;
		}
	
		/* Location names start at 0x2E0 and end by AC4 */
		#define CMN_STRING1_START 0x64 
		#define CMN_STRING1_END   0x78
		#define CMN_STRING1_SIZE  0xA
		printf("Decoding File %s\n", inFileName);
		fprintf(outFile,"%s:\n",inFileName);
	
		numRead = fread(buffer,1,0x5000,inFile);
		if(numRead <= 0){
			printf("Error in reading from the file occurred.\n");
			free(buffer);
			return -1;
		}

		/* Get beginning and end offset of this block */
		/* Only need both if further decoding needed  */
		beginOffset = *((unsigned int*)&buffer[4]);
		swap32(&beginOffset);
		endOffset = *((unsigned int*)&buffer[8]);
		swap32(&endOffset);

		initialOffset = currentOffset = beginOffset + 0x3C;
		while(currentOffset < endOffset){
			unsigned short* ptrString;
			unsigned short tmpShort;
		
	//		fprintf(outFile,"0x%06X %d\t",currentOffset,elementIndex++);
		
			for(x = 0; x < CMN_STRING1_SIZE;x++){
				ptrString = (unsigned short*)&buffer[currentOffset];
				tmpShort = *(unsigned short*)ptrString;
				swap16(&tmpShort);
		
				if(tmpShort == 0xFFFF){
					;//printf("<0xFFFF>");
	//				fprintf(outFile,"<0xFFFF>");
				}
				else if((tmpShort &0xF000) == 0x9000){
					//printf(" ");
					fprintf(outFile," ");
				}
				else if(tmpShort < 0x1000){
					memset(data,0,10);
					if(getUTF8character((int)tmpShort, data) >= 0){
						//printf("%s",data);
						fprintf(outFile,"%s",data);
					}
					else{
						printf("\nError printing 0x%X\n",tmpShort&0xFFFF);
						fprintf(outFile,"\nError printing 0x%X\n",tmpShort&0xFFFF);
					}
				}
				else{
					//printf("<0x%4X>",tmpShort);
					fprintf(outFile,"<0x%4X>",tmpShort);
				}
			
				currentOffset += 2;
			}
		
			/* Newline before next entry */
			initialOffset += 0x50;
			currentOffset = initialOffset;
			fprintf(outFile,"\n");
		}

	    fclose(inFile);
		fprintf(outFile,"\n\n");
	}
    
    /* Close files */
	fclose(outFile);

    return 0;
}

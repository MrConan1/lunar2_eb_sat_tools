/******************************************************************************/
/* main.c - Main execution file for preliminary Lunar 2 Common Text Decoder   */
/*          l2_vdp1txt_decode.exe                                             */
/*          For use with Sega Saturn Lunar 2 Common File only.                */
/******************************************************************************/

/***********************************************************************/
/* Lunar 2 Common Text Decoder (l2_cmntxt_decode.exe) Usage            */
/* =========================================================           */
/* l2_vdp1txt_decode.exe                                               */
/*                                                                     */
/* InputFname should be the 96kB common text file (File #1233).        */
/*                                                                     */
/* This exe does not decode the dialog "ES" file text                  */
/*                                                                     */
/* Note: Expects table file to be within same directory as exe.        */
/*       Table file should be named lunar2_font_table.txt              */
/***********************************************************************/

/*
Data Structure Notes

File 1227 has 4bpp images embedded within it
Common file 1233 contains a data structure with offsets and width/height 
First data structure is at 2785A0 in LORAM.  This is offset 5A0 in the file.
They likely continue until offset C7CC, the next section in the file.

0x16 bytes
==========
00      Palette?  Usually 0x01
01      Usually 0x00, unknown
02-03   (Offset / 8); 0x1400 usually added (1400*8 = 0xA000, offset in vdp1 ram of file data)
04      Width / 8
05      Height
06-07   Relative X position
08-09   Relative Y position
0A-0B   word
0C-0D   word
0E-0F   word  002F
10-11   word 
12-13   word
14-15   word


Calculation: (27C2A6-2785A0) / 0x16 = 0x2C6
Start of data structure of interest is 0x27C2A4


*/



/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"


#define VER_MAJ    0
#define VER_MIN    1

#define MAX_BUF_SIZE (1024*1024)  /* 1MB, overkill, file is 96kB */


#define BITS_PER_PIXEL           0x4
#define IMAGE_INFO_OFFSET        0x5A0
#define IMAGE_INFO_RECORD_SIZE   0x16
#define IMG_HDR_SIZE             8    //8 byte header size


typedef struct {
	unsigned char palette;
	unsigned char b0;
	unsigned short imgOffset; /* VDP1 Offset / 8 ; Also File offset */
	unsigned char  width;     /* Pixel Width / 8 */
	unsigned char  height;    /* Pixel Height */
	unsigned short rel_x_posit;
	unsigned short rel_y_posit;
	unsigned short wd0;
	unsigned short wd1;
	unsigned short wd2;
	unsigned short wd3;
	unsigned short wd4;
	unsigned short wd5;
}imageInfo;


/******************************************************************************/
/* printUsage() - Display command line usage of this program.                 */
/******************************************************************************/
void printUsage(){
    printf("Error in input arguments.\n");
    printf("Usage:\n===============================\n");
    printf("l2_vdp1txt_decode.exe\n");
    printf("\n\n");
    return;
}

imageInfo infoData[10000];
imageInfo infoDataSorted[10000];

/******************************************************************************/
/* main()                                                                     */
/******************************************************************************/
int main(int argc, char** argv){

    FILE* batchFile;
    FILE *inFile, *inImgFile, *outLog, *outFile;
	char* buffer,*buffer2;
    static char inFileName[300];
	static char inFileName2[300];
    static char outFileName[300];
	static char outLogName[300];
    int numRead, x, y, numData, currentIndex;
	unsigned int currentOffset;

    printf("Lunar2 VDP1 Text Decoder v%d.%02d\n", VER_MAJ, VER_MIN);

    /**************************/
    /* Check input parameters */
    /**************************/

    /* Check for valid # of args */
    if (argc != 1){
        printUsage();
        return -1;
    }

    //Handle Generic Input Parameters
    memset(inFileName, 0, 300);
	memset(inFileName2, 0, 300);
	memset(outFileName, 0, 300);
	memset(outLogName, 0, 300);
	strcpy(inFileName,"1233_cmn_file");
	strcpy(inFileName2,"1227");
	strcpy(outLogName,"log.txt");

    /*******************************/
    /* Open the input/output files */
    /*******************************/
    inFile = inImgFile = outLog = outFile = NULL;
    inFile = fopen(inFileName, "rb");
    if (inFile == NULL){
        printf("Error occurred while opening input common file %s for reading\n", inFileName);
        return -1;
    }
	inImgFile = fopen(inFileName2, "rb");
    if (inImgFile == NULL){
        printf("Error occurred while opening input vdp1 image file %s for reading\n", inFileName2);
        return -1;
    }
    outLog = fopen(outLogName, "wb");
    if (outLog == NULL){
        printf("Error occurred while opening output log file %s for writing\n", outLogName);
        return -1;
    }
	
	batchFile = fopen("batch.txt", "wb");
    if (batchFile == NULL){
        printf("Error occurred while opening output log file %s for writing\n", outLogName);
        return -1;
    }


    /************************/
    /* Parse the Input File */
    /************************/
    printf("Parsing input common file.\n");

	/* Read the whole thing into memory */
	buffer = (char*)malloc(MAX_BUF_SIZE);
	if(buffer == NULL){
		printf("Error allocating memory for file.\n");
		return -1;
	}
	buffer2 = (char*)malloc(MAX_BUF_SIZE);
	if(buffer2 == NULL){
		printf("Error allocating memory for file.\n");
		return -1;
	}
	numRead = fread(buffer,1,MAX_BUF_SIZE,inFile);
	if(numRead <= 0){
		printf("Error in reading from the file occurred.\n");
		free(buffer);
		return -1;
	}
	fclose(inFile);

	currentOffset = IMAGE_INFO_OFFSET;
	numData = 0;
	while(currentOffset < 0xBA44){
		int duplicateFound;
		imageInfo* pInfo;
		pInfo = (imageInfo*)&buffer[currentOffset];
		
		//swap16(&pInfo->width);
		//swap16(&pInfo->height);
		swap16(&pInfo->imgOffset);
		swap16(&pInfo->rel_x_posit);
		swap16(&pInfo->rel_y_posit);
		swap16(&pInfo->wd0);
		swap16(&pInfo->wd1);
		swap16(&pInfo->wd2);
		swap16(&pInfo->wd3);
		swap16(&pInfo->wd4);
		swap16(&pInfo->wd5);

		duplicateFound = 0;
		for(x = 0; x < numData; x++){
			if(infoData[x].imgOffset == pInfo->imgOffset){
				duplicateFound = 1;
				break;
			}
		}

		if(!duplicateFound){
			memcpy(&infoData[numData], pInfo, sizeof(imageInfo));
			numData++;
		}
		currentOffset += IMAGE_INFO_RECORD_SIZE;
	}

printf("Sorting\n");

	/* Sort the data in-order of offset */

	currentIndex = 0;
	for(y = 0; y < numData; y++){
		currentOffset = 0xFFFF;
		for(x = 0; x < numData; x++){

			if( (infoData[x].imgOffset != 0xFFFF) && (infoData[x].imgOffset <  currentOffset)){
				currentOffset = infoData[x].imgOffset;
				currentIndex = x;
			}
		}
		infoDataSorted[y] = infoData[currentIndex];
		infoData[currentIndex].imgOffset = 0xFFFF;
	}


//	for(x = 0; x < numData; x++){
//		printf("Img Offset = 0x%X\n",infoDataSorted[x].imgOffset);
//	}



printf("Second Pass\n");
	/* Second Pass */
	for(x = 0; x < numData; x++){
		int width, height, sizeBytes;
		unsigned int offset;

		width  = infoDataSorted[x].width * 8;
		height = infoDataSorted[x].height;
		offset = infoDataSorted[x].imgOffset;
		offset *= 8;
		sizeBytes = (width * height * BITS_PER_PIXEL) / 8;

		/* Print out the information */
		printf("Img %d\n",x);
		fprintf(outLog,"Img %d\n",x);
		fprintf(outLog,"Offset 0x%04X to 0x%04X (0x%08X)\n",offset, offset+sizeBytes-1, (offset+0xA000+0x25C00000));
		fprintf(outLog,"Width %d\n",width);
		fprintf(outLog,"Height %d\n",height);
		fprintf(outLog,"SizeBytes %d\n",sizeBytes);
		fprintf(outLog,"Palette 0x%02X\n",infoDataSorted[x].palette);
		fprintf(outLog,"Unknown Byte 0x%02X\n",infoDataSorted[x].b0);
		fprintf(outLog,"Rel_X 0x%04X Rel_Y 0x%04X\n",infoDataSorted[x].rel_x_posit,infoDataSorted[x].rel_y_posit);
		fprintf(outLog,"Wd0 0x%04X Wd1 0x%04X Wd2 0x%04X Wd3 0x%04X Wd4 0x%04X Wd5 0x%04X\n\n",
			infoDataSorted[x].wd0,infoDataSorted[x].wd1,infoDataSorted[x].wd2,
			infoDataSorted[x].wd3,infoDataSorted[x].wd4,infoDataSorted[x].wd5);

        /* Batch file */
        fprintf(batchFile,"%d.bin %d %d\n",x,width,height);

		/* Create the image */
		memset(outFileName,0,300);
		sprintf(outFileName,"output\\%d.bin",x);
		outFile = fopen(outFileName, "wb");
		if (outFile == NULL){
			printf("Error occurred while opening output file %s for writing\n", outFileName);
			return -1;
		}

		fseek(inImgFile,offset+IMG_HDR_SIZE,SEEK_SET);
		numRead = fread(buffer2,1,sizeBytes,inImgFile);
		if(numRead <= 0){
			printf("Error in reading from file %d occurred.\n",x);
		}
		fwrite(buffer2,1,sizeBytes,outFile);
		fclose(outFile);
	}

	free(buffer);
	free(buffer2);
    
    /* Close files */
    fclose(inImgFile);
	fclose(outLog);
    fclose(batchFile);

    return 0;
}

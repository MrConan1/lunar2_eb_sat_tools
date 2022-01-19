/******************************************************************************/
/* main.c - Main execution file for Lunar Save/Load Text Encoder              */
/******************************************************************************/

/***********************************************************************/
/* Lunar Save/Load Text Encoder Usage                                  */
/* ========================================                            */
/* l2_saveld_encode.exe inputDir outputDir                             */
/* Expects for the following files in the immediate executable path:   */
/*                                                                     */
/*     Input Game Files                                                */
/*     =================                                               */
/*     1268 to 1366 - Original game files containing save locations    */
/*                                                                     */
/*     Translation Files                                               */
/*     =================                                               */
/*     saves_translated.txt - English Text for save locations.         */
/*                                                                     */
/*     Encoding Files                                                  */
/*     =================                                               */
/*     bpe.table - Byte Pair Encoding table to be used for encoding.   */
/***********************************************************************/
#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "bpe_compression.h"



/***********/
/* Defines */
/***********/

/* Offsets */
#define SAVELD_NAME_OFFSET        0x1C

/* Max # 8-BIT Characters */
#define MAX_CHARACTERS            20

/* Space in the file in bytes to store the save location */
#define SAVE_SIZE              (NUM_SAVES * MAX_CHARACTERS)


/* Globals */
FILE* translatedFile = NULL;

/* Function Prototypes */
int encodeSaveNames(char* inFileName, char* outFileName);


/******************************************************************************/
/* printUsage() - Display command line usage of this program.                 */
/******************************************************************************/
void printUsage(){
    printf("Error in input arguments.\n");
    printf("Usage:\n=========\n");
	printf("l2_saveld_encode.exe input_path output_path\n\n");
    return;
}


/******************************************************************************/
/* main()                                                                     */
/******************************************************************************/
int main(int argc, char** argv){

	int x;
	static char inDir[300];
    static char outDir[300];
	static char transFname[300];
	static char inFileName[300];
	static char outFileName[300];

    /**************************/
    /* Check input parameters */
    /**************************/

	/* So it doesnt crash */
	if (argc < 3){
		printUsage();
		return -1;
	}

	/* Copy the directory names */
	strcpy(inDir,argv[1]);
	strcpy(outDir,argv[2]);
	if(inDir[strlen(inDir)-1] == '/')
		inDir[strlen(inDir)-1] = '\\';
	if(outDir[strlen(inDir)-1] == '/')
		outDir[strlen(inDir)-1] = '\\';
	if(inDir[strlen(inDir)-1] != '\\')
		strcat(inDir,"\\");
	if(outDir[strlen(inDir)-1] != '\\')
		strcat(outDir,"\\");
    
    /***********************************************/
    /* Finished Checking the Input File Parameters */
    /***********************************************/



    /********************/
    /* Perform Encoding */
    /********************/
	
	// Load BPE Table
	if (loadBPETable("bpe.table","l2_8bit_table.txt") < 0){
        printf("Error loading BPE Tables for Text Encoding/Decoding.\n");
        return -1;
    }

	// Open the translated name file
	memset(transFname,0,300);
	//strcpy(transFname,inDir);
	strcpy(transFname,"saves_translated.txt");
	translatedFile = fopen(transFname,"rb");
	if(translatedFile == NULL){
		printf("Error occurred while opening %s for reading\n", transFname);
		return -1;
	}

    printf("\n\nEncoding...\n\n");
	

	/*****************************/
	/* Encode the Save Locations */
	/*****************************/
	for(x = 1268; x <= 1366; x++){

		char tmpbuf[50];

		/* Set up input and output filenames*/
		itoa(x,tmpbuf,10);
		memset(inFileName,  0, 300);
		memset(outFileName, 0, 300);
		strcpy(inFileName, inDir);
		strcat(inFileName, tmpbuf);
		strcpy(outFileName, outDir);
		strcat(outFileName, tmpbuf);

		if(encodeSaveNames(inFileName, outFileName) < 0){
			printf("An error occurred while trying to encode save locations, File = %s.\n",inFileName);
			return -1;
		}
	}

	fclose(translatedFile);

    return 0;
}




/* Reads from input file and a translated save name file */
/* Updates the input file save locations to english using BPE encoding */
int encodeSaveNames(char* inFileName, char* outFileName){

	FILE* infile, *outFile;
	unsigned char* buffer;
	static char line[1024];
	int outputSizeBytes;
	int str1Len, numRead;
	unsigned int currentOffset, initialOffset, beginOffset, endOffset;

	/* Open Input file and get file size */
	infile = NULL;
    infile = fopen(inFileName,"rb");
    if(infile == NULL){
		printf("Error occurred while opening %s for reading\n", inFileName);
        return -1;
    }
	fseek(infile,0,SEEK_END);
	outputSizeBytes = ftell(infile);
	fseek(infile,0,SEEK_SET);

	/* Create temp output buffer & read input file into memory */
	buffer = (unsigned char*)malloc(outputSizeBytes);
	if(buffer == NULL){
		printf("Memory alloc error for temp buffer.\n");
		return -1;
	}
	numRead = fread(buffer,1,outputSizeBytes,infile);
	if(numRead != outputSizeBytes){
		printf("Error in reading from the file occurred.\n");
		free(buffer);
		return -1;
	}
	fclose(infile);

	/* Read past the input header on the translation file */
	while(1){
		memset(line,0,1023);
		fgets(line,1023,translatedFile);
		if(strncmp(line,"input",5)==0)
			break;
	}

	/* Get beginning and end offset of this block */
	/* Only need both if further decoding needed  */
	beginOffset = *((unsigned int*)&buffer[4]);
	swap32(&beginOffset);
	endOffset = *((unsigned int*)&buffer[8]);
	swap32(&endOffset);
	initialOffset = currentOffset = beginOffset + 0x3C;

    /* Overwrite each save name entry in the file */
	while(currentOffset < endOffset){
		
		int z;
		char* pSaveNameEntry;
		pSaveNameEntry = (char*)&buffer[currentOffset];
		
		/* Read in the next translated entry */
		memset(line,0,1023);
	    fgets(line,1023,translatedFile);
		for(z = 0 ; z < strlen(line); z++){
			if( (line[z] == '\r') || 
			    (line[z] == '\n') ) {
				line[z] = '\0';
				break;
			}
		}
		str1Len = strlen(line);
		if(str1Len <= 0){
			printf("Error in translated text file, missing entry.\n");
			free(buffer);
			return -1;
		}
		
//		printf("%s\n",line);

		//BPE Compression
		if(str1Len > 0){
			int z;
			compressBPE((unsigned char*)line,(unsigned int*) &str1Len);
			if(str1Len >= MAX_CHARACTERS){
				line[MAX_CHARACTERS-1] = 0xFF;
			}
			else{
				for(z = str1Len; z < MAX_CHARACTERS; z++){
					line[z] = 0xFF;
				}
			}
		}
		else{
			memset(line,0xFF, MAX_CHARACTERS);
		}
		str1Len = MAX_CHARACTERS;

#if 0
		{
			static unsigned char decmprStr[300];
			unsigned int size;
			memset(decmprStr,0,300);
			decompressBPE(decmprStr,(unsigned char*)line,&size);
			printf("Decompr tmpStr: %s\n",decmprStr);
		}
#endif

		/* Overwrite the Entry */
		memcpy(pSaveNameEntry,line,MAX_CHARACTERS);

		/* Newline before next entry */
		initialOffset += 0x50;
		currentOffset = initialOffset;
	}


	/****************************/
	/* Write to the output file */
	/****************************/

	/* Open the output file */
	outFile = fopen(outFileName,"wb");
	if(outFile == NULL){
		printf("Error occurred while opening %s for writing\n", outFileName);
		return -1;
	}

	/* Write the binary version of the items to the output file */
	if(fwrite(buffer,1,outputSizeBytes,outFile) != outputSizeBytes){
		printf("Error writing save locations to output file.\n");
		fclose(outFile);
		return -1;
	}

	/* Close output file */
	fclose(outFile);

	free(buffer);

	return 0;
}


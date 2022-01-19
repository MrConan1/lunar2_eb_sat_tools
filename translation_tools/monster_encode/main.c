/******************************************************************************/
/* main.c - Main execution file for Lunar Monster Text Encoder                */
/******************************************************************************/

/***********************************************************************/
/* Lunar Monster Text Encoder Usage                                    */
/* ========================================                            */
/* l2_mnst_encode.exe inputDir outputDir                               */
/*                                                                     */
/* Input path has the original extracted game files, output path is    */
/* the destination for reencoded files.                                */
/* Expects the following files are the immediate executable path:      */
/*                                                                     */
/*     Translation Files                                               */
/*     =================                                               */
/*     monster_names.txt - Formatted Text File with replacement        */
/*                         monster names.                              */
/*                                                                     */
/*     Encoding Files                                                  */
/*     =================                                               */
/*     bpe.table - Byte Pair Encoding table to be used for encoding.   */
/*     l2_8bit_table.txt - 8-bit mappings to be used.                  */
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
#define MONSTER_DATA_OFFSET       0x4

/* Max # 8-BIT Characters for Re-Encoding */
#define MAX_CHARACTERS            24


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
	printf("l2_mnst_encode.exe input_path output_path\n\n");
    return;
}

	static char line[1024];
	static char tmpbuf[1024];

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
	int term = 0;

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
	strcpy(transFname,"monster_names.txt");
	translatedFile = fopen(transFname,"rb");
	if(translatedFile == NULL){
		printf("Error occurred while opening %s for reading\n", transFname);
		return -1;
	}

    printf("\n\nEncoding...\n\n");
	

	/*****************************/
	/* Encode the Save Locations */
	/*****************************/
	while(!feof(translatedFile)){

		/* Look for the next input file */
		while(1){
			if(feof(translatedFile)){
				term = 1;
				break;
			}
			memset(line,0,1023);
		    fgets(line,1023,translatedFile);
			if(strstr(line,"File:") != NULL){
				sscanf(line, "%s %d", tmpbuf,&x);
				break;
			}
		}
		if(term)
			break;

		/* Set up input and output filenames*/
		itoa(x,tmpbuf,10);
		memset(inFileName,  0, 300);
		memset(outFileName, 0, 300);
		strcpy(inFileName, inDir);
		strcat(inFileName, tmpbuf);
		strcpy(outFileName, outDir);
		strcat(outFileName, tmpbuf);
		printf("Encoding Monster Names in File# %d\n",x);

		if(encodeSaveNames(inFileName, outFileName) < 0){
			printf("An error occurred while trying to encode save locations, File = %s.\n",inFileName);
			return -1;
		}
	}

	fclose(translatedFile);
	printf("Completed.\n");

    return 0;
}




/* Reads from input file and a translated save name file */
/* Updates the input file save locations to english using BPE encoding */
int encodeSaveNames(char* inFileName, char* outFileName){

	FILE* infile, *outFile;
	unsigned char* buffer;
	static char line[1024];
	int outputSizeBytes, x;
	int str1Len, numRead, textSizeBytes, numMonsterNames;
	unsigned int currentOffset, beginOffset;
	char* pStr;
	int startLocation;

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
		if(strstr(line,"Monsters") != NULL){
			break;
		}
		else if(feof(translatedFile)){
			printf("Error, premature end of input file while reading headers.\n");
			return -1;
		}
	}

	/* Get beginning and end offset of this block */
	/* Only need both if further decoding needed  */
	beginOffset = *((unsigned int*)&buffer[MONSTER_DATA_OFFSET]);
	swap32(&beginOffset);
	beginOffset += 8;    /* Add 8 since its a relative offset */

	/* Read in the # of bytes for monster names */
	textSizeBytes = *((unsigned int*)&buffer[beginOffset]);
	swap32(&textSizeBytes);
	printf("Size (Bytes): %d\n",textSizeBytes);
		
	/* Calculate # of monsters */
	numMonsterNames = textSizeBytes / MAX_CHARACTERS;
	printf("Size (Bytes): %d [%d Monsters]\n",textSizeBytes, numMonsterNames);

    /* Overwrite each monster name entry in the file */
	currentOffset = beginOffset + 4;
	for(x = 0; x < numMonsterNames; x++){
		
		int z;
		char* pSaveNameEntry;
		pSaveNameEntry = (char*)&buffer[currentOffset];
		
		/* Read in the next translated entry */
		while(1){
			memset(line,0,1023);
		    fgets(line,1023,translatedFile);
			if(strstr(line,"Monster") != NULL){
				//Sanity check
				break;
			}
			else if(feof(translatedFile)){
				printf("Error, premature end of input file while reading monster names.\n");
				return -1;
			}
		}
		
		/* Extract the string */
		startLocation = 0;
		for(z = 0 ; z < (int)strlen(line); z++){

			if(!startLocation){
				if(line[z] == '\"'){
					pStr = &line[z];
					startLocation = 1;
				}
			}
			else{
				if(line[z] == '\"'){
					line[z] = '\0';
					break;
				}
			}

			/* Error */
			if(line[z] == '\0'){
				printf("Error, premature end of input, no quote detected.\n");
				return -1;
			}
		}
		pStr++;
		str1Len = strlen(pStr);
		if(str1Len <= 0){
			printf("Error in translated text file, missing entry.\n");
			free(buffer);
			return -1;
		}
		
//		printf("%s\n",pStr);

		//BPE Compression
		if(str1Len > 0){
			int z;
			compressBPE((unsigned char*)pStr,(unsigned int*) &str1Len);
			if(str1Len >= MAX_CHARACTERS){
				pStr[MAX_CHARACTERS-1] = 0xFF;
			}
			else{
				for(z = str1Len; z < MAX_CHARACTERS; z++){
					pStr[z] = 0xFF;
				}
			}
		}
		else{
			memset(pStr,0xFF, MAX_CHARACTERS);
		}
		str1Len = MAX_CHARACTERS;

#if 0
		{
			static unsigned char decmprStr[300];
			unsigned int size;
			memset(decmprStr,0,300);
			decompressBPE(decmprStr,(unsigned char*)pStr,&size);
			printf("Decompr tmpStr: %s\n",decmprStr);
		}
#endif

		/* Overwrite the Entry */
		memcpy(pSaveNameEntry,pStr,MAX_CHARACTERS);

		/* Newline before next entry */
		currentOffset += MAX_CHARACTERS;
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


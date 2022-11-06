/******************************************************************************/
/* main.c - Main execution file for Lunar 2 Action Script Encoder             */
/*                                                                            */
/*          Encodes action related portion of Saturn LEB scripts.             */
/******************************************************************************/

/***********************************************************************/
/* Usage                                                               */
/* ==================================================                  */
/* l2_actionEnc.exe origSaturnFile updateFile outputFile               */
/*                                                                     */
/* origSaturnFile should be a packed script file corresponding to the  */
/*                original sega saturn file to be updated.             */
/* updateFile contains a text readable version of the updated action   */
/*            script.  Format should adhere to the one used by the     */
/*            extraction version of this program.                      */
/* outputFile is the filename for the updated binary packet script file*/
/*                                                                     */
/* Note:  The tools have been set up such that the action script       */
/* should be updated prior to the dialog in the packed scripts.        */
/***********************************************************************/
#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"


#define VER_MAJ    1
#define VER_MIN    1





/* Packed File with ES Script embedded within it */
typedef struct{
    unsigned int zeroPadOffset;     /* In Saturn, offset to zero padding */
                                    /* to 2048 byte alignment at EOF     */
								    /* Need to modify if ES Section altered.  */
								    /* PSX holds something different.         */
    unsigned int numActionScripts;  /* # of Action Scripts in this file       */
	unsigned int asOffset;          /* Action Script Offset */
	unsigned int esHdrOffset;       /* Offset to ES Header */
	unsigned int postActionOffset;  /* Points to the offset for the setion */
                                    /* of the file that occurs immediately */
									/* after the Action script, but before */
									/* the ES Header section.              */
	unsigned int updateOffset4;     /* Points to some section offset AFTER ES Header */
}packedHeaderType;


typedef struct{
    unsigned int actionScriptID;    /* Unique ID corresponding to this action script */
    unsigned int actionScriptOffset;/* Offset in bytes measured from end of first header */
}actionScriptHdrType;

typedef struct actionScriptMetaType actionScriptMetaType;
typedef struct actionScriptCmdType actionScriptCmdType;

struct actionScriptCmdType{
    unsigned short scriptCmd;        /* Script Command */
    unsigned int numParameters;      /* # of parameters associated with this script command */
	unsigned short pParameters[30];  /* Array of ushort parameter data, max 30, wont hit that */
};

/* Statically sized the below to rush this exe out */
/* shouldnt hit any limitations, will just hog memory */
struct actionScriptMetaType{
    unsigned int actionScriptID;     /* Unique ID corresponding to this action script */
    unsigned int preambleSizeBytes;  /* Size of preamble data in bytes */
	unsigned int preData[100];       /* Array of preamble data, max 100 will not hit */
	unsigned int numCmds[24];        /* # of commands associated with this entry */
    actionScriptCmdType entryData[24][10000];  /* Pointers to Commands, max 1k, will not hit */
};





/***********************/
/* Function Prototypes */
/***********************/
void printUsage();
unsigned int createActionScript(int numSections, actionScriptMetaType* pMeta, char* buffer);



/******************************************************************************/
/* printUsage() - Display command line usage of this program.                 */
/******************************************************************************/
void printUsage(){
    printf("Error in input arguments.\n\n");
    printf("Usage\n===============================\n");
    printf("l2_actionEnc.exe origSaturnFile updateFile outputFile\n");
    return;
}


/* For parsing update data */
#define LINE_BUF_BYTES  2048
char line[LINE_BUF_BYTES];

#define DELIM " "





/******************************************************************************/
/* main()                                                                     */
/******************************************************************************/
int main(int argc, char** argv){

    FILE *inFile, *updateFile, *outFile;
	static char inFileName[300];
	static char updateFileName[300];
    static char outFileName[300];
	static char targetStr[100];
	static char targetStr2[100];
    int x, y, outputFsize, i, targetLen, targetLen2, lineNum, dataFoundFlg;
	int offsetDelta, copySize;
	unsigned int preambleSize, numSections, numwritten, actionScriptSizeBytes, zeroBytes;
	char* outputBuf, *tmpActnScriptBuf;
	char* pStr;
	unsigned int* pUint32Data;
	packedHeaderType oldPackedHdr, *pNewPackedHdr;
	actionScriptMetaType* ascriptMeta = NULL;

    printf("Lunar2 Action Script Encoder v%d.%02d\n", VER_MAJ, VER_MIN);


    /**************************/
    /* Check input parameters */
    /**************************/

    /* Check for valid # of args */
    if (argc != 4){
        printUsage();
        return -1;
    }

    //Handle Input Parameters
    memset(inFileName, 0, 300);
	memset(updateFileName, 0, 300);
	memset(outFileName, 0, 300);
	strcpy(inFileName,argv[1]);
	strcpy(updateFileName,argv[2]);
	strcpy(outFileName,argv[3]);


    /*************************************************/
    /* Open the update file to read in the meta data */
    /*************************************************/
    inFile = updateFile = outFile = NULL;
	updateFile = fopen(updateFileName, "rb");
    if (updateFile == NULL){
        printf("Error occurred while opening input script %s for reading\n", inFileName);
        return -1;
    }


    /************************************/
	/* Read # of sections and           */
	/* Allocate memory for each section */
	/* Ex - "NumSections: 2"            */
	/************************************/
	lineNum = 0;
	strcpy(targetStr,"NumSections: ");
	targetLen = strlen(targetStr);
	while(1){
		memset(line,0,LINE_BUF_BYTES);
	    fgets(line,LINE_BUF_BYTES-1,updateFile); lineNum++;
		if( strncmp(line,targetStr,targetLen) == 0)
			break;
		if(feof(updateFile)){
			printf("Error unexpected EOF reached - # Sections, line = %d\n",lineNum);
			fclose(updateFile);
			return -1;
		}
	}
	/* Read the # of sections */
	sscanf((char*)(line+targetLen), "%x", &numSections);

	/* Allocate Data Structure to hold meta data from input file */
	ascriptMeta = (actionScriptMetaType*)malloc(sizeof(actionScriptMetaType)* numSections);
	if(ascriptMeta == NULL){
		printf("Error allocing memory for metadata\n");
		fclose(updateFile);
		return -1;
	}
	memset(ascriptMeta, 0, sizeof(actionScriptMetaType)* numSections);

	/**********************************************************/
	/* For each section, construct an action script in memory */
	/**********************************************************/
	for(x = 0; x < (int)numSections; x++){
		
		/* Read in the unique script ID, data in hex */
		/* Ex - "Action Script ID: 0x00" */
		strcpy(targetStr,"Action Script ID: ");
		targetLen = strlen(targetStr);
		while(1){
			memset(line,0,LINE_BUF_BYTES);
			fgets(line,LINE_BUF_BYTES-1,updateFile); lineNum++;
			if( strncmp(line,targetStr,targetLen) == 0)
				break;
			if(feof(updateFile)){
				printf("Error unexpected EOF reached - script id, line = %d\n",lineNum);
				free(ascriptMeta);
				fclose(updateFile);
				return -1;
			}
		}
		/* Read the unique script id */
		sscanf((char*)(line+targetLen), "%x", &ascriptMeta[x].actionScriptID);


		/* Read in the preamble size in bytes, data in hex */
		/* Ex - "Preamble Size: 1234" */
		strcpy(targetStr,"Preamble Size: ");
		targetLen = strlen(targetStr);
		while(1){
			memset(line,0,LINE_BUF_BYTES);
			fgets(line,LINE_BUF_BYTES-1,updateFile); lineNum++;
			if( strncmp(line,targetStr,targetLen) == 0)
				break;
			if(feof(updateFile)){
				printf("Error unexpected EOF reached - preamble size, line = %d\n",lineNum);
				free(ascriptMeta);
				fclose(updateFile);
				return -1;
			}
		}
		/* Read the preamble size in bytes */
		sscanf((char*)(line+targetLen), "%x", &preambleSize);
		ascriptMeta[x].preambleSizeBytes = preambleSize;
		if(preambleSize > 100){
			printf("Error, array bounds of 100 bytes on preamble data exceeded, got size = %d bytes. Line = %d\n",preambleSize,lineNum);
			free(ascriptMeta);
			fclose(updateFile);
			return -1;
		}

		/* Read in the preamble data line */
		/* Ex - "Preamble Data: 1234" */
		strcpy(targetStr,"Preamble Data: ");
		targetLen = strlen(targetStr);
		while(1){
			memset(line,0,LINE_BUF_BYTES);
			fgets(line,LINE_BUF_BYTES-1,updateFile); lineNum++;
			if( strncmp(line,targetStr,targetLen) == 0)
				break;
			if(feof(updateFile)){
				printf("Error unexpected EOF reached - preamble data, line = %d\n",lineNum);
				free(ascriptMeta);
				fclose(updateFile);
				return -1;
			}
		}

		/* Read in the preamble data, data in hex */
		/* Ex - "Preamble Data: 00 00 01"         */
		pStr = line;
		pStr = (char*)strtok((char *)line, DELIM);
		pStr = (char*)strtok(NULL, DELIM);
		for(y = 0; y < (int)preambleSize; y++){
			pStr = (char*)strtok(NULL, DELIM);
			if(pStr == NULL){
				printf("Error unexpected end of preamble data, line = %d\n",lineNum);
				free(ascriptMeta);
				fclose(updateFile);
				return -1;
			}
			else{
				sscanf(pStr, "%x", &ascriptMeta[x].preData[y]);
			}
		}


		/**********************************************/
		/* Read in all 24 Entries (always 24 entries) */
		/* along with the associated cmd data.        */
		/**********************************************/
		for(y = 0; y < 24; y++){
			ascriptMeta[x].numCmds[y] = 0;

			/* Look for "Entry" */
			strcpy(targetStr,"Entry ");
			targetLen = strlen(targetStr);
			while(1){
				memset(line,0,LINE_BUF_BYTES);
				fgets(line,LINE_BUF_BYTES-1,updateFile); lineNum++;
				if( strncmp(line,targetStr,targetLen) == 0)
					break;
				if(feof(updateFile)){
					printf("Error unexpected EOF reached - entry %d search, line = %d\n",y, lineNum);
					fclose(updateFile);
					return -1;
				}
			}

			/* Look for "Script_DATA" to find the start of script data */
			/* If "NO_DATA" is found instead, there is no data for this entry */
			dataFoundFlg = 0;
			strcpy(targetStr,"Script_DATA");
			strcpy(targetStr2,"NO_DATA");
			targetLen = strlen(targetStr);
			targetLen2 = strlen(targetStr2);
			while(1){
				memset(line,0,LINE_BUF_BYTES);
				fgets(line,LINE_BUF_BYTES-1,updateFile); lineNum++;
				if(feof(updateFile)){
					printf("Error unexpected EOF reached - Script_DATA start, line = %d\n",lineNum);
					free(ascriptMeta);
					fclose(updateFile);
					return -1;
				}
				if( strncmp(line,targetStr,targetLen) == 0){
					dataFoundFlg = 1;
					break;
				}
				if( strncmp(line,targetStr2,targetLen2) == 0)
					break;
			}


			/* Look for "End_Of_DATA" to exit */
			/* Otherwise should find 0xNUMBER Cmd for a command */
			/* Followed by a line that looks like Args: #args arg1 arg2 ... argN */
			/* Note 0xFFFF is just copied verbatim, no arg line */
			/* Example Entry -                         */
            /* 0x0007   Cmd: Video Effect              */
            /*          Args: 3 0x0000 0x0000 0x005A   */ 
			if(dataFoundFlg){

				while(1){

					/* Exit Condition Check */
					strcpy(targetStr,"End_Of_DATA");
					targetLen = strlen(targetStr);
					memset(line,0,LINE_BUF_BYTES);
					fgets(line,LINE_BUF_BYTES-1,updateFile); lineNum++;
					if(feof(updateFile)){
						printf("Error unexpected EOF reached - Cmd or End_Of_DATA check, line = %d, Entry = %d\n",lineNum, y);
						free(ascriptMeta);
						fclose(updateFile);
						return -1;
					}
					if( strncmp(line,targetStr,targetLen) == 0){
						break;
					}

					/* Command Check */
					if((line[0] == '0') && (line[1] == 'x')){

						/* Read Command */
						sscanf((char*)(line), "%hx", &ascriptMeta[x].entryData[y][ascriptMeta[x].numCmds[y]].scriptCmd);
						ascriptMeta[x].numCmds[y]++;
						ascriptMeta[x].entryData[y][ascriptMeta[x].numCmds[y]-1].numParameters = 0;
						
						/* Next line is arguments (if Cmd != 0xFFFF) */
						if(ascriptMeta[x].entryData[y][ascriptMeta[x].numCmds[y]-1].scriptCmd != 0xFFFF){
							fgets(line,LINE_BUF_BYTES-1,updateFile); lineNum++;
							if(feof(updateFile)){
								printf("Error unexpected EOF reached - Argument Reading, line = %d\n",lineNum);
								free(ascriptMeta);
								fclose(updateFile);
								return -1;
							}
							sscanf((char*)(line), "%s %d", targetStr, &ascriptMeta[x].entryData[y][ascriptMeta[x].numCmds[y]-1].numParameters);

							/* Tokenize to read in arguments */
							pStr = line;
							pStr = (char*)strtok((char *)line, DELIM);
							pStr = (char*)strtok(NULL, DELIM);
							for(i = 0; i < (int)ascriptMeta[x].entryData[y][ascriptMeta[x].numCmds[y]-1].numParameters; i++){
								pStr = (char*)strtok(NULL, DELIM);
								if(pStr == NULL){
									printf("Error fewer arguments than expected detected, line = %d\n",lineNum);
									free(ascriptMeta);
									fclose(updateFile);
									return -1;
								}
								sscanf(pStr, "%hx", &ascriptMeta[x].entryData[y][ascriptMeta[x].numCmds[y]-1].pParameters[i]);
							}
						}
					}
					else if((line[0] == '#') || (line[0] == ';')){
						; /* Comment Line, do nothing */
					}
					else{
						; /* Ignore */
					}
				}
			}

			/* End of y = 0 to 23 entries */
		}

		/* End of Loop for this Section */
	}
	fclose(updateFile);


	/*****************************************************************************************/
	/* Now put the action script data in memory into the format the game will be looking for */
	/*****************************************************************************************/
	tmpActnScriptBuf  = (char*)malloc(1024*1024);
	if(tmpActnScriptBuf == NULL){
		printf("Error allocating memory to create new script.\n");
		free(ascriptMeta);
		return -1;
	}
	memset(tmpActnScriptBuf,0,1024*1024);
	actionScriptSizeBytes = createActionScript(numSections, ascriptMeta, tmpActnScriptBuf);
	free(ascriptMeta); /* release resources */

	/*******************************************************************/
	/* And now update the original script to use the new action script */
	/* Remember to update offsets at the beginning of the file, and    */
	/* also remember to adjust final padding correctly.                */
	/*******************************************************************/
	outputFsize = 0;
	inFile = fopen(inFileName, "rb");
    if (inFile == NULL){
        printf("Error occurred while opening input script %s for reading\n", inFileName);
		free(tmpActnScriptBuf);
        return -1;
    }
	outputBuf = (char*)malloc(1024*1024);
	if(outputBuf == NULL){
		printf("Error allocating memory to create new script.\n");
		free(tmpActnScriptBuf);
		return -1;
	}
	memset(outputBuf,0,1024*1024);

	/* Read in the old packed header */
	if(fread(&oldPackedHdr, 1, sizeof(packedHeaderType), inFile) != sizeof(packedHeaderType)){
		printf("Error copying packed header data from input binary file.\n");
		free(tmpActnScriptBuf);
		free(outputBuf);
		return -1;
	}
	pUint32Data = (unsigned int*)&oldPackedHdr;
	for(x = 0; x < sizeof(packedHeaderType)/4; x++)
		swap32(&pUint32Data[x]);

	/* Write the new header, accounting for offset differences */
	/* # Action Scripts and Offset to the start of them does not change */
	pNewPackedHdr = (packedHeaderType*)outputBuf;
	pNewPackedHdr->numActionScripts = oldPackedHdr.numActionScripts;
	pNewPackedHdr->asOffset = oldPackedHdr.asOffset;
	pNewPackedHdr->postActionOffset = pNewPackedHdr->asOffset + actionScriptSizeBytes;
	offsetDelta = (int)(pNewPackedHdr->postActionOffset) - (int)oldPackedHdr.postActionOffset;
	pNewPackedHdr->esHdrOffset = oldPackedHdr.esHdrOffset + offsetDelta;
	pNewPackedHdr->updateOffset4 = oldPackedHdr.updateOffset4 + offsetDelta;
	pNewPackedHdr->zeroPadOffset = oldPackedHdr.zeroPadOffset + offsetDelta;

	/* Copy the data (new action script + all old data) */
	memcpy(&outputBuf[pNewPackedHdr->asOffset], tmpActnScriptBuf, actionScriptSizeBytes);
	fseek(inFile,oldPackedHdr.postActionOffset,SEEK_SET);
	copySize = oldPackedHdr.zeroPadOffset - oldPackedHdr.postActionOffset;
	if( fread(&outputBuf[pNewPackedHdr->postActionOffset],1,copySize,inFile) != copySize){
		printf("Error copying old data from input binary file.\n");
		free(tmpActnScriptBuf);
		free(outputBuf);
		return -1;
	}
	/* Done with the input file */
	free(tmpActnScriptBuf);
	fclose(inFile);

	/* Adjust the zero padding */
	outputFsize = pNewPackedHdr->zeroPadOffset;
	if((outputFsize % 2048) != 0){
 	    zeroBytes = 2048 - (outputFsize % 2048);
		memset(&outputBuf[pNewPackedHdr->zeroPadOffset], 0, zeroBytes);
		outputFsize += zeroBytes;
	}

	/* Swap the new Packed Header */
	pUint32Data = (unsigned int*)pNewPackedHdr;
	for(x = 0; x < sizeof(packedHeaderType)/4; x++)
		swap32(&pUint32Data[x]);

	/* Open and Write the data to the output file */
    outFile = fopen(outFileName, "wb");
    if (outFile == NULL){
        printf("Error occurred while opening output file %s for writing\n", outFileName);
		free(outputBuf);
        return -1;
    }
	numwritten = fwrite(outputBuf,1,outputFsize,outFile);
	if(numwritten != outputFsize){
		printf("Error writing script to disk.\n");
		free(outputBuf);
		return -1;
	}

	/* Free Resources */
	free(outputBuf);
	fclose(outFile);

	/* Success */
	printf("Output file %s created successfully!\n\n",outFileName);
	
	return 0;
}
	

/************************************************************************************/
/* createActionScript - creates the action script in buffer using data from pMeta.  */
/* Returns the size of the script in bytes                                          */
/************************************************************************************/
unsigned int createActionScript(int numSections, actionScriptMetaType* pMeta, char* buffer){

	int index, x, y, z;
	unsigned int* pEntry32;
	unsigned short* pUshort16;
	unsigned int entryStartOffset;
	actionScriptHdrType* pHdr = (actionScriptHdrType*)buffer;
	unsigned int relByteOffset = numSections*8;

	/* Iterate through all sections of the action script */
	for(index = 0; index < numSections; index++){

		/* Align data on 4 byte boundary */
		if((relByteOffset % 4) != 0)
			relByteOffset += 4 - (relByteOffset % 4);

		/* Fill in the header of Script ID/Offsets */
		pHdr[index].actionScriptID = pMeta[index].actionScriptID;
		pHdr[index].actionScriptOffset = relByteOffset;
		swap32(&pHdr[index].actionScriptID);
		swap32(&pHdr[index].actionScriptOffset);

		/* Write out the preamble data */
		for(x = 0; x < (int)pMeta[index].preambleSizeBytes; x++){
			buffer[relByteOffset++] = (unsigned char)pMeta[index].preData[x];
		}

		/* Iterate on all 24 entries */
		pEntry32 = (unsigned int*)&buffer[relByteOffset];
		entryStartOffset = relByteOffset;
		relByteOffset += 24 * 4; /* First address following the Entry Offsets */
		for(x = 0; x < 24; x++){

			/* Update Relative Entry Start Offset */
			if(pMeta[index].numCmds[x] == 0){
				/* No Data for this Entry */
				pEntry32[x] = 0x00000000;
				continue;
			}
			else{
				/* Write relative short offset from start of entry offsets*/
				pEntry32[x] = (relByteOffset - entryStartOffset) / 2;
				swap32(&pEntry32[x]);
			}

			/* Copy Data that is associated with this entry */
			pUshort16 = (unsigned short*)&buffer[relByteOffset];
			for(y = 0; y < (int)pMeta[index].numCmds[x]; y++){

				/* Copy Script Command */
				*pUshort16 = (unsigned short)pMeta[index].entryData[x][y].scriptCmd;
				swap16(pUshort16);
				relByteOffset += 2;
				pUshort16++;

				/* Copy Command parameters */
				for(z = 0; z < (int)pMeta[index].entryData[x][y].numParameters; z++){
					*pUshort16 = (unsigned short)pMeta[index].entryData[x][y].pParameters[z];
					swap16(pUshort16);
					relByteOffset += 2;
					pUshort16++;
				}
			}
		}
	}

	/* Align end of data on a 4 byte boundary */
	if((relByteOffset % 4) != 0)
		relByteOffset += 4 - (relByteOffset % 4);

	/* Return the size of the data in bytes */
	return relByteOffset;
}

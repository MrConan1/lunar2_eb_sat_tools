/******************************************************************************/
/* main.c - Main execution for Updating Lunar 2 Item Script file (#2441)      */
/*          Updates text in Saturn Item File for Items Rx/Cant Carry.         */
/******************************************************************************/

/***********************************************************************/
/* Lunar 2 Item Script Updater Usage                                   */
/* ==================================================                  */
/* l2_itemScriptUpdater.exe satFname updateFile outputFolder           */
/*                                                                     */
/* Input File should be a #2441                                        */
/*                                                                     */
/***********************************************************************/


/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"


#define VER_MAJ    1
#define VER_MIN    1


#undef PRINT_CMDS

/* Sections of ES File Format */
/* 1.) 0x20 Byte ES Header */
/* 2.) Command Section     */
/* 3.) Text Section        */
/* 4.) Some data (not sure what)  */
/* 5.) Character Portrait section */
/* 6.) Zero padding (if needed) to make file size a multiple of 2048 bytes */
typedef struct{
    char esString[4];
    unsigned int portraitOffset;  /* Start of character portraits */
	unsigned int textOffsetMinusEsHeader; /* textOffset - 0x20 */
	unsigned int esHeaderSizeBytes;       /* Always 0x20 */
	unsigned int spareZeroes;    /* 4 bytes of zero */
	unsigned int textOffset;     /* Offset to dialog section */
	unsigned int sizeTextBytes;  /* Size of Text section in bytes */
	unsigned int textOffsetCopy; /* Offset to dialog section (identical to textOffset) */
}esHeaderType;

/* Item Packed File with ES Script embedded within it */
typedef struct{
    unsigned int unknown1;       /* ???  */
	unsigned int esHdrOffset;    /* Offset to ES Header */
	unsigned int unknown2;       /* Points to ??? */
	unsigned int unknown3;       /* Points to ??? */
 								 /* Need to modify if ES Section altered. */
}packedItemHeaderType;

/*
Building an updated file from a Saturn Packed Item File:
Copy Saturn header and file up to ES Script section.
Create modified ES script based on Update File and insert.
    Ensure command section matches saturn section.  Error if #dialogs are not equal.
Update "updateOffset4" and copy in all data up to the offset provided by LW 0 in the original file.
Now set LW 0 to the next location in the new file.
If it isnt already a multiple of 2048 bytes, zero pad the new file until the file size is a multiple of 2048 bytes.
*/


int dlgCmdLocation[1000];
int dlogCmdOffset[1000];
int dlgCmdLink[1000];

int dlogSatCmdOffset[1000];
int dlgSatCmdLocation[1000];

/***********************/
/* Function Prototypes */
/***********************/
void printUsage();
int createDlgTextFromUpdateFile(char* updateFname, int numDlogs, char* pOutbuf,
    unsigned int* outputSizeBytes);


/******************************************************************************/
/* printUsage() - Display command line usage of this program.                 */
/******************************************************************************/
void printUsage(){
    printf("Error in input arguments.\n\n");
    printf("Usage\n===============================\n");
    printf("l2_itemScriptUpdater.exe satFname_2441 txtUpdateFname outputName\n");
    printf("\n\n");
    return;
}


/******************************************************************************/
/* main()                                                                     */
/******************************************************************************/
int main(int argc, char** argv){

    packedItemHeaderType* pPackedHdr;
    FILE *inFile_SAT, *outFile;
	unsigned int* pUint;
	static char inSATFileName[300];
	static char inUpdateFileName[300];
    static char outFileName[300];
    int x, rval, portraitExists;
	unsigned int satESPortraitSizeBytes, satFileLocByteOffset;
	unsigned int updatedTextSizeBytes, updatedPortraitSection, remainderZeroes, numPortraits;
	int readsizeBytes;
    int saturn_es_file_start_offset;
	char* outputBuf;
	esHeaderType* pSaturnESHdr;
	unsigned int numSatTextPointers, satTextOffset, outputFsize;
	portraitExists = 1;

    printf("Lunar2 Saturn Item Dialog Text Updater v%d.%02d\n", VER_MAJ, VER_MIN);

    /**************************/
    /* Check input parameters */
    /**************************/

    /* Check for valid # of args */
    if (argc != 4){
        printUsage();
        return -1;
    }

    //Handle Input Parameters
    memset(inSATFileName, 0, 300);
	memset(inUpdateFileName, 0, 300);
	memset(outFileName, 0, 300);
	strcpy(inSATFileName,argv[1]);
	strcpy(inUpdateFileName,argv[2]);
	strcpy(outFileName,argv[3]);


    /*******************************/
    /* Open the input/output files */
    /*******************************/
    inFile_SAT = outFile = NULL;
    inFile_SAT = fopen(inSATFileName, "rb");
    if (inFile_SAT == NULL){
        printf("Error occurred while opening SAT input script %s for reading\n", inSATFileName);
        return -1;
    }

    /********************************/
    /* Parse the Input Script Files */
    /********************************/
    printf("Parsing input files: %s And %s.\n",inSATFileName,inUpdateFileName);	
	
    /***********************************************************************/
	/* Read the first 0x20 bytes of Saturn header into the output file     */
	/* Doesnt matter which type of file it is, both have headers there     */
	/***********************************************************************/
	outputBuf = (char*)malloc(1024*1024);   /* 1MB, overkill */
	if(outputBuf == NULL){
		printf("Error allocating room.\n");
		return -1;
	}
	memset(outputBuf,0,(1024*1024));
	outputFsize = 0;
    if( fread(outputBuf,1,0x20,inFile_SAT) != 0x20){
		printf("Error occured while reading input file.\n");
	}
	outputFsize = 0x20;

    /***************************************************************/
	/* Determine the start offset of the ES Header in the SAT File */
	/***************************************************************/

	/* The file doesnt start with an ES Header             */
	/* Its a packed file with other stuff in it.           */
	/* Copy all Saturn data up and including the ES Header */
	/* To the Output Buffer.                               */
	pUint = (unsigned int*)outputBuf;
		
	/* Swap the first 4 LW to read the offsets */
	for(x = 0; x < 4; x++){
		swap32(&pUint[x]);
	}
	pPackedHdr = (packedItemHeaderType*)outputBuf;
	saturn_es_file_start_offset = pPackedHdr->esHdrOffset;

	/* Swap the first 4 LW back */
	for(x = 0; x < 4; x++){
		swap32(&pUint[x]);
	}

	if(saturn_es_file_start_offset > 0){
		if( fread(&outputBuf[outputFsize],1,saturn_es_file_start_offset,inFile_SAT) != saturn_es_file_start_offset){
		    printf("Error occured while reading input file.\n");
		}
        outputFsize += saturn_es_file_start_offset;
	}
	pSaturnESHdr = (esHeaderType*)(&outputBuf[saturn_es_file_start_offset]);
	pUint = (unsigned int*)&outputBuf[saturn_es_file_start_offset];
	/* Swap ES header to read the offsets */
	for(x = 1; x < 8; x++){
		swap32(&pUint[x]);
	}
	satTextOffset = pSaturnESHdr->textOffset;
	
	/* Portrait Section Calculation */
	fseek(inFile_SAT, pSaturnESHdr->portraitOffset+saturn_es_file_start_offset, SEEK_SET);
	rval = fread(&numPortraits,1,4,inFile_SAT);
	if(rval != 4)
		portraitExists = -1;
	rval = fread(&satESPortraitSizeBytes,1,4,inFile_SAT);
	if(rval != 4)
		portraitExists = -1;
	swap32(&numPortraits);
	swap32(&satESPortraitSizeBytes);
	fseek(inFile_SAT, saturn_es_file_start_offset+0x20, SEEK_SET); //reset

    /* Verify a text section exists */
    if(pSaturnESHdr->sizeTextBytes == 0){
		printf("No Text Section Exists!  Hit enter to exit.\n");
		fflush(stdin);
		getchar();
		free(outputBuf);
		fclose(inFile_SAT);
		return 0;
	}
	
	/* Verify that a portrait section exists */
	if(pSaturnESHdr->portraitOffset <= pSaturnESHdr->textOffset){
		printf("No portrait section detected.\n");
		portraitExists = -1;
	}

	
	/* At this point the output holds a copy of the original saturn file */
    /* up to, and including the ES Header                                */
	/* pSaturnESHdr is pointing to the beginning of the ES Header.       */



	/*************************/
	/* Create the ES Section */
	/*************************/
	

    /*********************************************************************/
    /* Copy the Saturn Command Section and                               */
	/* verify text offsets are always incrementing.                      */
	/*********************************************************************/
	numSatTextPointers = 0;
	satFileLocByteOffset = 0x20;
	while(satFileLocByteOffset < satTextOffset){
		unsigned int cmd;
		unsigned int satData;
		if( fread(&outputBuf[outputFsize],1,4,inFile_SAT) != 4){
		    printf("Error occured while reading saturn input file.\n");
		}
		satData =  *((unsigned int*)(&outputBuf[outputFsize]));
		outputFsize+=4;
		swap32(&satData);
		cmd = satData;
		
		/* Dialog Command that points to the text section */
		if(((cmd & 0xFF000000) >> 24) == 0x1D){
			dlgSatCmdLocation[numSatTextPointers] = satFileLocByteOffset + saturn_es_file_start_offset;
			dlogSatCmdOffset[numSatTextPointers] = (cmd & 0x00FFFFFF)+satTextOffset;
			if((numSatTextPointers > 0) && 
			   (dlogSatCmdOffset[numSatTextPointers] < dlogSatCmdOffset[numSatTextPointers-1])){
				printf("Error in SAT Cmds, not continually increasing.\n");
				fflush(stdin);
      	        getchar();
				free(outputBuf);
				fclose(inFile_SAT);
				return -1;
			}
    		numSatTextPointers++;
		}
		satFileLocByteOffset += 4;
	}
	printf("SAT Num_Text_Pointers: 0x%X\n",numSatTextPointers);
	
	
	/***************************/
	/* Create the Text Section */
	/***************************/
	updatedTextSizeBytes = outputFsize;
	if( createDlgTextFromUpdateFile(inUpdateFileName, numSatTextPointers , outputBuf, &outputFsize) < 0){
		printf("Error creating text section of ES file.\n");
		fclose(inFile_SAT);
		free(outputBuf);
		return -1;
	}
	updatedTextSizeBytes = outputFsize - updatedTextSizeBytes;
	
	/* Copy Saturn Portraits Section */
	/* Set offset to next location after text script, regardless if there is a portraits section */
	updatedPortraitSection = outputFsize - saturn_es_file_start_offset;
	if(portraitExists >= 0){
	    fseek(inFile_SAT, pSaturnESHdr->portraitOffset+saturn_es_file_start_offset, SEEK_SET);
	    readsizeBytes = satESPortraitSizeBytes;
	    if(fread(&outputBuf[outputFsize],1,readsizeBytes,inFile_SAT) != readsizeBytes){
		    fclose(inFile_SAT);
		    free(outputBuf);
		    return -1;
	    }
        outputFsize += satESPortraitSizeBytes;
	}



	/* Now Fill with Zeroes such that the File is a multiple of 2048 bytes */
	if((outputFsize % 2048) != 0){
        remainderZeroes = 2048 - (outputFsize % 2048);
	    outputFsize += remainderZeroes;  /* Already zeroed */
	}
	
	/* Update ES Headers */
	pSaturnESHdr->sizeTextBytes = updatedTextSizeBytes;  /* Size of Text Section */
    pSaturnESHdr->portraitOffset = updatedPortraitSection;  /* Start of character portraits */

    /* Swap ES Headers back */
	pUint = (unsigned int*)pSaturnESHdr;
	for(x = 1; x < 8; x++){
		swap32(&pUint[x]);
	}
	
	
    /**********************************/
    /* Write the buffer to the output */
	/**********************************/
	outFile = fopen(outFileName, "wb");
    if (outFile == NULL){
        printf("Error occurred while opening SAT output file %s for writing\n", outFileName);
		free(outputBuf);
		fclose(inFile_SAT);
        return -1;
    }
	
    fwrite(outputBuf,1,outputFsize,outFile);	
    free(outputBuf);
	
    /* Close files */
	fclose(inFile_SAT);
	fclose(outFile);

    return 0;
}





/* Saturn version of US PSX Format */

/* 06xx means enter 8-bit text mode. */
/*   While in text mode, keep reading 8-bit text until:         */
/*   A.) 0600 encountered, which means return to command mode   */
/*   B.) A character with the upper 7th bit set is encountered. */
/*   C.) When re-entering CMD mode, must be on the next 16-bit  */
/*       aligned adderss.                                       */
/* Other notes:                                                 */
/*    0x1F is added to all 8-bit text values.  The upper bit    */
/*         test is performed prior to adding 0x1F               */
int createDlgTextFromUpdateFile(char* updateFname, int numDlogs, char* pOutbuf,
    unsigned int* outputSizeBytes){

	int textLen, x;
	static char linebuf[300];
	static char tag[300];
	unsigned int cc;
	unsigned short controlCode;
	unsigned short* pDlogLength;
	FILE* inUpdate = NULL;
	int bufferOffset = *outputSizeBytes;
	unsigned int relativeSatTxtOffset = 0;
	int countedDlogs = 0;

	inUpdate = fopen(updateFname, "rb");
    if (inUpdate == NULL){
        printf("Error occurred while opening Updater input file %s for reading\n", updateFname);
        return -1;
    }


	/********************************************************************/
	/* Text section will be translated & copied into the Saturn Output  */
	/* It is assumed the text in the update file is in order 1 to N.    */
	/* For each section of dialog copied, the corresponding saturn CMD  */
	/* offset will be modified to point to the proper location.         */
	/* Note that the updated text format will require an ASM hack on the*/
	/* saturn side, which shouldnt be too major.                        */
	/********************************************************************/

	/* Locate the beginning of text, search for <Text_Section> */
	while(1){
		fgets(linebuf,300,inUpdate);
		sscanf(linebuf,"%s",tag);
		if(strcmp(tag,"<Text_Section>") == 0)
			break;

		if(feof(inUpdate)){
			printf("Error, premature EOF reached while searching for <Text_Section>\n");
			fclose(inUpdate);
			return -1;
		}
	}


	/* Continue until last text section read. */
	/* If EOF or </Text_Section> encountered first, this is an error */
	while(1){
		fgets(linebuf,300,inUpdate);
		sscanf(linebuf,"%s",tag);
        if((strcmp(tag,"</Text_Section>") == 0) || feof(inUpdate)){
			printf("Error, premature end of Text Section Detected\n");
			fclose(inUpdate);
			return -1;
		}
		else if(strcmp(tag,"Text_Start") == 0){
	  	    /* Found Text_Start */
			/* When found, update Cmd Section Pointer and Dialog Size Pointer */
			*(unsigned int*)&pOutbuf[dlgSatCmdLocation[countedDlogs]] = 0x1D000000 | relativeSatTxtOffset;
			swap32(&pOutbuf[dlgSatCmdLocation[countedDlogs]]);
			pDlogLength = (unsigned short*)&pOutbuf[bufferOffset+relativeSatTxtOffset];
			*pDlogLength = 2;
			relativeSatTxtOffset += 2;

			countedDlogs++;
			//keep going
		}
		else{
			//Ignore this line, move on to the next
			continue;
		}
		
		
        /* In a dialog section if you get here.         */
	    /* Read text & ctrl codes and convert to binary */
		while(1){
			char* ptextStart;
   		    fgets(linebuf,300,inUpdate);
			if(linebuf[0] == '\"'){

				/* Convert the text in quotes to binary */
				
				/* Start with 0x06 */
                pOutbuf[bufferOffset+relativeSatTxtOffset] = 0x06;
		        relativeSatTxtOffset++;
				(*pDlogLength)++;
					
				/* Determine the length of text */
				ptextStart = &linebuf[1];
				textLen = strlen(ptextStart);
				while(1){
					if(ptextStart[textLen-1] != '\"')
						textLen--;
					else
						break;
				}
				textLen--;

//				printf("\n");
				for(x = 0; x < textLen; x++){
					pOutbuf[bufferOffset+relativeSatTxtOffset] = ptextStart[x] - 0x1F;
					relativeSatTxtOffset++;
//					printf(".%c.",ptextStart[x]);
				}
//				printf("\n");
				pOutbuf[bufferOffset+relativeSatTxtOffset-1] |= 0x80; /* set high bit on last character */

				/* Update the Dialog length */
				*pDlogLength += textLen;
				if(((*pDlogLength) % 2) != 0){
					/* Add 0x00 to the output for alignment purposes */
                    pOutbuf[bufferOffset+relativeSatTxtOffset] = 0x00;
		            relativeSatTxtOffset++;
					(*pDlogLength)++;
				}

			}
			else if(linebuf[0] == 'C'){
				/* Control Code detected, convert to Saturn Version and insert into the binary */
				/* The update files all use the PSX version of the control codes */
			    sscanf(linebuf,"%s %x",tag,&cc);
				controlCode = (unsigned short)cc;
				if(controlCode != 0x0000){
				    pOutbuf[bufferOffset+relativeSatTxtOffset] = (controlCode>>8) & 0xFF;
				    relativeSatTxtOffset++;
				    pOutbuf[bufferOffset+relativeSatTxtOffset] = controlCode & 0xFF;
				    relativeSatTxtOffset++;
				    *pDlogLength += 2;
				}
			}
			else if(linebuf[0] == 'T'){
				/* End of This Dialog Detected, only line that starts with a T would be Text_End */
				break;
			}
			else{
				printf("Error, unexpected entry, ignoring\n");
			}

		}

		swap16(pDlogLength);

		/* Check to see if complete */
		if(numDlogs == countedDlogs)
			break;
	}

	/* Ensure section is a multiple of 4 bytes */
	while((relativeSatTxtOffset % 4) != 0){
		pOutbuf[bufferOffset+relativeSatTxtOffset] = 0x00;
		relativeSatTxtOffset++;
	}

	/* Update size of output file */
	*outputSizeBytes += relativeSatTxtOffset;

	return 0;
}

/******************************************************************************/
/* main.c - Main execution file for Lunar 2 Script Converting                 */
/*          l2_psx2sat.exe                                                    */
/*          Converts Lunar 2 PSX US files to Saturn for translation project.  */
/******************************************************************************/

/***********************************************************************/
/* Lunar 2 Text Converter Usage                                        */
/* ==================================================                  */
/* l2_psx2sat.exe satFname psxFname outputFolder                       */
/*                                                                     */
/* Input File should be a file with an "ES" header somewhere inside of */
/* it (Search for 0x45530001; 0x4553 is "ES").  For some files this    */
/* header is at the beginning.  Others have it buried in the file.     */
/*                                                                     */
/* This exe does not convert the common item/battle text, just dialog  */
/* text.                                                               */
/***********************************************************************/


/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"


#define VER_MAJ    1
#define VER_MIN    2


#undef PRINT_CMDS

/* Note - It appears all sections need to be 4-byte aligned*/

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

/* Packed File with ES Script embedded within it */
typedef struct{
    unsigned int zeroPadOffset;  /* In Saturn, offset to zero padding */
                                 /* to 2048 byte alignment at EOF     */
								 /* Need to modify if ES Section altered. */
								 /* PSX holds something different.        */
    unsigned int offset1;        /* Points to some offset before ES Header */
	unsigned int offset2;        /* Points to some offset before ES Header */
	unsigned int esHdrOffset;    /* Offset to ES Header */
	unsigned int offset3;        /* Points to some offset before ES Header */
	unsigned int updateOffset4;  /* Points to some offset AFTER ES Header */
 								 /* Need to modify if ES Section altered. */
}packedHeaderType;

/*
Building an updated file from a Saturn Packed File:
Copy Saturn header and file up to ES Script section.
Create modified ES script based on PSX Script and insert.
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



/******************************************************************************/
/* printUsage() - Display command line usage of this program.                 */
/******************************************************************************/
void printUsage(){
    printf("Error in input arguments.\n\n");
    printf("Usage\n===============================\n");
    printf("l2_psx2sat.exe satFname psxFname outputFolder\n");
    printf("\n\n");
    return;
}


/******************************************************************************/
/* main()                                                                     */
/******************************************************************************/
int main(int argc, char** argv){

    packedHeaderType* pPackedHdr;
    FILE *inFile_SAT, *inFile_PSX, *outFile;
    unsigned char* buffer;
	unsigned int* pUint;
	unsigned int* pData32;
	unsigned short* pData16;
	static char inSATFileName[300];
	static char inPSXFileName[300];
    static char outFileName[300];
	unsigned int fsize;
    int x, outIndex, file_start_offset, rval, portraitExists;
	unsigned int textOffset, portraitOffset, fileLocByteOffset;
	unsigned int satESPortraitSizeBytes, numTextPointers, satFileLocByteOffset;
	unsigned int updatedTextSizeBytes, updatedPortraitSection, remainderZeroes, zeroBytes, numPortraits;
	int esFileFlg;
	int readsizeBytes;
    int saturn_es_file_start_offset;
    char* outputBuf;
	esHeaderType* pSaturnESHdr;
	unsigned int numSatTextPointers, satTextOffset, outputFsize;
	file_start_offset = 0;
	portraitExists = 1;

    printf("Lunar2 Psx2Sat v%d.%02d\n", VER_MAJ, VER_MIN);

    /**************************/
    /* Check input parameters */
    /**************************/

    /* Check for valid # of args */
    if (argc != 4){
        printUsage();
        return -1;
    }

    //Handle Generic Input Parameters
    memset(inSATFileName, 0, 300);
	memset(inPSXFileName, 0, 300);
	memset(outFileName, 0, 300);
	strcpy(inSATFileName,argv[1]);
	strcpy(inPSXFileName,argv[2]);
	strcpy(outFileName,argv[3]);
	if(outFileName[strlen(outFileName)-1] == '\\')
		outFileName[strlen(outFileName)-1] = '/';
	else if(outFileName[strlen(outFileName)-1] != '/')
		strcat(outFileName,"/");
	x = 0; outIndex = 0;
	while(inSATFileName[x] != '\0'){
		if((inSATFileName[x] == '/') || (inSATFileName[x] == '\\'))
			outIndex = x+1;
		x++;
	}
	strcat(outFileName,&inSATFileName[outIndex]);


    /*******************************/
    /* Open the input/output files */
    /*******************************/
    inFile_SAT = inFile_PSX = outFile = NULL;
    inFile_SAT = fopen(inSATFileName, "rb");
    if (inFile_SAT == NULL){
        printf("Error occurred while opening SAT input script %s for reading\n", inSATFileName);
        return -1;
    }
	inFile_PSX = fopen(inPSXFileName, "rb");
    if (inFile_PSX == NULL){
        printf("Error occurred while opening PSX input script %s for reading\n", inPSXFileName);
		fclose(inFile_SAT);
        return -1;
    }



    /********************************/
    /* Parse the Input Script Files */
    /********************************/
    printf("Parsing input files: %s And %s.\n",inSATFileName,inPSXFileName);
		
	
	/* Get the PSX file size */
	fseek(inFile_PSX,0,SEEK_END);
	fsize = ftell(inFile_PSX);
	fseek(inFile_PSX,0,SEEK_SET);
	
	/* Put the PSX file in memory */
	buffer = (unsigned char*)malloc(fsize*2);
	if(buffer == NULL){
		printf("Error allocating memory.\n");
	}
	if( fread(buffer,1,fsize,inFile_PSX) != fsize){
		printf("Error occured while reading input file.\n");
	}
	fclose(inFile_PSX);
	

    /***************************************************************/
	/* Determine the start offset of the ES Header in the PSX File */
	/* If "ES"0001 (0x45530001) is at the beginning, the file      */
	/* contains the script at the start of the file.  Otherwise    */
	/* we're dealing with the other type of packed file.           */
	/***************************************************************/
	if( (buffer[0] == 0x45) && (buffer[1] == 0x53) &&
        (buffer[2] == 0x00) && (buffer[3] == 0x01) ){
	    file_start_offset = 0;
		esFileFlg = 1;
	}
    else{
		unsigned int* pHeader = (unsigned int*)buffer;
		file_start_offset = pHeader[3];
		esFileFlg = 0;
	}
	
	
    /***********************************************************************/
	/* Now read the first 0x20 bytes of Saturn header into the output file */
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
	
	/* If the file doesnt start with an ES Header          */
	/* Then its a packed file with other stuff in it.      */
	/* Copy all Saturn data up and including the ES Header */
	/* To the Output Buffer.                               */
	saturn_es_file_start_offset = 0;
	if(!esFileFlg){
		int x;
		pUint = (unsigned int*)outputBuf;
		
		/* Swap the first 6 LW to read the offsets */
		for(x = 0; x < 6; x++){
			swap32(&pUint[x]);
		}
		pPackedHdr = (packedHeaderType*)outputBuf;
		saturn_es_file_start_offset = pPackedHdr->esHdrOffset;
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
		free(buffer);
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



	/********************************/
	/* Create the ES Header Section */
	/********************************/
	
	/* From the PSX File: */
	/* Read the offset to start of the Text Section */
	fileLocByteOffset = file_start_offset;
	pData32 = (unsigned int*)&buffer[file_start_offset];
	fileLocByteOffset += 0x20;
	textOffset = pData32[7] + file_start_offset;
	portraitOffset = pData32[1] + file_start_offset;

    printf("PSX Text Offset 0x%X\n",textOffset);
	

	
	/*****************************************************/
	/* Parse PSX Command Section                         */
	/* Also verify text offsets are always incrementing. */
	/*****************************************************/
	numTextPointers = 0;
	pData32 = (unsigned int*)&buffer[file_start_offset+0x20];
	while(fileLocByteOffset < textOffset){
		unsigned int cmd;
		cmd = *pData32;
		
		/* Dialog Command that points to the text section */
		if(((cmd & 0xFF000000) >> 24) == 0x1D){
			dlogCmdOffset[numTextPointers] = (cmd & 0x00FFFFFF)+textOffset;
			if((numTextPointers > 0) && 
			   (dlogCmdOffset[numTextPointers] < dlogCmdOffset[numTextPointers-1])){
				printf("Error in PSX Cmds, not continually increasing.\n");
				free(buffer);
				free(outputBuf);
				fclose(inFile_SAT);
				return -1;
			}
   	   	    numTextPointers++;
		}
		pData32++;
		fileLocByteOffset += 4;
	}
	printf("PSX Num_Text_Pointers: 0x%X\n",numTextPointers);

    /*********************************************************************/
    /* Copy the Saturn Command Section and                               */
	/* Also verify text offsets are always incrementing.                 */
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
	
	
	/*******************************************************/
	/* Verify the # of Dialog commands match between SAT   */
	/* and PSX.  Otherwise this needs to be fixed manually */
	/*******************************************************/
    if(numSatTextPointers != numTextPointers){
		printf("Error, # of Text Pointers do not match %d on SAT, %d on PSX!!\n", numSatTextPointers,numTextPointers);
		printf("File must be manually converted.\n");
		fflush(stdin);
		getchar();
		free(buffer);
		free(outputBuf);
		fclose(inFile_SAT);
		return -1;		
	}



	
	/***************************/
	/* Create the Text Section */
	/***************************/
	
	/********************************************************************/
	/* The PSX Text section will be copied into the Saturn Output file. */
	/* For each section of dialog copied, the corresponding saturn CMD  */
	/* offset will be modified to point to the proper location.         */
	/* Note that the updated text format will require an ASM hack on the*/
	/* saturn side, which shouldnt be too major.                        */
	/********************************************************************/
	
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
	/*    On PSX, Character 0x5F is a space not an _ character      */  /* FIX THIS! */

	/* Need to fix certain commands from PSX to Saturn */
	/* Unknown:           3xxx (Just remove it & adjust size) */
	/* Dialog Selection:  4xxx becomes 5xxx */
	/* Alt Btn Press:     6xxx becomes 8xxx */
    /* Item Text:         Axxx becomes Bxxx */
    /* Portrait Text:     Bxxx becomes Cxxx */
    /* Animations:        Cxxx becomes Dxxx */
    /* Text Pause:        Dxxx becomes Exxx */

	{
        unsigned short origSizeBytes;

		int skipData = 0;
		int textOutputActive = 0;
		int relativeSatTxtOffset = 0;
		int x = 0;
		
		pData16 = (unsigned short*)pData32;
		origSizeBytes = *pData16;
		while(fileLocByteOffset < portraitOffset){

			/* Read size of the next dialog */	
            unsigned short* pDlgSizeBytes;			
			unsigned short data;
			unsigned short modifiedNumBytes = 0;
            
			/* 16-bit Size in Bytes of Text, including the 2 bytes that hold the size */
			origSizeBytes = *pData16;
			if(origSizeBytes == 0x0000){
				/* No Dialog */
				printf("Zero Fill located (No Dialog)\n");
				memcpy(&outputBuf[outputFsize],pData16,2);outputFsize+=2;
				relativeSatTxtOffset += 2;
				fileLocByteOffset += 2;
				pData16++;
				continue;
			}
			
			/* Update Corresponding CMD Pointer's Offset to this Dialog Block */
			*(unsigned int*)&outputBuf[dlgSatCmdLocation[x]] = 0x1D000000 | relativeSatTxtOffset;
			swap32(&outputBuf[dlgSatCmdLocation[x]]);
			x++;

			/* Save a pointer to the text size */
			/* Skip copying the size for now */
			pDlgSizeBytes = (unsigned short*)&outputBuf[outputFsize];
			outputFsize+=2; 
			modifiedNumBytes = 2;
			origSizeBytes-=2;
			while(origSizeBytes > 0){
				
				/* Read next 16-bit word */
				pData16++;
				fileLocByteOffset += 2;
			    origSizeBytes -= 2;
				data = *pData16;
			
  			    /* PSX WD Font - Transition from Text Mode Inactive to Active */
			    if( ((data &0xFF00) == 0x0600) &&
			        (textOutputActive == 0) ){
				    unsigned char firstChar;
					textOutputActive = 1;
					firstChar = data & 0xFF;
							
					if(firstChar & 0x80){
						textOutputActive = 0;
					}

					/* Check for 0x5F, if found update to space & modify pData16 */
					if((firstChar & 0x7F) == (0x5F-0x1F)){
						firstChar = (firstChar & 0x80) | (0x20-0x1F);
						data = (data & 0xFF00) | firstChar;
						*pData16 = data;
					}
				}
				
				/* Return to Control Mode*/
				else if( (data == 0x0600) &&
					(textOutputActive == 1) ){
					textOutputActive = 0;	
				}
				
				/* PSX WD Font - Already in Text Mode */
				else if(textOutputActive){
					unsigned short firstWd,secondWd;
					firstWd = (0xFF00 & data) >> 8;
					secondWd =(0x00FF & data);
									
					if(firstWd & 0x80){
						textOutputActive = 0;
					}				
					if(secondWd & 0x80){
						textOutputActive = 0;
					}

					/* Check for 0x5F, if found update to space & modify pData16 */
					if((firstWd & 0x7F) == (0x5F-0x1F)){
						firstWd = (firstWd & 0x80) | (0x20-0x1F);
						data = (data & 0x00FF) | (firstWd << 8);
						*pData16 = data;
					}
					if((secondWd & 0x7F) == (0x5F-0x1F)){
						secondWd = (secondWd & 0x80) | (0x20-0x1F);
						data = (data & 0xFF00) | secondWd;
						*pData16 = data;
					}
				}
				
				/* Control Mode */
				else {
					unsigned short ctrlCode = data & 0xF000;
					switch(ctrlCode){
						case 0x3000:
							/**pData16 = 0x0000;*/
							skipData = 1;  /* Dont add any data */
							break;
						case 0x4000:
							*pData16 = 0x5000 | (data & 0x0FFF);
							break;
						case 0x6000:   /* This was previously missing, added 1-17-24 */
							*pData16 = 0x8000 | (data & 0x0FFF);
							break;
						case 0xA000:
							*pData16 = 0xB000 | (data & 0x0FFF);
							break;
						case 0xB000:
							*pData16 = 0xC000 | (data & 0x0FFF);
							break;
						case 0xC000:
							*pData16 = 0xD000 | (data & 0x0FFF);
							break;
						case 0xD000:
							*pData16 = 0xE000 | (data & 0x0FFF);
							break;
					}
					
				}
				
				if(skipData){
					skipData = 0;
				}
				else{
					/* Swap the data */
					swap16(pData16);
					memcpy(&outputBuf[outputFsize],pData16,2);outputFsize+=2;
					modifiedNumBytes += 2;
					relativeSatTxtOffset += 2;
				}
			}
			
			/* Update size of copied dialog block */
			*pDlgSizeBytes = modifiedNumBytes;
			swap16(pDlgSizeBytes);
			pData16++;
			fileLocByteOffset += 2;
			relativeSatTxtOffset += 2;
		}
	}
	//Align by 4
	if((outputFsize % 4) != 0)
		outputFsize += 2;
	updatedTextSizeBytes = (outputFsize - saturn_es_file_start_offset) - satTextOffset;
    free(buffer);
	/*************************************/
	/* Done with Text, so Done with PSX! */
	/*************************************/
	
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


	/* Only for files that are NOT packed */
	/* Fill with Zeroes such that the ES File is a multiple of 2048 bytes */
	if(esFileFlg){
		if((outputFsize % 2048) != 0){
 	        remainderZeroes = 2048 - (outputFsize % 2048);
	        outputFsize += remainderZeroes;  /* Already zeroed */
	    }
	}
	
	/* Update ES Headers */
	pSaturnESHdr->sizeTextBytes = updatedTextSizeBytes;  /* Size of Text Section */
    pSaturnESHdr->portraitOffset = updatedPortraitSection;  /* Start of character portraits */

    /* Swap ES Headers back */
	pUint = (unsigned int*)pSaturnESHdr;
	for(x = 1; x < 8; x++){
		swap32(&pUint[x]);
	}
	
	
	/*************************************************************/
	/* If a packed file, copy and update "Offset4" data          */
	/* After that add the zerodata section and update its offset */
	/* Once again, data is padded with zeroes to achieve a       */
	/* multiple of 2048 bytes.                                   */
	/*************************************************************/
	if(!esFileFlg){
		int readsizeBytes;
		int location;
		location = fseek(inFile_SAT, pPackedHdr->updateOffset4, SEEK_SET);
		if(location != 0){
			printf("Error, updateOffset4 invalid.\n");
			fflush(stdin);
			getchar();
			return -1;
		}
		readsizeBytes = pPackedHdr->zeroPadOffset - pPackedHdr->updateOffset4;
		pPackedHdr->updateOffset4 = outputFsize;
		if( fread(&outputBuf[outputFsize],1,readsizeBytes,inFile_SAT) != readsizeBytes){
			printf("Error reading data from packed file.\n");
			free(outputBuf);
			fclose(inFile_SAT);
			return -1;
		}
		outputFsize += readsizeBytes;
		pPackedHdr->zeroPadOffset = outputFsize;
		if((outputFsize % 2048) != 0){
 	        zeroBytes = 2048 - (outputFsize % 2048);
		    outputFsize += zeroBytes;
		}

		/* Swap Packed Headers back */
	    pUint = (unsigned int*)pPackedHdr;
	    for(x = 0; x < 6; x++){
		    swap32(&pUint[x]);
    	}
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

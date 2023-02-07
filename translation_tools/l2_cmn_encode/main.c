/******************************************************************************/
/* main.c - Main execution file for Lunar Common Text Encoder                 */
/******************************************************************************/

/***********************************************************************/
/* Lunar Common Text Encoder Usage                                     */
/* ========================================                            */
/* l2_cmn_encode.exe outputName                                        */
/* Expects for the following files in the immediate executable path:   */
/*                                                                     */
/*     Game Files                                                      */
/*     ==========                                                      */
/*     1233 - Original common file from Lunar EB Disc.                 */
/*                                                                     */
/*     Translation Files                                               */
/*     =================                                               */
/*     spells.txt - English Text for spells and descriptions.          */
/*     items.txt - English Text for items and descriptions.            */
/*     ui.txt - English Text for user interface messages.              */
/*     bromides.txt - English Text for bromides & misc text.           */
/*     locations.txt - English Text for locations.                     */
/*                                                                     */
/*     Encoding Files                                                  */
/*     =================                                               */
/*     l2_8bit_table.txt - UTF-8 to table encodings for the translation*/
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

/*

There are 136 Spells
There are 345 Items
There are 75 UI/Error Messages
There are 54 Bromides
There are 30 Locations

Spells    : 0x0C7D0 to 0x0D5BB  (0x0DEC = 3564  bytes)
Items     : 0x0D5C0 to 0x0F8E3  (0x2324 = 8996  bytes)
SpellEx   : 0x0F8E8 to 0x1013F  (0x858  = 2136 bytes) 
UI/Error  : 0x10144 to 0x10D23  (0x0BE0 = 63720 bytes)
Bromides  : 0x10D28 to 0x11097  (0x0370 = 880   bytes)
Locations : 0x1109C to 0x1128B  (0x01F0 = 496   bytes)

Dictionary has 0x1D48 (7496) bytes
At 0x17E60 there are 1A0 (416) bytes unused

For spells and items, use 2-byte pointers
An 0xA character will be used to separate the Name from the description.


*/


/***********/
/* Defines */
/***********/

/* File Offsets */
#define SPELL_OFFSET     0x0C7D0
#define ITEM_OFFSET      0x0D5C0
#define SPELL_EX_OFFSET  0x0F8E8
#define UI_OFFSET        0x10144
#define BROMIDE_OFFSET   0x10D28
#define LOCATION_OFFSET  0x1109C
#define DICT_OFFSET      0x11290

/* # of listings for each type */
#define NUM_SPELL_ITEMS        137
#define NUM_ITEMS              346
#define NUM_SPELL_EXTRA_ITEMS  89
#define NUM_UI_ITEMS           76
#define NUM_BROMIDE_ITEMS      55
#define NUM_LOCATION_ITEMS     31

/* Region sizes in bytes */
#define SPELL_SIZE             0xDEC
#define ITEM_SIZE              0x2324
#define SPELL_EX_SIZE          0x858
#define UI_SIZE                0xBE0
#define BROMIDE_SIZE           0x370
#define LOCATION_SIZE          0x1F0

/* Fixed Widths (# 16-BIT CHARACTERS) */
#define SPELL_NAME_FXD_WIDTH     10  //10 visible
#define SPELL_DESC_FXD_WIDTH     16  //15 visible
#define ITEM_NAME_FXD_WIDTH      10  //12 visible, not sure how big buffer is
#define ITEM_DESC_LN1_FXD_WIDTH  12
#define ITEM_DESC_LN2_FXD_WIDTH  04  //12 visible, not sure if buffer is larger than 4
#define SPELLEX_LN1_FXD_WIDTH    12
#define SPELLEX_LN2_FXD_WIDTH    12
#define UI_LN1_FXD_WIDTH         20
#define BROMIDE_LN1_FXD_WIDTH    16   // 14 visible
#define MAP_LN1_FXD_WIDTH        16



/* Extra Space in the file in bytes */
#define DICT_SIZE              0x1D48
#define EXTRA_SPACE_SIZE       0x1A0


/* Globals */

/* Function Prototypes */
int encodeSpells(int numSpells, char* inFileName, 
    unsigned char* outBuffer, unsigned int* outNumBytes);
int encodeItems(int numItems, char* inFileName, 
    unsigned char* outBuffer, unsigned int* outNumBytes);
int encodeSpellsEx(int numSpells, char* inFileName, 
    unsigned char* outBuffer, unsigned int* outNumBytes);
int encodeCmnStandard(int numSlots, char* inFileName, 
    unsigned char* outBuffer, unsigned int* outNumBytes);
int createUpdatedCommonTextFile(char* outFileName);


/******************************************************************************/
/* printUsage() - Display command line usage of this program.                 */
/******************************************************************************/
void printUsage(){
    printf("Error in input arguments.\n");
    printf("Usage:\n=========\n");
	printf("l2_cmn_encode.exe OutputFname\n\n");
    return;
}


/******************************************************************************/
/* main()                                                                     */
/******************************************************************************/
int main(int argc, char** argv){

    FILE *outFile;
	unsigned char* buffer;
    static char outFileName[300];
	static char outFileName_Spell[300];
	static char outFileName_Item[300];
	unsigned int outBytes = 0;

    /**************************/
    /* Check input parameters */
    /**************************/

	/* So it doesnt crash */
	if (argc < 2){
		printUsage();
		return -1;
	}

	/* Copy the output filename */
	memset(outFileName, 0, 300);
	strncpy(outFileName, argv[1], 299);
    
    /***********************************************/
    /* Finished Checking the Input File Parameters */
    /***********************************************/



    /********************/
    /* Perform Encoding */
    /********************/
	buffer = (unsigned char*)malloc(4*1024*1024);
	if(buffer == NULL){
		printf("Memory allocation error, exiting.\n");
		return -1;
	}

	/* Load in the Table File for Encoding Text */
	if (loadUTF8Table("l2_8bit_table.txt") < 0){
		printf("Error loading UTF8 Table for Text Encoding.\n");
		return -1;
	}

#if 1
	// Load BPE Table
	if (loadBPETable("bpe.table","l2_8bit_table.txt") < 0){
            printf("Error loading BPE Tables for Text Encoding/Decoding.\n");
            return -1;
    }
#endif

    printf("\n\nEncoding...\n\n");
	

	/*********************/
    /* Encode the spells */
	/*********************/
	outBytes = 0;
	memset(buffer,0,SPELL_SIZE);
	if(encodeSpells(NUM_SPELL_ITEMS, "spells.txt", buffer, &outBytes) < 0){
		printf("An error occurred while trying to encode spells.\n");
		free(buffer);
		return -1;
	}

	/* Open the output file */
	strcpy(outFileName_Spell, outFileName); 
	strcat(outFileName_Spell,"_spell.bin");
	outFile = fopen(outFileName_Spell,"wb");
	if(outFile == NULL){
		printf("Error occurred while opening %s for writing\n", outFileName_Spell);
		free(buffer);
        return -1;
    }

	/* Set size to original unless larger, then print a warning */
	if(outBytes > SPELL_SIZE){
		printf("WARNING, Spell size exceeds original!\n");
	}
	else
		outBytes = SPELL_SIZE;

    /* Write the binary version of the spells to the output file */
	if(fwrite(buffer,1,outBytes,outFile) != outBytes){
		printf("Error writing spells to output file.\n");
	    fclose(outFile);
		free(buffer);
		return -1;
	}

	/* Close output file */
    fclose(outFile);


	/********************/
	/* Encode the items */
	/********************/
	outBytes = 0;
	memset(buffer,0,ITEM_SIZE);
	if(encodeItems(NUM_ITEMS, "items.txt", buffer, &outBytes) < 0){
		printf("An error occurred while trying to encode spells.\n");
        fclose(outFile);
		free(buffer);
		return -1;
	}

	/* Open the output file */
	strcpy(outFileName_Item, outFileName); 
	strcat(outFileName_Item,"_items.bin");
	outFile = fopen(outFileName_Item,"wb");
	if(outFile == NULL){
		printf("Error occurred while opening %s for writing\n", outFileName_Item);
		free(buffer);
        return -1;
    }

	/* Set size to original unless larger, then print a warning */
	if(outBytes > ITEM_SIZE){
		printf("WARNING, Item size exceeds original!\n");
	}
	else
		outBytes = ITEM_SIZE;

    /* Write the binary version of the items to the output file */
	if(fwrite(buffer,1,outBytes,outFile) != outBytes){
		printf("Error writing spells to output file.\n");
	    fclose(outFile);
		free(buffer);
		return -1;
	}

	/* Close output file */
    fclose(outFile);


	/*******************************/
    /* Encode the Extra spell Info */
	/*******************************/
	outBytes = 0;
	memset(buffer,0,SPELL_EX_SIZE);
	if(encodeSpellsEx(NUM_SPELL_EXTRA_ITEMS, "extra_spell_msgs.txt", buffer, &outBytes) < 0){
		printf("An error occurred while trying to encode extra spell msgs.\n");
		free(buffer);
		return -1;
	}

	/* Open the output file */
	strcpy(outFileName_Spell, outFileName); 
	strcat(outFileName_Spell,"_extra_spell_msgs.bin");
	outFile = fopen(outFileName_Spell,"wb");
	if(outFile == NULL){
		printf("Error occurred while opening %s for writing\n", outFileName_Spell);
		free(buffer);
        return -1;
    }

	/* Set size to original unless larger, then print a warning */
	if(outBytes > SPELL_EX_SIZE){
		printf("WARNING, Spell size exceeds original!\n");
	}
	else
		outBytes = SPELL_EX_SIZE;

    /* Write the binary version of the spells to the output file */
	if(fwrite(buffer,1,outBytes,outFile) != outBytes){
		printf("Error writing spells to output file.\n");
	    fclose(outFile);
		free(buffer);
		return -1;
	}

	/* Close output file */
    fclose(outFile);



	/****************************/
	/* Encode the UI/Error Msgs */
	/****************************/
	outBytes = 0;
	memset(buffer,0,UI_SIZE);
	if(encodeCmnStandard(NUM_UI_ITEMS, "ui.txt", buffer, &outBytes) < 0){
		printf("An error occurred while trying to encode spells.\n");
        fclose(outFile);
		free(buffer);
		return -1;
	}

	/* Open the output file */
	strcpy(outFileName_Item, outFileName); 
	strcat(outFileName_Item,"_ui.bin");
	outFile = fopen(outFileName_Item,"wb");
	if(outFile == NULL){
		printf("Error occurred while opening %s for writing\n", outFileName_Item);
		free(buffer);
        return -1;
    }

	/* Set size to original unless larger, then print a warning */
	if(outBytes > UI_SIZE){
		printf("WARNING, UI/Error size exceeds original!\n");
	}
	else
		outBytes = UI_SIZE;

    /* Write the binary version of the items to the output file */
	if(fwrite(buffer,1,outBytes,outFile) != outBytes){
		printf("Error writing spells to output file.\n");
	    fclose(outFile);
		free(buffer);
		return -1;
	}

	/* Close output file */
    fclose(outFile);



	/****************************/
	/* Encode the Bromide Msgs */
	/***************************/
	outBytes = 0;
	memset(buffer,0,BROMIDE_SIZE);
	if(encodeCmnStandard(NUM_BROMIDE_ITEMS, "bromides.txt", buffer, &outBytes) < 0){
		printf("An error occurred while trying to encode spells.\n");
        fclose(outFile);
		free(buffer);
		return -1;
	}

	/* Open the output file */
	strcpy(outFileName_Item, outFileName); 
	strcat(outFileName_Item,"_bromides.bin");
	outFile = fopen(outFileName_Item,"wb");
	if(outFile == NULL){
		printf("Error occurred while opening %s for writing\n", outFileName_Item);
		free(buffer);
        return -1;
    }

	/* Set size to original unless larger, then print a warning */
	if(outBytes > BROMIDE_SIZE){
		printf("WARNING, Bromide size exceeds original!\n");
	}
	else
		outBytes = BROMIDE_SIZE;

    /* Write the binary version of the items to the output file */
	if(fwrite(buffer,1,outBytes,outFile) != outBytes){
		printf("Error writing bromides to output file.\n");
	    fclose(outFile);
		free(buffer);
		return -1;
	}

	/* Close output file */
    fclose(outFile);



	/****************************/
	/* Encode the Location Msgs */
	/****************************/
	outBytes = 0;
	memset(buffer,0,LOCATION_SIZE);
	if(encodeCmnStandard(NUM_LOCATION_ITEMS, "locations.txt", buffer, &outBytes) < 0){
		printf("An error occurred while trying to encode locations.\n");
        fclose(outFile);
		free(buffer);
		return -1;
	}

	/* Open the output file */
	strcpy(outFileName_Item, outFileName); 
	strcat(outFileName_Item,"_locations.bin");
	outFile = fopen(outFileName_Item,"wb");
	if(outFile == NULL){
		printf("Error occurred while opening %s for writing\n", outFileName_Item);
		free(buffer);
        return -1;
    }

	/* Set size to original unless larger, then print a warning */
	if(outBytes > LOCATION_SIZE){
		printf("WARNING, Location size exceeds original!\n");
	}
	else
		outBytes = LOCATION_SIZE;

    /* Write the binary version of the items to the output file */
	if(fwrite(buffer,1,outBytes,outFile) != outBytes){
		printf("Error writing locations to output file.\n");
	    fclose(outFile);
		free(buffer);
		return -1;
	}

	/* Close output file */
    fclose(outFile);
    free(buffer);

	/**************************************************************/
	/* Now put everything together and create the new common file */
	/**************************************************************/
	createUpdatedCommonTextFile(outFileName);



    return 0;
}





int createUpdatedCommonTextFile(char* outFileName){

	FILE* infile;
	FILE* outFile;
	int fsize, x, sectionSize;
	static char fname[300];
	char* buffer = NULL;
	char* tmpBuf = NULL;
	char* filenames[] = {"_spell.bin","_items.bin", "_extra_spell_msgs.bin","_ui.bin","_bromides.bin", "_locations.bin"};
	unsigned int offsets[] = {SPELL_OFFSET, ITEM_OFFSET, SPELL_EX_OFFSET, UI_OFFSET,
		                    BROMIDE_OFFSET, LOCATION_OFFSET, DICT_OFFSET};

	/* Open the output file for writing */
	outFile = fopen(outFileName,"wb");
	if(outFile == NULL){
		printf("Error occurred while opening %s for writing\n", outFileName);
		free(buffer);
        return -1;
    }
	printf("Writing final output to %s\n",outFileName);

	/* Open Original File for Reading, copy it all to the buffer */
	infile = NULL;
    infile = fopen("1233","rb");
    if(infile == NULL){
		printf("Error occurred while opening %s for reading\n", "1233");
        return -1;
    }

	/* Determine original file size, allocate memory, and copy the file to memory */
	fseek(infile,0,SEEK_END);
	fsize = ftell(infile);
	fseek(infile,0,SEEK_SET);
	buffer = (char*)malloc(fsize);
	if(buffer == NULL){
		printf("Memory allocation error, exiting.\n");
		return -1;
	}
	fread(buffer,1,fsize,infile);
	fclose(infile);
	
	/*  Overwrite Section 1 Data */
	/*  0x4 bytes
		==========
		00-01      # Image Data Headers to read in Section 2
		02-03      Offset First Data Header in Section 2
		
		Section2_File_Data_Location_Offset = (Offset) * 0x16 + 0x05A0
    */

	strcpy(fname,"section1_updates.txt");
	infile = fopen(fname,"r");
	if(infile == NULL){
		printf("Error occurred while opening %s for reading\n", fname);
		return -1;
	}
	while(1){
		unsigned int rval, offset, value1;
		unsigned short* pData;

		/* Read in modification information */		
		rval = fscanf(infile,"%X %X ",&offset,&value1);
		if(rval != 2)
			break;

		/* Overwrite Original Data */
		swap16(&value1);
		pData = (unsigned short*)&buffer[offset];		
		pData[0] = (unsigned short)value1;
	}
	fclose(infile);

	/* Overwrite Section 2 Data */
	/* 0x16 bytes
		==========
		00      Palette?
		01      Palette?
		02-03   (Offset / 8); 0x1400 usually added (1400*8 = 0xA000, offset in vdp1 ram of file data)
		04      Width / 8
		05      Height
		06-07   Relative X position
		08-09   Relative Y position
		0A-0B   word
		0C-0D   word
		0E-0F   word
		10-11   word 
		12-13   word
		14-15   word
	*/
	strcpy(fname,"section2_updates.txt");
	infile = fopen(fname,"r");
	if(infile == NULL){
		printf("Error occurred while opening %s for reading\n", fname);
		return -1;
	}
	while(1){
		unsigned int rval, offset, value1, value2, xcoord, ycoord;
		unsigned short* pData;

		/* Read in modification information */		
		rval = fscanf(infile,"%X %X %X %X %X",&offset,&value1, &value2, &xcoord, &ycoord);
		if(rval != 5)
			break;

		/* Overwrite Original Data */
		swap16(&value1);
		swap16(&value2);
		swap16(&xcoord);
		swap16(&ycoord);
		pData = (unsigned short*)&buffer[offset];		
		pData[1] = (unsigned short)value1; /* Offset */
		pData[2] = (unsigned short)value2; /* Width & Height */
		if(xcoord < 0xFFFF){
			pData[3] = (unsigned short)xcoord; /* X Coord */
			pData[4] = (unsigned short)ycoord; /* Y Coord */
		}
	}
	fclose(infile);

	/* Now overwrite with the replacement data */
	/* Spells, Items, Spell_ex, UI/Error Msgs, Bromides, Locations */
	for(x = 0; x < 6; x++){
		tmpBuf = NULL;

		/* Open modified data file & determine size of data */
		strcpy(fname,outFileName);
		strcat(fname,filenames[x]);
		infile = fopen(fname,"rb");
		if(infile == NULL){
			printf("Error occurred while opening %s for reading\n", fname);
			return -1;
		}
		fseek(infile,0,SEEK_END);
		sectionSize = ftell(infile);
		fseek(infile,0,SEEK_SET);

		/* Read in modified data */
		tmpBuf = (char*)malloc(sectionSize);
		if(tmpBuf == NULL){
			printf("Memory allocation error, exiting.\n");
			return -1;
		}
		fread(tmpBuf,1,sectionSize,infile);
		fclose(infile);

		/* Overwrite Original Data */
		memcpy(&buffer[offsets[x]],tmpBuf,sectionSize);
		free(tmpBuf);
	}

	/* Now overwrite the dictionary */
	tmpBuf = NULL;

	/* Open modified dictionary & determine size of data */
	strcpy(fname,"bpe.table");
	infile = fopen(fname,"rb");
	if(infile == NULL){
		printf("Error occurred while opening %s for reading\n", fname);
		return -1;
	}
	fseek(infile,0,SEEK_END);
	sectionSize = ftell(infile);
	fseek(infile,0,SEEK_SET);

	/* Read in modified data */
	tmpBuf = (char*)malloc(sectionSize);
	if(tmpBuf == NULL){
		printf("Memory allocation error, exiting.\n");
		return -1;
	}
	fread(tmpBuf,1,sectionSize,infile);
	fclose(infile);

	/* Overwrite Original Data */
	memset(&buffer[DICT_OFFSET],0,DICT_SIZE);
	memcpy(&buffer[DICT_OFFSET],tmpBuf,sectionSize);
	free(tmpBuf);

	/* Write the output file back to disc */
	if(fwrite(buffer,1,fsize,outFile) != fsize){
		printf("Error writing to output file.\n");
	    fclose(outFile);
		free(buffer);
		return -1;
	}
	fclose(outFile);
	free(buffer);
	return 0;
}








/* Reads spells from an input file and encodes them */
/* Expected format is:  Original_Address_Hex, Spell_Number_Dec, "Spell_Name" "Spell_Desc" */
/* Example: 0x1234, 1, "Name" "Description" */
/* Output Format: Compr_Name, 0xFFFF, Compr_Desc, 0xFFFF */

#define DELIM "\""

int encodeSpells(int numSpells, char* inFileName, 
    unsigned char* outBuffer, unsigned int* outNumBytes){

	FILE* infile;
	unsigned char* pData, *pChar;
	unsigned short* pTextPointers;
	static char line[1024];
	static char str1[1024];
	static char str2[1024];
	static char modifiedStr[1024];
	int outputSizeBytes;
	int numTextPointers, str1Len,str2Len, numCharacters;
	unsigned int oldAddress;
	int txtIndex, x;

	/* Init Output Size */
	*outNumBytes = 0;

	/* Open Input file */
	infile = NULL;
    infile = fopen(inFileName,"rb");
    if(infile == NULL){
		printf("Error occurred while opening %s for reading\n", inFileName);
        return -1;
    }
	
	/* Create and init the text pointers */
	numTextPointers = numSpells;
	memset(outBuffer,0x0,numTextPointers*2);
	pTextPointers = (unsigned short*)outBuffer;
	outputSizeBytes = numTextPointers*2;

    /* Parse the entries line by line */
    while(!feof(infile)){
		
		/* Read the line, make sure it starts with "0x" */
		/* Otherwise, just ignore it */
		memset(line,0,1023);
	    fgets(line,1023,infile);
		if( strncmp(line,"0x",2) != 0)
			continue;
		
		/* Read the old address and the index */
		sscanf((char*)line, "%X %d", &oldAddress, &txtIndex);
		
		/* Read the First Spell String */
		str1Len = 0;
		pData = (unsigned char*)strtok((char *)line, DELIM);
		pData = (unsigned char*)strtok(NULL, DELIM);
		if(pData == NULL)
			continue;
		x = 0;
		while((pData[x] != '\0') && (pData[x] != '\"')){
			x++;
		}
		str1Len = x;
		strcpy(str1,(char*)pData);

		/* Convert the UTF-8 text to use our table */
        if(str1Len > 0){
			pChar = (unsigned char*)str1;
			numCharacters = 0;
			while(*pChar != '\0'){
				unsigned char utf8Code;
				int nBytes;
				nBytes = numBytesInUtf8Char(*pChar);
				if(getUTF8code_Byte((char*)pChar, &utf8Code) < 0){
					int y;
					printf("Error, missing code for %d byte character!\n",nBytes);
					for(y=0;y<nBytes;y++){
						printf("\tByte %d: 0x%2X\n",y,pChar[y]);
					}
					fclose(infile);
					return -1;
				}
				modifiedStr[numCharacters] = utf8Code;

				pChar += nBytes;
				numCharacters++;
			}
			modifiedStr[numCharacters] = 0;
			memset(str1,0,1024);
			strncpy(str1,modifiedStr,numCharacters);
			str1Len = numCharacters;
		}

		/* Read the Second Spell String */
		str2Len = 0;
		pData = (unsigned char*)strtok(NULL, DELIM);
		if(pData == NULL)
			continue;
		pData = (unsigned char*)strtok(NULL, DELIM);
		if(pData != NULL){
			x = 0;
			while((pData[x] != '\0') && (pData[x] != '\"')){
				x++;
			}
			str2Len = x;
			strcpy(str2,(char*)pData);
		}
		else{
			str2Len = 0;
			str2[0] = 0;
		}

		/* Convert the UTF-8 text to use our table */
		if(str2Len > 0){
			pChar = (unsigned char*)str2;
			numCharacters = 0;
			while(*pChar != '\0'){
				unsigned char utf8Code;
				int nBytes;
				nBytes = numBytesInUtf8Char(*pChar);
				if(getUTF8code_Byte((char*)pChar, &utf8Code) < 0){
					int y;
					printf("Error, missing code for %d byte character!\n",nBytes);
					for(y=0;y<nBytes;y++){
						printf("\tByte %d: 0x%2X\n",y,pChar[y]);
					}
					fclose(infile);
					return -1;
				}
				modifiedStr[numCharacters] = utf8Code;

				pChar += nBytes;
				numCharacters++;
			}
			modifiedStr[numCharacters] = 0;
			memset(str2,0,1024);
			strncpy(str2,modifiedStr,numCharacters);
			str2Len = numCharacters;
		}

		/* Now Encode to the output file */
		/* Set the associated index to point to the next text location */
		/* Index stores the relative short word location from the beginning of the section */
		/* Each entry must be on a short word boundary.                                    */
		/* Encode the first text as compressed BPE, terminate with 0xFF or 0xFFFF */
		/* Encode the second text as compressed BPE, terminate with 0xFF or 0xFFFF */


        //BPE Compression
        if(str1Len > 0)
			compressBPE((unsigned char*)str1,(unsigned int*) &str1Len);
		if(str2Len > 0)
		    compressBPE((unsigned char*)str2,(unsigned int*) &str2Len);

		/* Update Pointer to Spell */
		pTextPointers[txtIndex] = (unsigned short)(outputSizeBytes / 2) & 0xFFFF;
		swap16(&pTextPointers[txtIndex]);

        /* Copy Spell Name, terminate with spacing, not 0xFFFF */
		/* If not aligned on a 16-bit boundary, just need 0xFF */
		memcpy(&outBuffer[outputSizeBytes], str1, str1Len);
		outputSizeBytes += str1Len;
//		if((outputSizeBytes % 2) == 0)

		outBuffer[outputSizeBytes++] = 0xF4;
		outBuffer[outputSizeBytes++] = 0xFE;  //Zero out rest of line
//		outBuffer[outputSizeBytes++] = 0x0A;  //Separator

		
		/* Copy Spell Description, terminate with 0xFFFF */
		/* If not aligned on a 16-bit boundary, just need 0xFF */
		memcpy(&outBuffer[outputSizeBytes], str2, str2Len);
		outputSizeBytes += str2Len;
		if((outputSizeBytes % 2) == 0)
			outBuffer[outputSizeBytes++] = 0xFF;
		outBuffer[outputSizeBytes++] = 0xFF;
	}

    /* Update number of bytes written */
	*outNumBytes = outputSizeBytes;
	fclose(infile);
	return 0;
}




/* Reads items from an input file and encodes them */
/* Expected format is:  Original_Address_Hex, Item_Number_Dec, "Item_Name" "Item_Desc" */
/* Example: 0x1234, 1, "" "" */
/* Output Format: */
int encodeItems(int numItems, char* inFileName, 
    unsigned char* outBuffer, unsigned int* outNumBytes){

	FILE* infile;
	unsigned char* pData, *pChar;
	unsigned short* pTextPointers;
	static char line[1024];
	static char str1[1024];
	static char str2[1024];
	static char modifiedStr[1024];
	int outputSizeBytes;
	int numTextPointers, str1Len,str2Len, numCharacters;
	unsigned int oldAddress;
	int txtIndex, x;

	/* Init Output Size */
	*outNumBytes = 0;

	/* Open Input file */
	infile = NULL;
    infile = fopen(inFileName,"rb");
    if(infile == NULL){
		printf("Error occurred while opening %s for reading\n", inFileName);
        return -1;
    }
	
	/* Create and init the text pointers */
	numTextPointers = numItems;
	memset(outBuffer,0x0,numTextPointers*2);
	pTextPointers = (unsigned short*)outBuffer;
	outputSizeBytes = numTextPointers*2;

    /* Parse the entries line by line */
    while(!feof(infile)){
		
		/* Read the line, make sure it starts with "0x" */
		/* Otherwise, just ignore it */
		memset(line,0,1023);
	    fgets(line,1023,infile);
		if( strncmp(line,"0x",2) != 0)
			continue;
		
		/* Read the old address and the index */
		sscanf((char*)line, "%X %d", &oldAddress, &txtIndex);
		
		/* Read the First Item String */
		str1Len = 0;
		pData = (unsigned char*)strtok((char *)line, DELIM);
		pData = (unsigned char*)strtok(NULL, DELIM);
		if(pData == NULL)
			continue;
		x = 0;
		while((pData[x] != '\0') && (pData[x] != '\"')){
			x++;
		}
		str1Len = x;
		strcpy(str1,(char*)pData);

		/* Convert the UTF-8 text to use our table */
        if(str1Len > 0){
			pChar = (unsigned char*)str1;
			numCharacters = 0;
			while(*pChar != '\0'){
				unsigned char utf8Code;
				int nBytes;
				nBytes = numBytesInUtf8Char(*pChar);
				if(getUTF8code_Byte((char*)pChar, &utf8Code) < 0){
					int y;
					printf("Error, missing code for %d byte character!\n",nBytes);
					for(y=0;y<nBytes;y++){
						printf("\tByte %d: 0x%2X\n",y,pChar[y]);
					}
					fclose(infile);
					return -1;
				}
				modifiedStr[numCharacters] = utf8Code;

				pChar += nBytes;
				numCharacters++;
			}
			strncpy(str1,modifiedStr,numCharacters);
			str1Len = numCharacters;
		}

		/* Read the Second Item String */
		str2Len = 0;
		pData = (unsigned char*)strtok(NULL, DELIM);
		if(pData == NULL)
			continue;
		pData = (unsigned char*)strtok(NULL, DELIM);
		if(pData != NULL){
			x = 0;
			while((pData[x] != '\0') && (pData[x] != '\"')){
				x++;
			}
			str2Len = x;
			strcpy(str2,(char*)pData);
		}
		else{
			str2Len = 0;
			str2[0] = 0;
		}

		/* Convert the UTF-8 text to use our table */
		if(str2Len > 0){
			pChar = (unsigned char*)str2;
			numCharacters = 0;
			while(*pChar != '\0'){
				unsigned char utf8Code;
				int nBytes;

				/* Insert carriage return */
				if((*pChar == '\\') && (*(pChar+1) == 'n')){
					pChar += 2;

					//Input code to zero out up to rest of this line
					modifiedStr[numCharacters] = 0xF4;
					numCharacters++;
					modifiedStr[numCharacters] = 0xFD;  // <-- The whole line/2 thing doesnt happen.  They are treated as individual lines.
					numCharacters++;

					continue;
				}

				nBytes = numBytesInUtf8Char(*pChar);
				if(getUTF8code_Byte((char*)pChar, &utf8Code) < 0){
					int y;
					printf("Error, missing code for %d byte character!\n",nBytes);
					for(y=0;y<nBytes;y++){
						printf("\tByte %d: 0x%2X\n",y,pChar[y]);
					}
					fclose(infile);
					return -1;
				}
				modifiedStr[numCharacters] = utf8Code;

				pChar += nBytes;
				numCharacters++;
			}
			strncpy(str2,modifiedStr,numCharacters);
			str2Len = numCharacters;
		}

		/* Now Encode to the output file */
		/* Set the associated index to point to the next text location */
		/* Index stores the relative short word location from the beginning of the section */
		/* Each entry must be on a short word boundary.                                    */
		/* Encode the first text as compressed BPE, terminate with 0xFF or 0xFFFF */
		/* Encode the second text as compressed BPE, terminate with 0xFF or 0xFFFF */

		//BPE Compression
        if(str1Len > 0)
			compressBPE((unsigned char*)str1,(unsigned int*) &str1Len);
		if(str2Len > 0)
		    compressBPE((unsigned char*)str2,(unsigned int*) &str2Len);


		/* Update Pointer to Item */
		pTextPointers[txtIndex] = (unsigned short)(outputSizeBytes / 2) & 0xFFFF;
		swap16(&pTextPointers[txtIndex]);

        /* Copy Item Name, terminate with spacing not 0xFFFF */
		/* If not aligned on a 16-bit boundary, just need 0xFF */
		memcpy(&outBuffer[outputSizeBytes], str1, str1Len);
		outputSizeBytes += str1Len;
//		if((outputSizeBytes % 2) == 0)

		outBuffer[outputSizeBytes++] = 0xF4;
		outBuffer[outputSizeBytes++] = 0xFE;  //Zero out rest of line
//		outBuffer[outputSizeBytes++] = 0x0A;  //Separator

		/* Copy Item Description, terminate with 0xFFFF */
		/* If not aligned on a 16-bit boundary, just need 0xFF */
		memcpy(&outBuffer[outputSizeBytes], str2, str2Len);
		outputSizeBytes += str2Len;
		if((outputSizeBytes % 2) == 0)
			outBuffer[outputSizeBytes++] = 0xFF;
		outBuffer[outputSizeBytes++] = 0xFF;
	}

    /* Update number of bytes written */
	*outNumBytes = outputSizeBytes;
	fclose(infile);
	return 0;
}



/* Reads extra spell info from an input file and encodes them */
/* Expected format is:  Original_Address_Hex, Spell_Number_Dec, "Spell_Name" "Spell_Desc" */
/* Example: 0x1234, 1, "Description" */
/* Output Format: Compr_Desc, 0xFFFF */

int encodeSpellsEx(int numSpells, char* inFileName, 
    unsigned char* outBuffer, unsigned int* outNumBytes){

	FILE* infile;
	unsigned char* pData, *pChar;
	unsigned short* pTextPointers;
	static char line[1024];
	static char str1[1024];
	static char modifiedStr[1024];
	int outputSizeBytes;
	int numTextPointers, str1Len, numCharacters;
	unsigned int oldAddress;
	int txtIndex, x;

	/* Init Output Size */
	*outNumBytes = 0;

	/* Open Input file */
	infile = NULL;
    infile = fopen(inFileName,"rb");
    if(infile == NULL){
		printf("Error occurred while opening %s for reading\n", inFileName);
        return -1;
    }
	
	/* Create and init the text pointers */
	numTextPointers = numSpells;
	memset(outBuffer,0x0,numTextPointers*2);
	pTextPointers = (unsigned short*)outBuffer;
	outputSizeBytes = numTextPointers*2;

    /* Parse the entries line by line */
    while(!feof(infile)){
		
		/* Read the line, make sure it starts with "0x" */
		/* Otherwise, just ignore it */
		memset(line,0,1023);
	    fgets(line,1023,infile);
		if( strncmp(line,"0x",2) != 0)
			continue;
		
		/* Read the old address and the index */
		sscanf((char*)line, "%X %d", &oldAddress, &txtIndex);
		
		/* Read the First Spell String */
		str1Len = 0;
		pData = (unsigned char*)strtok((char *)line, DELIM);
		pData = (unsigned char*)strtok(NULL, DELIM);
		if(pData == NULL)
			continue;
		x = 0;
		while((pData[x] != '\0') && (pData[x] != '\"')){
			x++;
		}
		str1Len = x;
		strcpy(str1,(char*)pData);

		/* Convert the UTF-8 text to use our table */
        if(str1Len > 0){
			pChar = (unsigned char*)str1;
			numCharacters = 0;
			while(*pChar != '\0'){
				unsigned char utf8Code;
				int nBytes;
				
				/* Insert carriage return */
				if((*pChar == '\\') && (*(pChar+1) == 'n')){
					pChar += 2;

					//Zero out up to rest of this line/2
					modifiedStr[numCharacters] = 0xF4;
					numCharacters++;
					modifiedStr[numCharacters] = 0xFD;
					numCharacters++;
//					modifiedStr[numCharacters] = 0x0A;  // FIX ME, Zero out rest of line only
//					numCharacters++;
					continue;
				}
				
				nBytes = numBytesInUtf8Char(*pChar);
				if(getUTF8code_Byte((char*)pChar, &utf8Code) < 0){
					int y;
					printf("Error, missing code for %d byte character!\n",nBytes);
					for(y=0;y<nBytes;y++){
						printf("\tByte %d: 0x%2X\n",y,pChar[y]);
					}
					fclose(infile);
					return -1;
				}
				modifiedStr[numCharacters] = utf8Code;

				pChar += nBytes;
				numCharacters++;
			}
			modifiedStr[numCharacters] = 0;
			memset(str1,0,1024);
			strncpy(str1,modifiedStr,numCharacters);
			str1Len = numCharacters;
		}

		/* Now Encode to the output file */
		/* Set the associated index to point to the next text location */
		/* Index stores the relative short word location from the beginning of the section */
		/* Each entry must be on a short word boundary.                                    */
		/* Encode the first text as compressed BPE, terminate with 0xFF or 0xFFFF */
		/* Encode the second text as compressed BPE, terminate with 0xFF or 0xFFFF */


        //BPE Compression
        if(str1Len > 0)
			compressBPE((unsigned char*)str1,(unsigned int*) &str1Len);

		/* Update Pointer to Spell */
		pTextPointers[txtIndex] = (unsigned short)(outputSizeBytes / 2) & 0xFFFF;
		swap16(&pTextPointers[txtIndex]);

        /* Copy Spell Name, terminate with spacing, not 0xFFFF */
		/* If not aligned on a 16-bit boundary, just need 0xFF */
		memcpy(&outBuffer[outputSizeBytes], str1, str1Len);
		outputSizeBytes += str1Len;
		if((outputSizeBytes % 2) == 0)
			outBuffer[outputSizeBytes++] = 0xFF;
		outBuffer[outputSizeBytes++] = 0xFF;
	}

    /* Update number of bytes written */
	*outNumBytes = outputSizeBytes;
	fclose(infile);
	return 0;
}




#define OPEN_QUOTE   0
#define CLOSED_QUOTE 1


/* Reads items from an input file and encodes them */
/* Expected format is:  Original_Address_Hex, Item_Number_Dec, "Text" */
/* Example: 0x1234, 1, "Text" */
/* Output Format: Compr_Text, 0xFF or 0xFFFF*/
int encodeCmnStandard(int numSlots, char* inFileName, 
    unsigned char* outBuffer, unsigned int* outNumBytes){

	FILE* infile;
	unsigned char* pData, *pChar, *pTmpOutput, *pStrEnd;
	unsigned short* pTextPointers;
	static char line[1024];
	static char str1[1024];
	static char modifiedStr[1024];
	int outputSizeBytes;
	int numTextPointers, str1Len, numCharacters;
	unsigned int oldAddress;
	int txtIndex;
	int tmpOutputSize = 0;


	/* Init Output Size */
	*outNumBytes = 0;

	/* Open Input file */
	infile = NULL;
    infile = fopen(inFileName,"rb");
    if(infile == NULL){
		printf("Error occurred while opening %s for reading\n", inFileName);
        return -1;
    }
	
	/* Create and init the text pointers */
	numTextPointers = numSlots;
	memset(outBuffer,0x0,numTextPointers*2);
	pTextPointers = (unsigned short*)outBuffer;
	outputSizeBytes = numTextPointers*2;

	/* Temp output buffer */
	pTmpOutput = (unsigned char*)malloc(1024*1024);
	if(pTmpOutput == NULL){
		printf("Memory alloc error for temp buffer.\n");
		return -1;
	}

    /* Parse the entries line by line */
    while(!feof(infile)){
		int parseError = 0;
	    int termFlag = 0;
		int emptySlotFlg = 0;
		int quoteFlag = OPEN_QUOTE;
		tmpOutputSize = 0;

		/* Read the line, make sure it starts with "0x" */
		/* Otherwise, just ignore it */
		memset(line,0,1023);
	    fgets(line,1023,infile);
		if( strncmp(line,"0x",2) != 0)
			continue;
		
		/* Read the old address and the index */
		sscanf((char*)line, "%X %d", &oldAddress, &txtIndex);
		
		/* Extract the Common String */
		pData = (unsigned char*)line;
		while(*pData != '\"'){
			pData++;
			if(*pData == '\0'){
//				printf("Empty Slot.\n");
				emptySlotFlg = 1;
                break;
			}
		}
		if(emptySlotFlg)
			continue;
		pData++;

		str1Len = strlen(line);
#if 0
		printf("%s\n",line);
#endif
		pStrEnd = (unsigned char*) (&line[str1Len-1]);
		while(*pStrEnd != '\"'){
			pStrEnd--;
			if(pStrEnd <= pData){
				printf("Input file error detected.\n");
				parseError = 1;
			}
		}
		if(parseError){
			continue;
		}
		*pStrEnd = '\0';


		/* pData now points to a Null terminated string containing a mixture */
		/* of text and control codes to be output.  Control codes all begin  */
		/* with 0xF*** and get decoded on the saturn side.                   */


		/* Now parse the utf-8 string looking for escape sequences for special */
		/* characters or Control Codes in Hex.                                 */
		/* Compress UTF-8 Strings and pad such that all text and hex codes begin        */
		/* on a 16-bit boundary.  Pad with 0x00 if the current line is not complete     */
		/* Pad with 0xFF if it is complete.  Add a final 0xFFFF if the line is complete */
		/* and prior padding of 0xFF did not occur.                                     */
		while(*pData != '\0'){
			int x,y;
			int hexValueLocated = 0;

			/* Copy the first string to str1 */
			x = y = 0;
			while(1){
				/* End of String */
				if(pData[x] == '\0')
					break;
				/* Escape Sequence */
				if(pData[x] == '\\'){
					x++;
					if(pData[x] == 'X'){
						/* Hex Value Follows */
						hexValueLocated = 1;
						x++;
						break;
					}
					else if(pData[x] == '\"'){
						if(quoteFlag == OPEN_QUOTE){
							/* Open Quote */
							quoteFlag = CLOSED_QUOTE;
							str1[y++] = 0x5B;  /* [ is open quote " */
						}
						else{
							/* Closed Quote */
							quoteFlag = OPEN_QUOTE;
							str1[y++] = 0x5D;  /* ] is closed quote " */
						}
						x++;
						continue;
					}
					else{
						; /* Nothing extra to be done */
						  /* The next character is to be printed */
					}
				}
				str1[y++] = pData[x];
				x++;
			}
			str1[y] = '\0';
			str1Len = y;

			/* Advance pointer to the next string after this one */
			pData += x;

			/* Encode string according to table file, then compress */
			if(str1Len > 0){
				pChar = (unsigned char*)str1;
				numCharacters = 0;
				while(*pChar != '\0'){
					unsigned char utf8Code;
					int nBytes;
					nBytes = numBytesInUtf8Char(*pChar);
					if(getUTF8code_Byte((char*)pChar, &utf8Code) < 0){
						int y;
						printf("Error, missing code for %d byte character!\n",nBytes);
						for(y=0;y<nBytes;y++){
							printf("\tByte %d: 0x%2X\n",y,pChar[y]);
						}
						fclose(infile);
						return -1;
					}
					modifiedStr[numCharacters] = utf8Code;

					pChar += nBytes;
					numCharacters++;
				}
				strncpy(str1,modifiedStr,numCharacters);
				str1Len = numCharacters;

				//BPE Compression
		        if(str1Len > 0){
					compressBPE((unsigned char*)str1,(unsigned int*) &str1Len);

					//Add to output string -- TBD
					memcpy(&pTmpOutput[tmpOutputSize],str1,str1Len);
					tmpOutputSize += str1Len;
					if((tmpOutputSize % 2) != 0){
						if(*pData == '\0'){
							pTmpOutput[tmpOutputSize++] = 0xFF;
							termFlag = 1;
						}
						// I dont think that this is needed
						//else
						//	pTmpOutput[tmpOutputSize++] = 0x00;
					}
				}
			}
			
			/* If Hex Data, Output Directly */
			if(hexValueLocated){
				unsigned int hexData;
				unsigned short swHexData;

				memset(str1,0,10);
				strncpy(str1,(char*)pData,4);
				sscanf(str1,"%X",&hexData);
				swHexData = hexData & 0xFFFF;
				swap16(&swHexData);
				pData += 4;

				//Add to output string
				memcpy(&pTmpOutput[tmpOutputSize],&swHexData,2);
				tmpOutputSize += 2;
			}

		}

		/* Ensure there is a string termination character or short word */
		if(!termFlag){
			unsigned short swHexData = 0xFFFF;
			memcpy(&pTmpOutput[tmpOutputSize],&swHexData,2);
			tmpOutputSize += 2;
		}


		/* Now Encode the current line to the output file */
		/* Set the associated index to point to the next text location */
		/* Index stores the relative short word location from the beginning of the section */
		/* Each entry must be on a short word boundary.                                    */
		pTextPointers[txtIndex] = (unsigned short)(outputSizeBytes / 2) & 0xFFFF;
		swap16(&pTextPointers[txtIndex]);
		memcpy(&outBuffer[outputSizeBytes], pTmpOutput, tmpOutputSize);
		outputSizeBytes += tmpOutputSize;
	}

    /* Update number of bytes written */
	*outNumBytes = outputSizeBytes;

	free(pTmpOutput);
	fclose(infile);
	return 0;
}


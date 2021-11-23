/******************************************************************************/
/* main.c - Main execution file for lunar Common Text Encoder                 */
/******************************************************************************/

/***********************************************************************/
/* Lunar Menu Editor (lunar_menu.exe) Usage                            */
/* ========================================                            */
/* lunar_menu.exe decodemenu InputMenuFname [sss]                      */
/* lunar_menu.exe encodemenu InputMenuFname InputCsvFname [sss]        */
/*     sss will interpret SSSC JP table as the SSS JP table.           */
/*     encoding assumes UTF8 characters.                               */
/*     Output will encode text with 8 bits per byte.  Control Codes    */
/*     are 16-bit, and records of control codes/text will start and    */
/*     end on 16-bit aligned boundaries.  Not completing on a boundary */
/*     will result in a filler byte of 0xFF appended at the end.       */
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
UI/Error  : 0x0F8E8 to 0x1013F  (0x0BE0 = 63720 bytes)
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
#define UI_OFFSET        0x0F8E8
#define BROMIDE_OFFSET   0x10D28
#define LOCATION_OFFSET  0x1109C
#define DICT_OFFSET      0x11290

/* # of listings for each type */
#define NUM_SPELL_ITEMS        137
#define NUM_ITEMS              346
#define NUM_UI_ITEMS           76
#define NUM_BROMIDE_ITEMS      55
#define NUM_LOCATION_ITEMS     31

/* Region sizes in bytes */
#define SPELL_SIZE             0xDEC
#define ITEM_SIZE              0x2324
#define UI_SIZE                0xBE0
#define BROMIDE_SIZE           0x370
#define LOCATION_SIZE          0x1F0

/* Extra Space in the file in bytes */
#define DICT_SIZE              0x1D48
#define EXTRA_SPACE_SIZE       0x1A0


/* Globals */

/* Function Prototypes */
int encodeSpells(int numSpells, char* inFileName, 
    unsigned char* outBuffer, unsigned int* outNumBytes);
int encodeItems(int numItems, char* inFileName, 
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
	char* filenames[] = {"_spell.bin","_items.bin","_ui.bin","_bromides.bin", "_locations.bin"};
	unsigned int offsets[] = {SPELL_OFFSET, ITEM_OFFSET, UI_OFFSET,
		                    BROMIDE_OFFSET, LOCATION_OFFSET, DICT_OFFSET};

	/* Open the output file for writing */
	outFile = fopen(outFileName,"wb");
	if(outFile == NULL){
		printf("Error occurred while opening %s for writing\n", outFileName);
		free(buffer);
        return -1;
    }

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

	/* Now overwrite with the replacement data */
	/* Spells, Items, UI/Error Msgs, Bromides, Locations */
	for(x = 0; x < 5; x++){
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
/* Example: 0x1234, 1, "" "" */
/* Output Format: */

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
	unsigned char outputVal;
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
		/* Encode the first text as 8-bit ascii - 0x1F, last character upper bit is set    */
		/* Encode the second text as 8-bit ascii - 0x1F, last character upper bit is set   */


        //BPE Compression
        if(str1Len > 0)
			compressBPE((unsigned char*)str1,(unsigned int*) &str1Len);
		if(str2Len > 0)
		    compressBPE((unsigned char*)str2,(unsigned int*) &str2Len);

		pTextPointers[txtIndex] = (unsigned short)(outputSizeBytes / 2) & 0xFFFF;
		swap16(&pTextPointers[txtIndex]);
		for(x = 0; x < str1Len; x++){
			outputVal = str1[x];// - 0x1F;
//			if(x == (str1Len-1))
//				outputVal |= 0x80;
			outBuffer[outputSizeBytes++] = outputVal;
		}
		for(x = 0; x < str2Len; x++){
			outputVal = str2[x];// - 0x1F;
//			if(x == (str2Len-1))
//				outputVal |= 0x80;
			outBuffer[outputSizeBytes++] = outputVal;
		}
		if((outputSizeBytes % 2) != 0)
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
	unsigned char outputVal;
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
			strncpy(str2,modifiedStr,numCharacters);
			str2Len = numCharacters;
		}

		/* Now Encode to the output file */
		/* Set the associated index to point to the next text location */
		/* Index stores the relative short word location from the beginning of the section */
		/* Each entry must be on a short word boundary.                                    */
		/* Encode the first text as 8-bit ascii - 0x1F, last character upper bit is set    */
		/* Encode the second text as 8-bit ascii - 0x1F, last character upper bit is set   */

		//BPE Compression
        if(str1Len > 0)
			compressBPE((unsigned char*)str1,(unsigned int*) &str1Len);
		if(str2Len > 0)
		    compressBPE((unsigned char*)str2,(unsigned int*) &str2Len);

		pTextPointers[txtIndex] = (unsigned short)(outputSizeBytes / 2) & 0xFFFF;
		swap16(&pTextPointers[txtIndex]);
		for(x = 0; x < str1Len; x++){
			outputVal = str1[x];// - 0x1F;
//			if(x == (str1Len-1))
//				outputVal |= 0x80;
			outBuffer[outputSizeBytes++] = outputVal;
		}
		outBuffer[outputSizeBytes++] = 0xFF;
		if((outputSizeBytes % 2) != 0)
			outBuffer[outputSizeBytes++] = 0xFF;

		for(x = 0; x < str2Len; x++){
			outputVal = str2[x];// - 0x1F;
//			if(x == (str2Len-1))
//				outputVal |= 0x80;
			outBuffer[outputSizeBytes++] = outputVal;
		}
		outBuffer[outputSizeBytes++] = 0xFF;
		if((outputSizeBytes % 2) != 0)
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
int encodeCmnStandard(int numSlots, char* inFileName, 
    unsigned char* outBuffer, unsigned int* outNumBytes){

	FILE* infile;
	unsigned char* pData, *pChar;
	unsigned short* pTextPointers;
	static char line[1024];
	static char str1[1024];
	static char modifiedStr[1024];
	unsigned char outputVal;
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
	numTextPointers = numSlots;
	memset(outBuffer,0x0,numTextPointers*2);
	pTextPointers = (unsigned short*)outBuffer;
	outputSizeBytes = numTextPointers*2;

    /* Parse the entries line by line */
    while(!feof(infile)){
		
		/* Read the line, make sure it starts with "0x" */
		/* Otherwise, just ignore it */
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
			strncpy(str1,modifiedStr,numCharacters);
			str1Len = numCharacters;
		}

		/* Now Encode to the output file */
		/* Set the associated index to point to the next text location */
		/* Index stores the relative short word location from the beginning of the section */
		/* Each entry must be on a short word boundary.                                    */
		/* Encode the first text as 8-bit ascii - 0x1F, last character upper bit is set    */
		/* Encode the second text as 8-bit ascii - 0x1F, last character upper bit is set   */

		//BPE Compression
        if(str1Len > 0)
			compressBPE((unsigned char*)str1,(unsigned int*) &str1Len);

		pTextPointers[txtIndex] = (unsigned short)(outputSizeBytes / 2) & 0xFFFF;
		swap16(&pTextPointers[txtIndex]);
		for(x = 0; x < str1Len; x++){
			outputVal = str1[x];// - 0x1F;
//			if(x == (str1Len-1))
//				outputVal |= 0x80;
			outBuffer[outputSizeBytes++] = outputVal;
		}
		outBuffer[outputSizeBytes++] = 0xFF;
		if((outputSizeBytes % 2) != 0)
			outBuffer[outputSizeBytes++] = 0xFF;

	}

    /* Update number of bytes written */
	*outNumBytes = outputSizeBytes;
	fclose(infile);
	return 0;
}


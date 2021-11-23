/*****************************************************************************/
/* bpe.exe - Byte Pair Encoding Routine for Saturn Lunar SSS/SSSM            */
/*           Compression will be limited to 1 level of encoding to reduce    */
/*           complexity on the Saturn side for decoding (in other words,     */
/*           if a new code is created to represent 2 characters, that new    */
/*           code will not then be merged with other characters/codes in     */
/*           subsequent passes).                                             */
/*                                                                           */
/*           Input: The program expects there to be several files            */
/*           "TEXTXXX.txt_dump.txt" in the same directory, where XXX is      */
/*            000-059.  The files should contain the text to be encoded,     */
/*            delimited by `                                                 */
/*                                                                           */
/*           Output: bpe.table, an encoding table for the files.             */
/*           Formatted as a 240 short word table.                            */
/*           If an entry is FFFF, it is not used.                            */
/*           If an entry starts with FF, then only the second byte is used   */
/*           Otherwise, both bytes are used for encoding.                    */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"
#include "text_list.h"


#define DELIMITER      "\""
#define NUM_CH_CODES   0xF0  /* 240 (0-239) */

#define NUM_INPUT_FILES 5
char* fileNameArray[] = {"spells.txt", "items.txt", "ui.txt", 
                         "bromides.txt", "locations.txt"};

/* Usage Struct for New/Existing Codes */
typedef struct chCodeRecord chCodeRecord;
struct chCodeRecord{
    unsigned int inuse;
    unsigned int numBytes;
    unsigned int encoded;
    unsigned char value[4];
	unsigned int hits;
};


/* For Statistics */
typedef struct bpeStats bpeStats;
struct bpeStats{
	unsigned int filenum;
	unsigned int fileStringSizeOrig;
	unsigned int fileStringSizeMod;
};


/* Globals */
chCodeRecord ch_codes[NUM_CH_CODES];
bpeStats* statArray;


/* Byte Pair Encoding Related Functions */
void decompressBPE(unsigned char* dst, unsigned char* src, unsigned int* nBytes);
void compressBPE(unsigned char* src, unsigned int* nBytes,
	unsigned char* newSeq, unsigned char newCode);




/*****************************************************************************/
/* Function:  Main Entry Point                                               */
/* Usage: lsb.exe file#1 [file#2 ... file#n]                                 */
/* Output: bpe.table, an encoding table for the files.                       */
/*****************************************************************************/
int main(int argc, char** argv){

    FILE* infile, *tablefile;
    char* fileData = NULL;
    int x = 0;
    int numFreeChCodes = 0;
	int numArg = 0;

	/* Parameter check */
	numArg = argc;
	if (numArg <= 1){
		printf("\n");
		printf("\tError in arguments.\n");
		printf("\tUsage: lsb.exe file#1 [file#2 ... file#n]\n");
		printf("\tEx: lsb.exe 1 2 3\n\n");

		return -1;
	}
	statArray = (bpeStats*)malloc(argc*sizeof(bpeStats));
	if (statArray < 0){
		printf("Error mallocing for parameters\n");
		return -1;
	}
	for (x = 1; x < numArg; x++){
		statArray[x-1].filenum = atoi(argv[x]);
		statArray[x - 1].fileStringSizeMod = 0;
		statArray[x - 1].fileStringSizeOrig = 0;
	}

	
    /********/
	/* Init */
    /********/
    
    /* Clear Used 8-Bit Character Codes */
    memset(ch_codes, 0, sizeof(chCodeRecord)*NUM_CH_CODES);

    /* Allocate Memory to Hold Text Dump Files */
    fileData = (char*) malloc(1024*1024);
	if(fileData == NULL){
		printf("Error, could not allocate file buffer\n");
		free(statArray);
		return -1;
	}

    /* Load UTF-8 Table */
    if(loadUTF8Table("l2_8bit_table.txt") < 0){
        printf("Error loading in utf-8 table\n");
		free(fileData);
		free(statArray);
        return -1;
    }

    /* Init Linked List for Text */
    if( initTextList() < 0){
        printf("Error initializing list for text strings.\n");
		free(fileData);
		free(statArray);
        return -1;
    }

    /***********************************************/
    /* Determine available characters for encoding */
    /* (Ones that are not currently in use)        */
    /* Also puts all text strings in memory.       */
    /***********************************************/
    for (x = 0; x < NUM_INPUT_FILES; x++){

		unsigned char* pch;
		int size;
		static char fname[300];

		sprintf(fname, "%s", fileNameArray[x]);

        /* Open File */
		printf("Opening %s\n", fname);
        infile = fopen(fname, "rb");
        if (infile == NULL){
            printf("Error, file %s does not exist.\n", fname);
            free(fileData);
			free(statArray);
            return -1;
        }

        /* Read File Into Memory */
        fseek(infile,0,SEEK_END);
        size = ftell(infile);
        if(size > (1024*1024)){
            printf("Error, file %s larger than 1MB, should not get here.\n", fname);
            free(fileData);
            fclose(infile);
			free(statArray);
            return -1;
        }
        fseek(infile,0,SEEK_SET);

		if (size <= 0){
			fclose(infile);
			continue;
		}
		memset(fileData, 0, 1024 * 1024);
        if(fread(fileData,1,size,infile) != size){
            printf("Error reading file %s into memory\n",fname);
                free(fileData);
            fclose(infile);
			free(statArray);
            return -1;
        }


        /* Tokenize */
        pch = (unsigned char*)strtok(fileData,DELIMITER);
        pch = (unsigned char*)strtok(NULL,DELIMITER);
        
        /* Tokenize & Store off each string.                   */
        /* Keep a note of detected 8-bit characters.           */
        /* And note if any symbols are missing from the table. */
        while(pch != NULL){
            int numCharacters;
            textNode* pNewNode;
            char* pChar;

            /* Create a Text Node, Copy in the Current UTF-8 String */
			if(createTextNode(&pNewNode) < 0){
                printf("Error occurred adding Text Node to Linked List.\n");
                free(fileData);
                destroyTextList();
                fclose(infile);
				free(statArray);
                return -1;
            }
			pNewNode->associatedFile = statArray[x].filenum;

            pNewNode->origNumBytes = strlen((char*)pch);
            pNewNode->origText = (unsigned char*)malloc(pNewNode->origNumBytes+1);
            if(pNewNode->origText == NULL){
                printf("Error allocing memory for original Text.\n");
                free(fileData);
                destroyTextList();
                fclose(infile);
				free(statArray);
                return -1;
            }
            memset(pNewNode->origText,0,pNewNode->origNumBytes+1);
            strcpy((char*)pNewNode->origText, (char*)pch);
                        
            /* Determine the number of UTF-8 Characters in the String  */
            /* Validate the character can be located in the Table File */
            /* Mark Characters as being used.                          */
            pChar = (char*)pch;
            numCharacters = 0;
            while(*pChar != '\0'){
                unsigned char utf8Code;
                int nBytes;
                nBytes = numBytesInUtf8Char(*pChar);
                if(getUTF8code_Byte(pChar, &utf8Code) < 0){
                    int y;
                    printf("Error, missing code for %d byte character!\n",nBytes);
                    for(y=0;y<nBytes;y++){
                        printf("\tByte %d: 0x%2X\n",y,pChar[y]);
                    }
                        free(fileData);
                    destroyTextList();
                    fclose(infile);
					free(statArray);
                    return -1;
                }

                /* Sanity Check */
				if (utf8Code >= NUM_CH_CODES){
                    printf("Error, utf8Code out of bounds.\n");
                        free(fileData);
                    destroyTextList();
                    fclose(infile);
					free(statArray);
                    return -1;
                }

                if(ch_codes[utf8Code].inuse != 1){
                    int y;
                    ch_codes[utf8Code].inuse = 1;
                    ch_codes[utf8Code].numBytes = nBytes;
                    for(y=0;y<nBytes;y++){
                        ch_codes[utf8Code].value[y] = pChar[y];
                    }
                }
                pChar += nBytes;
                numCharacters++;
            }


            /* Allocate memory for the modified string, which will be entirely 8-bit Codes */
            /* No NULL character required as this is no longer a text string */
            pNewNode->modNumBytes = numCharacters;
            pNewNode->modifiedText = (unsigned char*)malloc(pNewNode->modNumBytes+1);
            if(pNewNode->modifiedText == NULL){
                printf("Error allocing memory for modified Text.\n");
                free(fileData);
                destroyTextList();
                fclose(infile);
				free(statArray);
                return -1;
            }
            memset(pNewNode->modifiedText,0xFF,pNewNode->modNumBytes+1);

            /* Look up the Byte Code for the UTF-8 Characters in the String */
            /* Using the loaded 240 byte Table  */
            /* And store as the Modified String */
            pChar = (char*)pch;
            numCharacters = 0;
            while(*pChar != '\0'){
                unsigned char utf8Code;
                int nBytes;
                nBytes = numBytesInUtf8Char(*pChar);
                if(getUTF8code_Byte(pChar, &utf8Code) < 0){
                    int y;
                    printf("Error, missing code for %d byte character!\n",nBytes);
                    for(y=0;y<nBytes;y++){
                        printf("\tByte %d: 0x%2X\n",y,pChar[y]);
                    }
                        free(fileData);
                    destroyTextList();
                    fclose(infile);
					free(statArray);
                    return -1;
                }
                pNewNode->modifiedText[numCharacters] = utf8Code;

                pChar += nBytes;
                numCharacters++;
            }

            /* Add the Text Node to the linked list */
            if(addTextNode(pNewNode) < 0){
                printf("Error occurred adding Text Node to Linked List.\n");
                free(fileData);
                destroyTextList();
                fclose(infile);
				free(statArray);
                return -1;
            }

            /* Go to the next Text String */
            pch = (unsigned char*)strtok(NULL,DELIMITER);
			pch = (unsigned char*)strtok(NULL,DELIMITER);
        }

        fclose(infile);
    }
    free(fileData);


    /************************************************/
    /* Determine the number of remaining free codes */
    /************************************************/
    for (x = 0; x < 0xF0; x++){     //Needs to be less than 0xF0
        if (ch_codes[x].inuse == 0)
            numFreeChCodes++;
    }


    /**********************************************/
    /* Perform Encoding using the free characters */
    /**********************************************/
    while (numFreeChCodes > 0){
        
        unsigned char bestSequence[2];
        unsigned char newSequence[2];
        textNode* pCurrentNode = getHeadPtr();
        unsigned int offset = 0;
        unsigned int largestOccurence = 0;
        unsigned int occurence = 0;
		int a, b;

        /*************************************************/
        /* Determine the most frequent two byte sequence */
        /*************************************************/
		a = 0;
		while(pCurrentNode != NULL){

            if(offset < (pCurrentNode->modNumBytes-1)){
				int z;
				int skipCheck = 0;
                textNode* pSearch;
                newSequence[0] = pCurrentNode->modifiedText[offset];
                newSequence[1] = pCurrentNode->modifiedText[offset+1];
				occurence = 0;
				b = 0;

				/* Verify sequence not already located in table */
				for (z = 0; z < NUM_CH_CODES; z++){
					if ((ch_codes[z].inuse == 1) && (ch_codes[z].encoded == 1) &&
						(ch_codes[z].value[0] == newSequence[0]) && (ch_codes[z].value[1] == newSequence[1])){
						skipCheck = 1;
						break;
					}
				}
				if (skipCheck){
					offset++;
					continue;
				}

                /* Search all strings for the 2 byte sequence */
                pSearch = getHeadPtr();
                while(pSearch != NULL){
                    unsigned int srch_offset = 0;
                    while(srch_offset < (pSearch->modNumBytes-1)){
                        if(memcmp(newSequence, &pSearch->modifiedText[srch_offset],2) == 0){
							occurence++;
                            srch_offset += 2;
                        }
                        else{
                            srch_offset++;
                        }
                    }	
                    pSearch = pSearch->pNext;
					b++;
                }

				if (occurence > largestOccurence){
					largestOccurence = occurence;
					bestSequence[0] = newSequence[0];
					bestSequence[1] = newSequence[1];
				}

                offset++;
            }
            else{
                pCurrentNode = pCurrentNode->pNext;
				a++;
                offset = 0;
				occurence = 0;
            }
        }
		//Some indication of progress
		printf(".");

        /* If frequency <= 1, compression will not result, done */
        if(largestOccurence <= 1)
            break;

        /* Assign an available unused code to that sequence */
		for (x = 0; x < NUM_CH_CODES; x++){
            if (ch_codes[x].inuse == 0){
                ch_codes[x].inuse = 1;
                ch_codes[x].numBytes = 2;
                ch_codes[x].encoded = 1;
                ch_codes[x].value[0] = bestSequence[0];
                ch_codes[x].value[1] = bestSequence[1];
				ch_codes[x].hits = largestOccurence;
                numFreeChCodes--;
                break;
            }
        }


        /* Re-encode strings using the new sequence*/
        pCurrentNode = getHeadPtr();
        while(pCurrentNode != NULL){
			compressBPE(pCurrentNode->modifiedText, &pCurrentNode->modNumBytes,
				bestSequence, (unsigned char)x);
            pCurrentNode = pCurrentNode->pNext;
        }

    }
	printf("\n");

    /******************************************/
    /* Write New Table Encoding out to a File */
    /******************************************/
    tablefile = fopen("bpe.table", "wb");
    if (tablefile == NULL){
        printf("Error opening table file for outputting encoding table\n");
		free(statArray);
        return -1;
    }
	for (x = 0; x < NUM_CH_CODES; x++){
        unsigned char byte1;
        unsigned char byte2;

        if (ch_codes[x].inuse){
            if (ch_codes[x].encoded){
				byte1 = ch_codes[x].value[0];
				byte2 = ch_codes[x].value[1];
				printf("x = %2X, v1 = %2X v2 = %2X, hits = %u\n", x, ch_codes[x].value[0], ch_codes[x].value[1],ch_codes[x].hits);
            }
            else{
                byte1 = 0xFF;
                byte2 = ((unsigned char)x & 0xFF);
            }			
			fwrite(&byte1, 1, 1, tablefile);
            fwrite(&byte2, 1, 1, tablefile);
        }
		else{
			byte1 = 0xFF;
			byte2 = 0xFF;
			fwrite(&byte1, 1, 1, tablefile);
			fwrite(&byte2, 1, 1, tablefile);
		}
    }
    fclose(tablefile);



    /*****************************************/
    /* Summarize new encoding scheme results */
    /*****************************************/
    {
        textNode* pCurrentNode;
        int index, indexFound,y;
        float ratio,osize,msize;

        /* Get Stats */
        pCurrentNode = getHeadPtr();
        while(pCurrentNode != NULL){
            index = pCurrentNode->associatedFile;

			/* locate index */
			indexFound = 0;
			for(y = 0; y < (numArg-1); y++){
				if (statArray[y].filenum == index){
					indexFound = 1;
					break;
				}
			}

			if (!indexFound){
				printf("Program error, couldnt locate index %d for statistics.\n",index);
				continue;
			}

			statArray[y].fileStringSizeOrig += pCurrentNode->origNumBytes;
			statArray[y].fileStringSizeMod += pCurrentNode->modNumBytes;

            pCurrentNode = pCurrentNode->pNext;
        }

        /* Print Final Summary */
        for(x=0;x<(numArg-1);x++){

			osize = (float)statArray[x].fileStringSizeOrig / (float)1024.0;
			msize = (float)statArray[x].fileStringSizeMod / (float)1024.0;
			ratio = (msize / osize) * (float)100.0;

            printf(	"For File %d: \n"
                "\tOrigSize = %f kB\n"
                "\tModSize  = %f kB\n"
				"\tRatio    = %.2f%%\n", statArray[x].filenum, osize, msize, ratio);
        }
    }


	/****************/
	/* Decode again */
	/****************/
	{
		textNode* pCurrentNode;
		static char name[200];
		static char dest[2048];
		unsigned int nBytes;
		unsigned int currentFile;
		FILE* outfile;

		for (x = 0; x < (numArg - 1); x++){

			currentFile = statArray[x].filenum;

			sprintf(name, "verify_%u.txt", currentFile);
			outfile = fopen(name, "wb");
			if (outfile == NULL){
				printf("Error opening output file %s for decoded text.\n",name);
				free(statArray);
				return -1;
			}

			pCurrentNode = getHeadPtr();
			while (pCurrentNode != NULL){
				memset(dest, 0, 2048);
				nBytes = 0;

				/* Decompress for current file only */
				if (pCurrentNode->associatedFile == currentFile){
					decompressBPE((unsigned char*)dest, pCurrentNode->modifiedText, &nBytes);

					if (pCurrentNode->origNumBytes != nBytes){
						printf("Error, decoded numbytes (%u) != original (%u)\n", nBytes, pCurrentNode->origNumBytes);
					}

					if (strcmp((char*)pCurrentNode->origText, dest) != 0x0)
					{
						printf("Error, original string != decoded string.\n");
						printf("Orig: %s\n", pCurrentNode->origText);
						printf("Orig Bytes: %u\n", pCurrentNode->origNumBytes);
						printf("Decode: %s\n", dest);
						printf("Decode Bytes: %u", nBytes);
					}
					else
						fprintf(outfile, "%s`", dest);
				}

				pCurrentNode = pCurrentNode->pNext;
			}
			fclose(outfile);
		}
	}

    /* Free Resources */
    destroyTextList();
	free(statArray);
    
    return 0;
}


/**************************************************************/
/* compressBPE - Re-encodes a string using byte pair encoding */
/**************************************************************/
void compressBPE(unsigned char* src, unsigned int* nBytes, 
	unsigned char* newSeq, unsigned char newCode){

	int y;
	unsigned int srch_offset = 0;

	while (srch_offset < ((*nBytes) - 1)){
		if (memcmp(newSeq, &src[srch_offset], 2) == 0){
			src[srch_offset] = newCode;
			for (y = (int)(srch_offset + 1); y < (int)((*nBytes) - 1); y++){
				src[y] = src[y + 1];
			}
			(*nBytes)--;
			src[(*nBytes)] = 0xFF;
			srch_offset = 0;
		}
		else{
			srch_offset++;
		}
	}

	return;
}


//#define LKUP 0x00220000


/******************************************************************/
/* decompressBPE - Decompresses a string using byte pair encoding */
/******************************************************************/
void decompressBPE(unsigned char* dst, unsigned char* src, unsigned int* nBytes){

	unsigned int x;
//	unsigned short value;

	/* Base Case */
	if (*src >= 0xF0)
		return;

	/* Loop */
	while (*src < 0xF0){
		int byte1, byte2;

		/* Read Next Byte Value from table */
//		value = *(LKUP + *src);
//		byte1 = value >> 8;
//		byte2 = value & 0xFF;
		byte1 = ch_codes[*src].value[0];
		byte2 = ch_codes[*src].value[1];

		/* One character case */
		if (!ch_codes[*src].encoded){
			for (x = 0; x < ch_codes[*src].numBytes; x++){
				*dst = ch_codes[*src].value[x];
				dst++;
				(*nBytes)++;
			}
		}
		else{
			/* Two Byte Encoded Case */
			unsigned int nEncBytes1 = 0;
			unsigned int nEncBytes2 = 0;
			//unsigned int encodedVal1 = (*(LKUP + byte1) << 16) | 0xFFFF;
			//unsigned int encodedVal2 = (*(LKUP + byte2) << 16) | 0xFFFF;
			unsigned char encodedVal1[10];
			unsigned char encodedVal2[10];

			memset(encodedVal1, 0xFF, 10);
			memset(encodedVal2, 0xFF, 10);

			if (!ch_codes[byte1].encoded){
				for (x = 0; x < ch_codes[byte1].numBytes; x++){
					encodedVal1[x] = ch_codes[byte1].value[x];
				}
			}
			else{
				encodedVal1[0] = ch_codes[byte1].value[0];
				encodedVal1[1] = ch_codes[byte1].value[1];
			}

			if (!ch_codes[byte2].encoded){
				for (x = 0; x < ch_codes[byte2].numBytes; x++){
					encodedVal2[x] = ch_codes[byte2].value[x];
				}
			}
			else{
				encodedVal2[0] = ch_codes[byte2].value[0];
				encodedVal2[1] = ch_codes[byte2].value[1];
			}

			/* Recurse */
			decompressBPE(dst, &encodedVal1[0], &nEncBytes1);
			dst += nEncBytes1;
			*nBytes += nEncBytes1;
			decompressBPE(dst, &encodedVal2[0], &nEncBytes2);
			dst += nEncBytes2;
			*nBytes += nEncBytes2;
		}

		src++;
	}

	return;
}

/******************************************************************************/
/* main.c - Main execution file for preliminary Lunar 2 Common Text Decoder   */
/*          l2_cmntxt_decode.exe                                              */
/*          For use with Sega Saturn Lunar 2 Common File only.                */
/******************************************************************************/

/***********************************************************************/
/* Lunar 2 Common Text Decoder (l2_cmntxt_decode.exe) Usage            */
/* =========================================================           */
/* l2_txt_decode.exe decode InputFname                                 */
/*                                                                     */
/* InputFname should be the 96kB common text file (File #1233).        */
/*                                                                     */
/* This exe does not decode the dialog "ES" file text                  */
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
#define VER_MIN    1

#define MAX_BUF_SIZE (1024*1024)  /* 1MB, overkill, file is 96kB */

/* File Definitions */
#define DICT_LKUP_OFFSET 	0x11290
#define ITEM_START_OFFSET 	0xD5C0
#define NUM_ITEMS			338

/******************************************************************************/
/* printUsage() - Display command line usage of this program.                 */
/******************************************************************************/
void printUsage(){
    printf("Error in input arguments.\n");
    printf("Usage:\n===============================\n");
    printf("l2_cmntxt_decode.exe InputFname\n");
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
	unsigned int currentOffset;

    printf("Lunar2 Common Text Decoder v%d.%02d\n", VER_MAJ, VER_MIN);

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
	strcpy(inFileName,argv[1]);
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
    inFile = fopen(inFileName, "rb");
    if (inFile == NULL){
        printf("Error occurred while opening input script %s for reading\n", inFileName);
        return -1;
    }
    outFile = fopen(outFileName, "wb");
    if (outFile == NULL){
        printf("Error occurred while opening output file %s for writing\n", outFileName);
        fclose(inFile);
        return -1;
    }


    /**************************************************************************/
    /* Parse the Input Script File (Req'd for Encoding to Binary or Updating) */
    /**************************************************************************/
    printf("Parsing input file.\n");
	
	/* Read the whole thing into memory */
	buffer = (char*)malloc(MAX_BUF_SIZE);
	if(buffer == NULL){
		printf("Error allocating memory for file.\n");
		return -1;
	}
	numRead = fread(buffer,1,MAX_BUF_SIZE,inFile);
	if(numRead <= 0){
		printf("Error in reading from the file occurred.\n");
		free(buffer);
		return -1;
	}

	/* Spells are from 0xC7C4 to 0xD5BA.  These are all 2 byte lookups.  Terminate in 0xFFFF */
	currentOffset = 0xC7C4;
    #define SPELL_STRING_START 0xC7C4
	#define SPELL_STRING_END   0xD5BA
	printf("Decoding Spell Strings\n");
	fprintf(outFile,"Decoding Spell Strings\n");
	
	currentOffset = SPELL_STRING_START;
	while(currentOffset < SPELL_STRING_END){
		unsigned short* pData;
		unsigned short tmpShort = 0;
		unsigned short tmpIndex = 0;
		int indexLoc,z;
		
		/* Print Offset */
		fprintf(outFile,"0x%06X\t",currentOffset);
		tmpShort = 0;
		while(tmpShort != 0xFFFF){
			memcpy(&tmpIndex,&buffer[currentOffset],2);
			swap16(&tmpIndex);
			indexLoc = tmpIndex*4 + DICT_LKUP_OFFSET;
			pData = (unsigned short*)&buffer[indexLoc];
			for(z=0;z<2;z++){
				memcpy((char*)&tmpShort,&pData[z],2);
				swap16(&tmpShort);
				if((tmpShort & 0xF000) == 0x9000){
					printf(" ");
					fprintf(outFile," ");
				}
				else if(tmpShort < 0x1000){
					memset(data,0,10);
					if(getUTF8character((int)tmpShort, data) >= 0){
						printf("%s",data);
						fprintf(outFile,"%s",data);
					}
					else{
						printf("\nError printing 0x%X\n",(int)tmpShort&0xFFFF);
						fprintf(outFile,"\nError printing 0x%X\n",(int)tmpShort&0xFFFF);
					}
				}
				else{
					printf("<0x%4X>",tmpShort);
					fprintf(outFile,"<0x%4X>",tmpShort);
				}
			}
			currentOffset+=2;
		}
		printf("\n");
		fprintf(outFile,"\n");
	}
	
	fprintf(outFile,"\n\n\n\n");
	
	
	/* Item Name/Descriptions 
	  ======================
	  Start at Offset 0xD5C0 (0x2855C0-0x278000=0xD5C0)
	  END BY 0xF7FA
	  Each entry is 13 short words (5 for name, 8 for desc)
      Each short word is an index value to 2 consecutive short words located at:
 	       0x0001_1290+(index_val*4)  <-- (0x289290-0x278000=0x11290)
      Those short words map directly to the font table
      A short word entry of 0x0000 maps to blank space.
      This means names can be 10 characters, and descriptions can be 16 characters.
      It turns out only 15 characters are visible for descriptions though due to available screen space.
	*/
	

	
#if 1
	printf("Decoding Items\n");
	fprintf(outFile,"Decoding Items\n");
	fprintf(outFile,"Offset\tItem\tDescription\n");

	currentOffset = ITEM_START_OFFSET;
	for(x=0; x < NUM_ITEMS; x++){
		
		unsigned short itemNameRLE[10];
		unsigned short itemDescRLE[16];
		unsigned short* pData;
		int indexLoc,z;
		unsigned short tmpShort = 0;
		
		fprintf(outFile,"0x%06X\t",currentOffset);
		
		/* Read 5 SW For Item Name */
		memcpy(itemNameRLE,&buffer[currentOffset],10);
		for(y=0;y<5;y++)
			swap16(&itemNameRLE[y]);
		currentOffset += 10;

		/* Read 8 SW For Item Desc */
		memcpy(itemDescRLE,&buffer[currentOffset],16);
		for(y=0;y<8;y++)
			swap16(&itemDescRLE[y]);
		currentOffset += 16;
		
		/* Decode Item Name */
		for(y=0; y<5;y++){
			indexLoc = itemNameRLE[y]*4 + DICT_LKUP_OFFSET;
			pData = (unsigned short*)&buffer[indexLoc];
			for(z=0;z<2;z++){
				memcpy(&tmpShort,&pData[z],2);
				swap16(&tmpShort);
				if((tmpShort & 0xF000) == 0x9000){
					printf(" ");
					fprintf(outFile," ");
				}
				else if(tmpShort < 0x1000){
					memset(data,0,10);
					if(getUTF8character((int)tmpShort, data) >= 0){
						printf("%s",data);
						fprintf(outFile,"%s",data);
					}
					else{
						printf("\nError printing 0x%X\n",(int)tmpShort&0xFFFF);
						fprintf(outFile,"\nError printing 0x%X\n",(int)tmpShort&0xFFFF);
					}
				}
				else{
					printf("<0x%4X>",tmpShort);
					fprintf(outFile,"<0x%4X>",tmpShort);
				}
			}
		}
		
		/* Tab between item and description */
		fprintf(outFile,"\t");
		
		/* Decode Item Description */
		for(y=0; y<8;y++){
			indexLoc = itemDescRLE[y]*4 + DICT_LKUP_OFFSET;
			pData = (unsigned short*)&buffer[indexLoc];
			for(z=0;z<2;z++){
				memcpy(&tmpShort,&pData[z],2);
				swap16(&tmpShort);
				if((tmpShort & 0xF000) == 0x9000){
					printf(" ");
					fprintf(outFile," ");
				}
				else if(tmpShort < 0x1000){
					memset(data,0,10);
					if(getUTF8character((int)tmpShort, data) >= 0){
						printf("%s",data);
						fprintf(outFile,"%s",data);
					}
					else{
						printf("\nError printing 0x%X\n",(int)tmpShort&0xFFFF);
						fprintf(outFile,"\nError printing 0x%X\n",(int)tmpShort&0xFFFF);
					}
				}
				else{
					printf("<0x%4X>",tmpShort);
					fprintf(outFile,"<0x%4X>",tmpShort);
				}
			}
		}
		
		/* Newline before next entry */
		fprintf(outFile,"\n");
	}
	fprintf(outFile,"\n\n\n\n");
#endif	

/***********************************************************/
/* Note - no idea what is stored between 0xF7FA and 0x103C4*/
/***********************************************************/


#if 1
	/*
	Common Strings Notes
    text stored in 16-bit shorts that map to text.  0x14 short words or ends early in 0xFFFF
	*/
	#define CMN_STRING_START 0x103C4
	#define CMN_STRING_END   0x10D24
	#define CMN_STRING_SIZE  0x14
	printf("Decoding Common Strings\n");
	fprintf(outFile,"Decoding Common Strings\n");
	
	currentOffset = CMN_STRING_START;
	while(currentOffset <= CMN_STRING_END){
		unsigned short* ptrString;
		unsigned short tmpShort;
		
		fprintf(outFile,"0x%06X\t",currentOffset);
		
		for(x = 0; x < CMN_STRING_SIZE;x++){
			ptrString = (unsigned short*)&buffer[currentOffset];
			tmpShort = *(unsigned short*)ptrString;
			swap16(&tmpShort);
		
			if(tmpShort == 0xFFFF){
				printf("<0xFFFF>");
				fprintf(outFile,"<0xFFFF>");
			}
			else if((tmpShort &0xF000) == 0x9000){
				printf(" ");
				fprintf(outFile," ");
			}
			else if(tmpShort < 0x1000){
				memset(data,0,10);
				if(getUTF8character((int)tmpShort, data) >= 0){
					printf("%s",data);
					fprintf(outFile,"%s",data);
				}
				else{
					printf("\nError printing 0x%X\n",tmpShort&0xFFFF);
					fprintf(outFile,"\nError printing 0x%X\n",tmpShort&0xFFFF);
				}
			}
			else{
				printf("<0x%4X>",tmpShort);
				fprintf(outFile,"<0x%4X>",tmpShort);
			}
			
			currentOffset += 2;
		}
		
		/* Newline before next entry */
		fprintf(outFile,"\n");
	}
#endif






#if 1
	/* Decoding of stuff from 0x10D24 through 0x11290 (start of 2 byte dictionary lookup table) */
	printf("Decoding Other\n");
	fprintf(outFile,"Decoding Other\n");
	fprintf(outFile,"Offset\tItem\tDescription\n");

	currentOffset = 0x10D24+4;
	while(currentOffset < 0x11290){
		
		unsigned short itemDescRLE[16];
		unsigned short* pData;
		int indexLoc,z;
		unsigned short tmpShort = 0;
		
		fprintf(outFile,"0x%06X\t",currentOffset);

		/* Read 8 SW For Item Desc */
		memcpy(itemDescRLE,&buffer[currentOffset],16);
		for(y=0;y<8;y++)
			swap16(&itemDescRLE[y]);
		currentOffset += 16;
		
		/* Decode Description */
		for(y=0; y<8;y++){
			indexLoc = itemDescRLE[y]*4 + DICT_LKUP_OFFSET;
			pData = (unsigned short*)&buffer[indexLoc];
			for(z=0;z<2;z++){
				memcpy(&tmpShort,&pData[z],2);
				swap16(&tmpShort);
				if((tmpShort & 0xF000) == 0x9000){
					printf(" ");
					fprintf(outFile," ");
				}
				else if(tmpShort < 0x1000){
					memset(data,0,10);
					if(getUTF8character((int)tmpShort, data) >= 0){
						printf("%s",data);
						fprintf(outFile,"%s",data);
					}
					else{
						printf("\nError printing 0x%X\n",(int)tmpShort&0xFFFF);
						fprintf(outFile,"\nError printing 0x%X\n",(int)tmpShort&0xFFFF);
					}
				}
				else{
					printf("<0x%4X>",tmpShort);
					fprintf(outFile,"<0x%4X>",tmpShort);
				}
			}
		}
		
		/* Newline before next entry */
		fprintf(outFile,"\n");
	}
	fprintf(outFile,"\n\n\n\n");
#endif	


/* More strings between 0xF900 and 103C4*/

#if 1
	/*
	Common Strings Notes
    Definitely end by 0x0001_1290
    text stored in 16-bit shorts that map to text.  0x14 short words or ends early in 0xFFFF
	*/
	#define CMN_STRING2_START 0xF900-4
	#define CMN_STRING2_END   0x0103C4 //17E00 //0x11290
	#define CMN_STRING2_SIZE  0x14
	printf("Decoding Common Strings 2\n");
	fprintf(outFile,"Decoding Common Strings 2\n");
	
	currentOffset = CMN_STRING2_START;
	while(currentOffset <= CMN_STRING2_END){
		unsigned short* ptrString;
		unsigned short tmpShort;
		
		fprintf(outFile,"0x%06X\t",currentOffset);
		
		for(x = 0; x < CMN_STRING2_SIZE;x++){
			ptrString = (unsigned short*)&buffer[currentOffset];
			tmpShort = *(unsigned short*)ptrString;
			swap16(&tmpShort);
		
			if(tmpShort == 0xFFFF){
				printf("<0xFFFF>");
				fprintf(outFile,"<0xFFFF>");
			}
			else if((tmpShort &0xF000) == 0x9000){
				printf(" ");
				fprintf(outFile," ");
			}
			else if(tmpShort < 0x1000){
				memset(data,0,10);
				if(getUTF8character((int)tmpShort, data) >= 0){
					printf("%s",data);
					fprintf(outFile,"%s",data);
				}
				else{
					printf("\nError printing 0x%X\n",tmpShort&0xFFFF);
					fprintf(outFile,"\nError printing 0x%X\n",tmpShort&0xFFFF);
				}
			}
			else{
				printf("<0x%4X>",tmpShort);
				fprintf(outFile,"<0x%4X>",tmpShort);
			}
			
			currentOffset += 2;
		}
		
		/* Newline before next entry */
		fprintf(outFile,"\n");
	}
#endif




#if 0
	/* Spells are from 0xC7C4 to 0xD5BA.  These are all 2 byte lookups.  Terminate in 0xFFFF */
    #define CMN_STRING_START 0xF900
	#define CMN_STRING_END   0x11290
	printf("Decoding Common Strings\n");
	fprintf(outFile,"Decoding Common Strings\n");
	
	currentOffset = CMN_STRING_START;
	while(currentOffset < CMN_STRING_END){
		unsigned short* pData;
		unsigned short tmpShort = 0;
		unsigned short tmpIndex = 0;
		int indexLoc,z,cntr;
		
		/* Print Offset */
		fprintf(outFile,"0x%06X\t",currentOffset);
		tmpShort = 0;
		cntr = 0;

		while(tmpShort != 0xFFFF){
			memcpy(&tmpIndex,&buffer[currentOffset],2);
			swap16(&tmpIndex);
			indexLoc = tmpIndex*4 + DICT_LKUP_OFFSET;
			pData = (unsigned short*)&buffer[indexLoc];
			for(z=0;z<2;z++){
				memcpy((char*)&tmpShort,&pData[z],2);
				swap16(&tmpShort);
				if((tmpShort & 0xF000) == 0x9000){
					printf(" ");
					fprintf(outFile," ");
				}
				else if(tmpShort < 0x1000){
					memset(data,0,10);
					if(getUTF8character((int)tmpShort, data) >= 0){
						printf("%s",data);
						fprintf(outFile,"%s",data);
					}
					else{
						printf("\nError printing 0x%X\n",(int)tmpShort&0xFFFF);
						fprintf(outFile,"\nError printing 0x%X\n",(int)tmpShort&0xFFFF);
					}
				}
				else{
					printf("<0x%4X>",tmpShort);
					fprintf(outFile,"<0x%4X>",tmpShort);
				}
				cntr++;
			}
			currentOffset+=2;
			
			if(cntr >= (0x14*2))
				break;
		}
		printf("\n");
		fprintf(outFile,"\n");
	}
#endif
	fprintf(outFile,"\n\n\n\n");
	


    
    /* Close files */
    fclose(inFile);
	fclose(outFile);

    return 0;
}

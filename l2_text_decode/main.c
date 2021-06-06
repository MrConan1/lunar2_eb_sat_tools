/******************************************************************************/
/* main.c - Main execution file for preliminary Lunar 2 Text Decoder          */
/*          l2_txt_decode.exe                                                 */
/*          For use with Sega Saturn Lunar 2 Files only.                      */
/******************************************************************************/

/***********************************************************************/
/* Lunar 2 Text Decoder (l2_txt_decode.exe) Usage                      */
/* ===============================================                     */
/* l2_txt_decode.exe decode InputFname [offset]                        */
/*                                                                     */
/* InputFname should be a file with an "ES" header somewhere inside of */
/* it (Search for 0x45530001; 0x4553 is "ES").  For some files this    */
/* header is at the beginning.  Others have it buried in the file.     */
/*                                                                     */
/* This exe does not decode the common item/battle text, just dialog   */
/* text.                                                               */
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

/******************************************************************************/
/* printUsage() - Display command line usage of this program.                 */
/******************************************************************************/
void printUsage(){
    printf("Error in input arguments.\n");
    printf("Usage:\n===============================\n");
    printf("l2_txt_decode.exe InputFname [offset]\n");
    printf("\n\n");
    return;
}


/******************************************************************************/
/* main()                                                                     */
/******************************************************************************/
int main(int argc, char** argv){

    FILE *inFile, *outFile/*, *upFile, *csvOutFile, *txtOutFile*/;
    static char inFileName[300];
    static char outFileName[300];
    int rval, ienc, oenc, file_start_offset;
	unsigned int textOffset, portraitOffset, currentOffset;
    rval = ienc = oenc = -1;
	file_start_offset = 0;

    printf("Lunar2 Text Decoder v%d.%02d\n", VER_MAJ, VER_MIN);

    /**************************/
    /* Check input parameters */
    /**************************/

    /* Check for valid # of args */
    if (argc < 2){
        printUsage();
        return -1;
    }
	else if(argc == 3){
		file_start_offset = atoi(argv[2]);
	}
	else if(argc > 3){
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
	
	/* Read the offset to start of the Text Section */
	fseek(inFile,file_start_offset+0x1C,SEEK_SET);
	fread(&textOffset,1,4,inFile);
	swap32(&textOffset);
	
	/* Read the offset to the start of the Portrait Section */
	fseek(inFile,file_start_offset+0x4,SEEK_SET);
	fread(&portraitOffset,1,4,inFile);
	swap32(&portraitOffset);
	
	/* Jump to the start of the Text Section */
	fseek(inFile,file_start_offset+textOffset,SEEK_SET);
	currentOffset = textOffset;	
	
#if 0
	/* hack - I turned this on and it looks like if */
	/* portraitOffset <= currentOffset, there is no text section */
	if(portraitOffset <= currentOffset)
		portraitOffset = 0x7FFFFFFF;
#endif
	
	/* Loop until end of text section reached */
	/* Dump the text as we go along.          */
	while(currentOffset < portraitOffset){
		static char tmp[1024];
		unsigned short dlogWord,dlogSizeBytes;
		char data[10];
		int x;
		
		/* Read size of the next dialog */
		fread(&dlogSizeBytes,1,2,inFile);
		if(feof(inFile)){
			printf("File EOF Detected.\n");
			break;
		}
		swap16(&dlogSizeBytes);
		
		/* It appears a size of 0x00 bytes can also signify the end */
		if(dlogSizeBytes == 0x00)
		{
			printf("Detected size of 0x00, end of Dlog Text.\n");
			break;
		}

		/* Verify we wont spill into the portrait section */
		if((currentOffset + dlogSizeBytes) > portraitOffset){
			printf("Error, dialog will spill into portrait section!\n");
			break;
		}
		dlogSizeBytes-=2;
		currentOffset+=2;
		
		printf("\n\nStart DLOG\n==========\n");
		fprintf(outFile,"\n\nStart DLOG\n==========\n");
				
		/* Read and dump dialog contents */
		for(x = 0; x < dlogSizeBytes; x+=2){
		
			/* Read the next control code or font character */
			fread(&dlogWord,1,2,inFile);
			if(feof(inFile)){
				printf("EOF Detected.\n");
				break;
			}
			swap16(&dlogWord);
			
			/******************************/
			/* Print Text or Control Code */
			/******************************/

/*			
			Control Codes
			=============
			0x1000 <-- Moves text to next line. Control code 0x1000 3x in a row ends dialog with push button.
			0x2xxx <-- hit a button, when hit clears the box and keeps on printing
			0x3xxx <-- prints the left half of character 0xXXX
			0x4xxx <-- nothing is printed or performed
			0x5xxx <-- Dialog Selection.  If lowest byte is 2, there are 2 selections.  If lowest byte is 3 there are 3 selections.
					   Selections are just 16-bit text, each ending in 0x1000.
			0x6000 <-- hit a button, when hit continue printing
			0x7xxx <-- hit a button, when hit clears the box and keeps on printing
			0x8xxx <-- hit a button, when hit clears the box and keeps on printing
			0x9xxx <-- space printed.  Usually 9010, but since theres no vwf, i think it doesnt matter
			0xBxxx <-- Prints something about items but you dont get anything?
			0xCxxx <-- Display Left portrait
			0xDxxx <-- Animations? D001 is lucias entrance
			0xFxxx <-- Displays more left portraits
*/			
			
			/* Font Character */
			if(dlogWord < 0x1000){
				memset(data,0,10);
				if(getUTF8character((int)dlogWord, data) >= 0){
					printf("%s",data);
					fprintf(outFile,"%s",data);
				}
			}
			else{
				/* Control Word */
				unsigned int dlogLW = (unsigned int)dlogWord;
				unsigned int identifier = (dlogWord >> 12) & 0xF;
				switch(identifier){
					
					case 1:
						sprintf(tmp,"<newline>\n");
					break;
					case 2:
					case 7:
					case 8:
						sprintf(tmp,"<press_button,auto-clear box>\n");
					break;
					case 3:
					{
						memset(data,0,10);
						if(getUTF8character((int)(dlogLW&0xFFF), data) >= 0){
							sprintf(tmp,"<Print left half: %s>",data);
						}
						else
							sprintf(tmp,"<Print left half: 0x%X>",dlogLW&0xFFF);
					}
					break;					
					case 4:
						sprintf(tmp,"<does_nothing>");
					break;
					case 5:
					{
						sprintf(tmp,"<Dlog Selection (%d)>\n", dlogLW&0xF);
						
						/* Read # of selections */
						
						/* Go back to main loop to Print out the selections */
						/* Each should end in 0x1000 */
					}	
					break;
					case 6:
						sprintf(tmp,"<press_button,continue printing>\n");
					break;
					case 9:
						sprintf(tmp," ");
					break;	
					case 0xA:
					case 0xE:
						sprintf(tmp,"<Unknown(0x%X)>",dlogLW);
					break;					
					case 0xB:
						sprintf(tmp,"<Item_Print 0x%X>",dlogLW);
					break;
					case 0xC:
					case 0xF:
						sprintf(tmp,"<Left Portrait 0x%X>",dlogLW);
					break;
					case 0xD:
						sprintf(tmp,"<Animation?>");
					break;

					default:
						sprintf(tmp,"<ERROR: 0x%X>",dlogLW);
					break;					
				}
				fprintf(outFile,tmp);
				printf(tmp);
			}
			
			
		}
		currentOffset += dlogSizeBytes;
	}
    
    /* Close files */
    fclose(inFile);
	fclose(outFile);

    return 0;
}

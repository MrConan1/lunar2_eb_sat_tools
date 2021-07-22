/******************************************************************************/
/* main.c - Main execution file for Lunar 2 Script Decoder                    */
/*          l2_decode.exe                                                     */
/*          For use with Sega Saturn & PSX Lunar 2 Files only.                */
/******************************************************************************/

/***********************************************************************/
/* Lunar 2 Text Decoder (l2_txt_decode.exe) Usage                      */
/* ==================================================                  */
/* l2_decode.exe decode InputFname [offset]                            */
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
/*       JP PSX output will be in shift-jis.  US PSX in ASCII.  JP SAT */
/*       will be in UTF-8 format.                                      */
/***********************************************************************/


/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"


#define VER_MAJ    1
#define VER_MIN    1

#define SAT_US	0
#define PSX_JP	1
#define PSX_US	2

#undef PRINT_CMDS

int dlgCmdLocation[1000];
int dlogCmdOffset[1000];
int dlgCmdLink[1000];

/***********************/
/* Function Prototypes */
/***********************/
void printUsage();
void decodePsx_CtrlCode(int type, char* tmp, FILE* outfile,
                        unsigned int identifier, unsigned int dlogLW);
void decodeJPSat_CtrlCode(char* tmp, FILE* outfile,
                          unsigned int identifier, unsigned int dlogLW);


/******************************************************************************/
/* printUsage() - Display command line usage of this program.                 */
/******************************************************************************/
void printUsage(){
    printf("Error in input arguments.\n");
    printf("Usage:\n===============================\n");
    printf("l2_decode.exe InputFname type offset\n");
	printf("type = 0 for jp saturn, type = 1 for jp psx, 2 for us psx\n");
    printf("\n\n");
    return;
}


/******************************************************************************/
/* main()                                                                     */
/******************************************************************************/
int main(int argc, char** argv){

    FILE *inFile, *outFile;
	char* pFname;
	static char tmp[1024];
    static char inFileName[300];
    static char outFileName[300];
    int rval, ienc, oenc, x, file_start_offset,type,dialogNum,numTextPointers;
	unsigned int textOffset, portraitOffset, currentOffset;
    rval = ienc = oenc = type = -1;
	file_start_offset = 0;
	dialogNum = 1;

    printf("Lunar2 Decoder v%d.%02d\n", VER_MAJ, VER_MIN);

    /**************************/
    /* Check input parameters */
    /**************************/

    /* Check for valid # of args */
    if (argc != 4){
        printUsage();
        return -1;
    }
	type = atoi(argv[2]);
	file_start_offset = atoi(argv[3]);

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
    printf("Parsing input file %s.\n",inFileName);
	
	
	/*****************************/
	/* Write Initial File Header */
	/*****************************/
	fprintf(outFile,"<File_Info>\n");
	pFname = &inFileName[strlen(inFileName)-1];
	while(pFname != inFileName){
		pFname--;
		if((*pFname == '\\') || (*pFname == '/')){
			pFname++;
			break;
		}
	}
	fprintf(outFile,"Name: %s\n",pFname);
	fprintf(outFile,"Script_Offset: 0x%X\n",file_start_offset);
	fprintf(outFile,"</File_Info>\n\n");
	
	
	
	/****************************/
	/* Create ES Header Section */
	/****************************/
	
	/* Read the offset to start of the Text Section */
	fseek(inFile,file_start_offset+0x1C,SEEK_SET);
	fread(&textOffset,1,4,inFile);
	if(!type)
		swap32(&textOffset);

	/* Read the offset to the start of the Portrait Section */
	fseek(inFile,file_start_offset+0x4,SEEK_SET);
	fread(&portraitOffset,1,4,inFile);
	if(!type)
		swap32(&portraitOffset);

	
	fprintf(outFile,"<ES_Header>\n");
	fprintf(outFile,"Command_Offset: 0x%X\n",0x20);
	fprintf(outFile,"Text_Offset: 0x%X\n",textOffset);
    fprintf(outFile,"Portrait_Offset: 0x%X\n",portraitOffset);
    fprintf(outFile,"Text_Size: 0x%X\n",(portraitOffset-textOffset+1));
	fprintf(outFile,"</ES_Header>\n\n");
	

	/**************************/
	/* Create Command Section */
	/**************************/	
	fprintf(outFile,"<Command_Section>\n");
	fseek(inFile,file_start_offset+0x20,SEEK_SET);
	numTextPointers = 0;
	while(ftell(inFile) < (textOffset+file_start_offset)){
		unsigned int cmd, location;
		int rval;
		
		location = ftell(inFile) - file_start_offset;
		rval = fread(&cmd,4,1,inFile);
		if(rval != 1){
			printf("Command Header Read error\n");
			break;
		}
		if(!type)
			swap32(&cmd);
		
		/* Dialog Command that points to the text section */
		if(((cmd & 0xFF000000) >> 24) == 0x1D){
			
			dlgCmdLocation[numTextPointers] = location;
			dlogCmdOffset[numTextPointers] = (cmd & 0x00FFFFFF)+textOffset;
    		numTextPointers++;
#ifdef PRINT_CMDS
			fprintf(outFile,"Cmd: 0x1Dxxxxxx\n");
#endif
		}
#ifdef PRINT_CMDS
		else
			fprintf(outFile,"Cmd: 0x%08X\n",cmd);
#endif
	}
	fprintf(outFile,"\n");
	fprintf(outFile,"Num_Text_Pointers: 0x%X\n",numTextPointers);
	
	/*****************************************/
	/* Create Command Section - Text Offsets */
	/*****************************************/
	for(x = 0; x < numTextPointers; x++){
        sprintf(tmp,"\tText_Pointer_%04d: ScriptOffset 0x%08X DlgTextOffset 0x%08X\n",
		    x+1,dlgCmdLocation[x],dlogCmdOffset[x]);
		fprintf(outFile,tmp);
		printf(tmp);
		dlgCmdLink[x] = 0; /* init */
	}
	fprintf(outFile,"</Command_Section>\n\n");
	
	
	/***********************/
	/* Create Text Section */
	/***********************/		
	fprintf(outFile,"<Text_Section>\n");


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
		int x, textOutputActive, offset, numLinksFound;
		textOutputActive = 0;
		
		/* Read size of the next dialog */
		offset = ftell(inFile) - file_start_offset;
		fread(&dlogSizeBytes,1,2,inFile);
		if(feof(inFile)){
			printf("File EOF Detected.\n");
			break;
		}
		if(!type)
			swap16(&dlogSizeBytes);
//printf("dlogSizeBytes = 0x%X\n",dlogSizeBytes);	
		
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
		
		printf("\n\nStart DLOG %d\n==============\n",dialogNum);
		//fprintf(outFile,"\n\nStart DLOG %d\n==============\n",dialogNum);
		
		
		fprintf(outFile,"Label: Text %d\n",dialogNum);
        fprintf(outFile,"Offset: 0x%X\n",offset);
        fprintf(outFile,"Size: 0x%X bytes\n",dlogSizeBytes);
		
		/* Verify at least 1 link to this section */
		numLinksFound = 0;
		for(x = 0; x < numTextPointers; x++){
			
			if(dlogCmdOffset[x] == offset){		
    			dlgCmdLink[x] = 1;
				numLinksFound++;
			}
		}
		if(numLinksFound < 1){
			fprintf(outFile,"Error, missing Cmd pointer to text section 0x%X\n",offset);
		}
		else if(numLinksFound < 1){
			fprintf(outFile,"Warning, multiple Cmd pointers to text section 0x%X\n",offset);
		}
//        fprintf(outFile,"Cmd_Links: 0xNum_Links Text_Pointer_X Text_Pointer_Y\n");

		fprintf(outFile,"\nText_Start\n");
		dialogNum++;
		
		/* Read and dump dialog contents */
		for(x = 0; x < dlogSizeBytes; x+=2){
		
			/* Read the next control code or font character */
			fread(&dlogWord,1,2,inFile);
			if(feof(inFile)){
				printf("EOF Detected.\n");
				break;
			}
			if(!type)
				swap16(&dlogWord);
			
			/******************************/
			/* Print Text or Control Code */
			/******************************/

			/* PSX WD Font - Transition from Text Mode Inactive to Active */
			if((type == PSX_US) && ((dlogWord &0xFF00) == 0x0600) &&
			    (textOutputActive == 0)){
				unsigned char firstChar;
				unsigned char printChar;
				fprintf(outFile,"\"");
				textOutputActive = 1;
				firstChar = dlogWord & 0xFF;
				printChar = (firstChar&0x7F)+0x1F;
				if(printChar == 0x5F)
					printChar = ' ';
						
				if(firstChar & 0x80){
					if(firstChar != 0xFF){
						fprintf(outFile,"%c",printChar);
					}
					textOutputActive = 0;
					fprintf(outFile,"\"\n");
				}
				else{
					fprintf(outFile,"%c",printChar);
				}
				continue;
			}

            /* Extra PSX US Notes: */
			/* 0600 means return to command mode */
		    /* Character 0x5F is a space not an _ character */
			
			/* PSX WD Font - Already in Text Mode */
			if(textOutputActive && (type == PSX_US)){
				unsigned short firstWd,secondWd;
				unsigned char printChar;
				firstWd = (0xFF00 & dlogWord) >> 8;
				secondWd =(0x00FF & dlogWord);
				if(dlogWord == 0x0600){
					/* Re-enter command mode */
					textOutputActive = 0;
					fprintf(outFile,"\"\n");
					continue;
				}
				
				printChar = (firstWd&0x7F)+0x1F;
				if(printChar == 0x5F)
					printChar = ' ';
				
				if(firstWd & 0x80){
					if(firstWd != 0xFF)
						fprintf(outFile,"%c",printChar);
					textOutputActive = 0;
					fprintf(outFile,"\"\n");
				}
				else{
					fprintf(outFile,"%c",printChar);					
				}
				
				printChar = (secondWd&0x7F)+0x1F;
				if(printChar == 0x5F)
					printChar = ' ';
				
				if(secondWd & 0x80){
					if(secondWd != 0xFF)
						fprintf(outFile,"%c",printChar);
					textOutputActive = 0;
					fprintf(outFile,"\"\n");
				}
				else{
					fprintf(outFile,"%c",printChar);
				}
				continue;
			}

			
			/* Font Character */
            if((!type) && (dlogWord < 0x1000)){
                /* Saturn JP */
				
				/* Open Text Quote */
                if(!textOutputActive){
					textOutputActive = 1;
					fprintf(outFile,"\"");
				}
				memset(data,0,10);
				if(getUTF8character((int)dlogWord, data) >= 0){
					printf("%s",data);
					fprintf(outFile,"%s",data);
				}
			}
			else if(!type && (dlogWord==0x9010)){
				/* Saturn JP - Space */
				printf(" ");
				fprintf(outFile," ");
			}
			else if(type && ((dlogWord > 0x8000) && (dlogWord < 0xA000))){
				/* PSX JP Shift-jis */
				
				/* Open Text Quote */
                if(!textOutputActive){
					textOutputActive = 1;
					fprintf(outFile,"\"");
				}
				swap16(&dlogWord);
				fwrite(&dlogWord,2,1,outFile);
			}
			else{
				/* Control Word */
				unsigned int dlogLW = (unsigned int)dlogWord;
				unsigned int identifier = (dlogWord >> 12) & 0xF;
				
				/* Close Text Quote */
                if(textOutputActive){
					textOutputActive = 0;
					fprintf(outFile,"\"\n");
				}
				
				/* Supress printing this one out, some weird WD thing to end */
				/* lines with 0x3007 */
				if((type == PSX_US) && ((dlogWord == 0x3007) || dlogWord == 0x3000))
					continue;
				
				fprintf(outFile,"Control_Code 0x%4X ",dlogWord);

				/* Decode Control Code */
				if((type == PSX_US) || (type == PSX_JP)){
					decodePsx_CtrlCode(type, tmp, outFile, identifier, dlogLW);
				}
				else{
					decodeJPSat_CtrlCode(tmp, outFile, identifier, dlogLW);
				}
			}
		}
		fprintf(outFile,"Text_End\n\n");
		currentOffset += dlogSizeBytes;
	}	
	fprintf(outFile,"</Text_Section>\n\n");		
	
	
	
	fprintf(outFile,"<Script_Errors>\n");
	for(x = 0; x < numTextPointers; x++){
		if(dlgCmdLink[x] == 0){		
    		fprintf(outFile,"Error, no link found for 0x1D CMD at 0x%08X\n",dlgCmdLocation[x]);
		}
	}
	fprintf(outFile,"</Script_Errors>\n");
	
    
    /* Close files */
    fclose(inFile);
	fclose(outFile);

    return 0;
}




/*			
			Saturn JP Control Codes
			=============
			0x1000 <-- Moves text to next line. Control code 0x1000 3x in a row ends dialog with push button.
			0x2xxx <-- hit a button, when hit clears the box and keeps on printing
			0x3xxx <-- prints the left half of character 0xXXX
			0x4xxx <-- nothing is printed or performed
			0x5xxx <-- Dialog Selection.  If lowest byte is 2, there are 2 selections.  If lowest byte is 3 there are 3 selections.
					   Selections are just 16-bit text, each ending in 0x1000.
			0x6000 <-- Not used.
			0x7xxx <-- hit a button, when hit clears the box and keeps on printing
			0x8xxx <-- hit a button, when hit clears the box and keeps on printing
			0x9xxx <-- space printed.  Usually 9010, but since theres no vwf, i think it doesnt matter
			0xAxxx <-- Unknown
			0xBxxx <-- Prints some text about items
			0xCxxx <-- Display Left portrait
			0xDxxx <-- Animations? D001 is lucias entrance
			0xExxx <-- Text pause
			0xFxxx <-- Displays more left portraits
*/			
void decodeJPSat_CtrlCode(char* tmp, FILE* outfile,
                          unsigned int identifier, unsigned int dlogLW){
	
	switch(identifier){
					
		case 1:
			sprintf(tmp,"(newline)\n");
		break;

		case 2:
		case 7:
		case 8:
			sprintf(tmp,"(Press_button,auto-clear box)\n");
		break;

		case 3:
		{
			char data[32];
			memset(data,0,10);
			if(getUTF8character((int)(dlogLW&0xFFF), data) >= 0){
				sprintf(tmp,"(Print left half: \"%s\")\n",data);
			}
			else
				sprintf(tmp,"(Print left half: 0x%X)\n",dlogLW&0xFFF);
		}	
		break;

		case 4:
			sprintf(tmp,"(Not Used)\n");
		break;

		case 5:
		{
			sprintf(tmp,"(Dlog Selection (%d))\n", dlogLW&0xF);
			/* # of selections follow, each ending in 0x1000 */
		}	
		break;

		case 6:
			sprintf(tmp,"(Not Used)\n");	
		break;
		case 9:
			sprintf(tmp," ");
		break;	
		case 0xA:
			sprintf(tmp,"(Not Used)\n");
		break;					
		case 0xB:
			sprintf(tmp,"(Item_Print 0x%X)\n",dlogLW);
		break;
		case 0xC:
		case 0xF:
			sprintf(tmp,"(Left Portrait 0x%X)\n",dlogLW);
		break;
		case 0xD:
			sprintf(tmp,"(Animation?)\n");
		break;
		case 0xE:
			sprintf(tmp,"(Text Pause 0x%X)\n",dlogLW&0xFFF);
		break;
		default:
			sprintf(tmp,"(ERROR: 0x%X)\n",dlogLW);
		break;					
	}
	fprintf(outfile,tmp);
	printf(tmp);
	
	return;
}


/*			
			PSX JP Control Codes
			=============
			0x1000 <-- Moves text to next line. Control code 0x1000 3x in a row ends dialog with push button.
			0x2xxx <-- hit a button, when hit clears the box and keeps on printing
			0x3xxx <-- 0x3004 is a '!' ; 0x3005 is a '?'.  Probably half-width like Saturn version.
			0x4xxx <-- Dialog Selection.  If lowest byte is 2, there are 2 selections.  If lowest byte is 3 there are 3 selections.
					   Selections are just 16-bit text, each ending in 0x1000.
			0x5xxx <-- Not used.
			0x6000 <-- hit a button, when hit clears the box and keeps on printing
			0x7xxx <-- Print text graphic.  0x7001 prints a heart. 0x7006 is a music note.
			0x8xxx <-- Shift-JIS text
			0x9xxx <-- Shift-JIS text?
            0xAxxx <-- Prints some text about items
			0xBxxx <-- Display Left portrait
			0xCxxx <-- Animations? D001 is lucias entrance
			0xDxxx <-- Text pause
			0xExxx <-- Not used.
			0xFxxx <-- Not used.
*/
/*			
			PSX US Control Codes
			=============
			0x1000 <-- Moves text to next line. Control code 0x1000 3x in a row ends dialog with push button.
			0x2xxx <-- hit a button, when hit clears the box and keeps on printing
			0x3xxx <-- Used once (0x3000), 0x3007 used a lot at the end of printing. Seems like no action, some sort of WD formatting thing.
			0x4xxx <-- Dialog Selection.  If lowest byte is 2, there are 2 selections.  If lowest byte is 3 there are 3 selections.
					   Selections are just 16-bit text, each ending in 0x1000.
			0x5xxx <-- Not used.
			0x6000 <-- hit a button, when hit clears the box and keeps on printing
			0x7xxx <-- Not Used
			0x8xxx <-- Not used
			0x9xxx <-- Not used
            0xAxxx <-- Prints some text about items
			0xBxxx <-- Display Left portrait
			0xCxxx <-- Animations? D001 is lucias entrance
			0xDxxx <-- Text pause
			0xExxx <-- Not used.
			0xFxxx <-- Not used.
*/
void decodePsx_CtrlCode(int type, char* tmp, FILE* outfile,
                        unsigned int identifier, unsigned int dlogLW){
	
	switch(identifier){
					
		case 1:
			sprintf(tmp,"(newline)\n");
		break;
		case 2:
			sprintf(tmp,"(Press_button,auto-clear box)\n");
		break;

		case 3:
		{
			if(type == PSX_JP){
				if(dlogLW == 0x3004)
					sprintf(tmp,"(Print left half: \"!\")\n");
				else if (dlogLW == 0x3005)
					sprintf(tmp,"(Print left half: \"?\")\n");
				else
					sprintf(tmp,"(Print left half: Unknown)\n");
			}
			else
				return; //Nothing to do for us psx
		}
		break;

		case 4:
		{
			sprintf(tmp,"(Dlog Selection (%d))\n", dlogLW&0xF);
			/* # of selections follow, each ending in 0x1000 */
		}	
		break;
		case 5:
			sprintf(tmp,"(Not Used)\n");
		break;
		case 6:
			sprintf(tmp,"(Press_button,clears box,continues printing)\n");	
		break;

		case 7:
		{
			if(dlogLW == 0x7001)
				sprintf(tmp,"(Print heart\n");
			else if (dlogLW == 0x7006)
				sprintf(tmp,"(Print music note\n");
			else
				sprintf(tmp,"(Print Unknown Graphic Character)\n");
		}
		break;
										
		case 0xA:
			sprintf(tmp,"(Item_Print 0x%X)\n",dlogLW);
		break;
		case 0xB:
			sprintf(tmp,"(Left Portrait 0x%X)\n",dlogLW);
		break;
		case 0xC:
			sprintf(tmp,"(Animation?)\n");
		break;
		case 0xD:
			sprintf(tmp,"(Text Pause 0x%X)\n",dlogLW&0xFFF);
		break;
		
		case 0xE:
		case 0xF:
			sprintf(tmp,"(Not Used)\n");
		break;	
		
		default:
			sprintf(tmp,"(ERROR: 0x%X)\n",dlogLW);
		break;					
	}
	fprintf(outfile,tmp);
	printf(tmp);
	
	return;
}

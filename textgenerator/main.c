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
#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif


/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"


#define VER_MAJ    1
#define VER_MIN    0

#define FILE_MODE
/*#undef  FILE_MODE*/

#define FONT_CHARACTER_SIZE_BYTES (2 * 16 * 16 / 8) /* 2bpp, 16x16 character */
unsigned int ADDR_FONT_OFFSET = 0;


#define INTER_WORD_SPACING_PIXELS 1

static char inSATFileName[300];
static char outFileName[300];
static char outBuf[1024*1024];
static char outBuf2[1024*1024];
unsigned int outBufSize = 0;
int outBufSize2 = 0;

int G_ENABLE_SHADED_FONT = 0;

unsigned int G_NEXT_LOCATION = 0;
unsigned int G_ACTUAL_SLOT = 0;

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
    printf("l2_vwf_tester.exe\n");
    printf("\n\n");
    return;
}



int getWidth(unsigned char* pFontChar2bpp);
void DrawToBuffer(unsigned char* pOrigFontCharacter, int character_pixel_start, int pixels_to_draw,
					unsigned char* active_16x16_buffer, int activeBufferIndexPixel);
void DrawCharacterToLoRAM(unsigned char* activeBuffer, unsigned int dstAddr, int draw_slot);
void eraseBuffer(void* pBuffer);

/* Globals */
FILE * outFile = NULL;
unsigned char* ADDR_VWF_DRAW;

#ifdef FILE_MODE
char fontBuf[1024];
static char FONT_FILE[1024*1024];
unsigned char* G_Active_16x16_Buffer;
#endif


static char fourbppBuf[1024*1024];

static char allBuf[1024*1024];
unsigned int allBufSize;

void printVWF(unsigned int characterToDraw, unsigned int dstAddrBase, unsigned int r6val, int draw_slot);



void minimizeSize(int cursorPosition, int num16bitBlocks, char* buffer, int inputBufSize, 
	char* outBuf, int* outBufSizeBytes, int* new_width, int* new_height){

	char* zeroArray;
	int width_pixels, height_pixels, x, line, width_bytes, lineskip;
	int outSize = 0;
	int num_pixels = 0;

    /* Init Height */
	/* Top line and bottom lines are blank in the English Font */
	height_pixels = 16;
	*new_height = height_pixels;

	/* Width = # already drawn blocks * 16bits + Current Draw Block width in pixels */
	/* Cannot be reduced */
	if(cursorPosition != 0)
		num16bitBlocks++;
	width_pixels = num16bitBlocks * 16;
	*new_width = width_pixels;

	/* Now copy the pixels, At 2bpp, each line is 32 bits = 4 bytes */
	/* Font doesnt use top or bottom lines */
	/* Skip top lines and bottom lines */
	width_bytes = (width_pixels*2/8);
	zeroArray = (char*)malloc(width_bytes*2);
	if(zeroArray == NULL){
		printf("Malloc Error");
		exit(-1);
	}
	memset(zeroArray,0,width_bytes*2);

	lineskip = 0;
	for(line = 0; line < height_pixels; line++){
		int noskip = 0;

		/* Skip blank lines */
		for(x = 0; x < num16bitBlocks; x++){
			if(memcmp(zeroArray,&buffer[(x*16*4)+(line*4)],4) != 0){
				noskip = 1;
			}
		}
		if(!noskip){
			lineskip++;
			continue;
		}

		/* Copy Block Data */
		for(x = 0; x < num16bitBlocks; x++){
			memcpy(&outBuf[outSize],&buffer[(x*16*4)+(line*4)],4);
			outSize+=4;
		}
	}
	*new_height = *new_height-lineskip;
	*outBufSizeBytes = outSize;

	return;
}




unsigned int convertTo4bpp(char* inBuf2bpp, unsigned int sizeBytes, char* fourbppBuf){
#define FOURBPP_VAL 0x1
	int i,y;
	unsigned char b1,b2,val0,val1;
	y = 0;

	for(i=0; i < (int)sizeBytes; i++){
		b1=b2=0;

		val0 = (inBuf2bpp[i] & 0xC0) >> 6;
		val1 = (inBuf2bpp[i] & 0x30) >> 4;
		b1 = (val0 << 4) | val1;

		val0 = (inBuf2bpp[i] & 0x0C) >> 2;
		val1 = (inBuf2bpp[i] & 0x03);
		b2 = (val0 << 4) | val1;

		fourbppBuf[y++] = b1;
		fourbppBuf[y++] = b2;
	}

#if 0
	for(i=0; i < (int)sizeBytes; i++){
		b1=b2=0;

		val0 = (inBuf2bpp[i] & 0xC0);
		if(val0)
			val0 = FOURBPP_VAL;
		val1 = (inBuf2bpp[i] & 0x30);
		if(val1)
			val1 = FOURBPP_VAL;
		b1 = (val0 << 4) | val1;

		val0 = (inBuf2bpp[i] & 0x0C);
		if(val0)
			val0 = FOURBPP_VAL;
		val1 = (inBuf2bpp[i] & 0x03);
		if(val1)
			val1 = FOURBPP_VAL;
		b2 = (val0 << 4) | val1;

		fourbppBuf[y++] = b1;
		fourbppBuf[y++] = b2;
	}
#endif

	return y;
}




/******************************************************************************/
/* main()                                                                     */
/******************************************************************************/
int main(int argc, char** argv){


    unsigned int dstAddrBase, x, z;
	int draw_slot, numread;
	unsigned int newSize;
	int new_width,new_height;
	int allBufSize = 0;

    FILE *inFile_SAT = NULL;

	char* testString[] = {"Buy","Sell","Equip","Exit","Attack",
		"Magic","Item","Defense","Command", "Armor","Tools", 
		"memorized", "learned", "Exp","Self", "Line", "Item", 
		"Defense","Tactics 1","Tactics 2","Tactics 3", "Save",
		"Buy","Sell","Equip","Exit"};


    printf("Lunar2 VWF Test Program v%d.%02d\n", VER_MAJ, VER_MIN);

    /**************************/
    /* Check input parameters */
    /**************************/

    /* Check for valid # of args */
    if (argc != 2){
        printUsage();
        return -1;
    }
	if(atoi(argv[1]) == 0){
	    strcpy(inSATFileName, "1231");
	}
	else{
		strcpy(inSATFileName,"shaded_font.bin");
		G_ENABLE_SHADED_FONT = 1;
	}

    /*******************************/
    /* Open the input/output files */
    /*******************************/
    inFile_SAT = outFile = NULL;
    inFile_SAT = fopen(inSATFileName, "rb");
    if (inFile_SAT == NULL){
        printf("Error occurred while opening font file %s for reading\n", inSATFileName);
        return -1;
    }
	numread = fread(FONT_FILE,1,1024*1024,inFile_SAT);
	fclose(inFile_SAT);	
	ADDR_FONT_OFFSET = (unsigned int)FONT_FILE;


	
	/* Set up VWF Draw Buffers */
#ifdef FILE_MODE
    G_Active_16x16_Buffer = (unsigned char*)malloc(FONT_CHARACTER_SIZE_BYTES);
	if(G_Active_16x16_Buffer == NULL){
		printf("VWF Active_16x16_Buffer error.\n");
		return -1;
	}
	ADDR_VWF_DRAW = (unsigned char*)G_Active_16x16_Buffer;
#else
	Active_16x16_Buffer = (unsigned char*)ADDR_VWF_DRAW;
#endif	


	for(z=0; z < 26; z++){
		/* Init */
		outBufSize = 0;		
		dstAddrBase = 0;
		draw_slot = 0;
	
		/*Erase Scratch Buffers*/
		eraseBuffer(G_Active_16x16_Buffer);

		printf("Printing %s\n",testString[z]);

		/* Print each letter from the text string, one at a time */
		for(x = 0; x < strlen(testString[z]); x++){

			int r6val = 0;

//			printf("Printing '%c'\n",testString[x]);
			printVWF((unsigned int) testString[z][x], (unsigned int)outBuf, r6val, draw_slot);
			draw_slot++;
		}


		/**************************/
		/* Adjust size to minimum */
		/**************************/
		minimizeSize(G_NEXT_LOCATION, G_ACTUAL_SLOT, outBuf,outBufSize,
			outBuf2, &outBufSize2, &new_width,&new_height);
		memcpy(outBuf,outBuf2,outBufSize2);
		outBufSize = outBufSize2;

		newSize = convertTo4bpp(outBuf,outBufSize,fourbppBuf);

		/*******************/
		/* Open the output */
		/*******************/
#if 0
		if(z==0)
			sprintf(outFileName, "%s_%dx%d_4bpp.bin","own",new_width,new_height);
		else if(z==7)
			sprintf(outFileName, "%s_%dx%d_4bpp.bin","items",new_width,new_height);
		else if(z==9)
			sprintf(outFileName, "%s_%dx%d_4bpp.bin","received",new_width,new_height);
		else
#endif
		sprintf(outFileName, "%s_%dx%d_4bpp.bin",testString[z],new_width,new_height);

		outFile = fopen(outFileName, "wb");
		if (outFile == NULL){
			printf("Error occurred while opening output file %s for writing\n", outFileName);
			fclose(inFile_SAT);
			return -1;
		}	

		/* Write to output file */
		fwrite(fourbppBuf,1,newSize,outFile);
		/* Close files */
		fclose(outFile);
	}


#ifdef FILE_MODE
	free(G_Active_16x16_Buffer);
#endif

    return 0;
}





#define SHADED_INTER_WORD_SPACING_PIXELS 0

void printVWF(unsigned int characterToDraw, unsigned int dstAddrBase, unsigned int r6val, int draw_slot){

	static int Next_Pixel_Column_Start = 0;
	static int actualDrawSlot = 0;
	static int lastSlotDrawn = -1;
	int pixel_remainder, Pixel_Column_End, pixels_to_draw, character_width, interword_spacing_pixels;

	/* Init */
	unsigned char* Active_16x16_Buffer = (unsigned char*)ADDR_VWF_DRAW;
	unsigned char *pOrigFontCharacter;

	/* Get ptr to character data */
	if(G_ENABLE_SHADED_FONT){
		pOrigFontCharacter = (unsigned char*)(ADDR_FONT_OFFSET + characterToDraw*FONT_CHARACTER_SIZE_BYTES);
		interword_spacing_pixels = SHADED_INTER_WORD_SPACING_PIXELS;
	}
	else{
        pOrigFontCharacter = (unsigned char*)(ADDR_FONT_OFFSET + characterToDraw*FONT_CHARACTER_SIZE_BYTES);
		interword_spacing_pixels = INTER_WORD_SPACING_PIXELS;
	}

	/* If this is the first character to print to the current LORAM buffer, */
	/* Reset the start draw column to zero and erase the vwf buffer.        */
	if(draw_slot == 0){
	    Next_Pixel_Column_Start = 0;
		actualDrawSlot = 0;
		eraseBuffer(Active_16x16_Buffer);
		lastSlotDrawn = -1;
	}


	/*******************************************************/
	/* Determine if we are clearing the rest of the buffer */
	/* Or the first half of the rest of the buffer         */
	/*******************************************************/
	if((characterToDraw == 0x0100) || (characterToDraw == 0x0101)){

		int y, endDrawSlot;

		/* Update Draw Slot */
		actualDrawSlot = lastSlotDrawn + 1;

		/* Determine # of remaining 16-bit spacing to clear */
		if(characterToDraw == 0x0100){
			endDrawSlot = r6val;
		}
		else{
			endDrawSlot = r6val >> 1;

			//Error handling for single buffer, 2 line case
			//Just advance (or rewind) to the next line
			if(actualDrawSlot >= endDrawSlot){
				actualDrawSlot = endDrawSlot;
				Next_Pixel_Column_Start = 0;
				return;
			}
		}

		//Do Nothing
		if(actualDrawSlot >= endDrawSlot){
			return;
		}

		//Clear the buffer, clear the rest of the line
		eraseBuffer(Active_16x16_Buffer);
		for(y=actualDrawSlot; y < endDrawSlot; y++){
			DrawCharacterToLoRAM((unsigned char*)dstAddrBase, r6val, y);			
		}
		actualDrawSlot = y;
		Next_Pixel_Column_Start = 0;

		return;
	}

	

	/*****************************************************/
    /* Print the current letter to the active vwf buffer */
	/*****************************************************/

	/* Determine the width of the font character in pixels */
	/* Take care of 0x9xxx character codes                 */
	if(characterToDraw & 0x9000){
		character_width = characterToDraw & 0xFF;
		if(character_width > 16)
			character_width = 16;  //limit to 16 pixels
		
		/* Update ptr to character data */
		characterToDraw = 0x20;
		pOrigFontCharacter = (unsigned char*)(ADDR_FONT_OFFSET + characterToDraw*FONT_CHARACTER_SIZE_BYTES);
	}
	else{
		character_width = getWidth(pOrigFontCharacter);
	}

    /* Calculate where the final pixel in the image will land */
	Pixel_Column_End = Next_Pixel_Column_Start + character_width - 1;
	if(Pixel_Column_End > 15){
		/* Draw crosses a 16x16 boundary, need to write to 2 buffers */
		pixel_remainder = Pixel_Column_End - 15;
		Pixel_Column_End = 15;
		pixels_to_draw = Pixel_Column_End - Next_Pixel_Column_Start + 1;
	}
	else{
		/* Draw contained within the current 16x16 buffer */
		pixels_to_draw = character_width;
		pixel_remainder = 0;
	}

	/* VWF Buffer Draw */
	DrawToBuffer(pOrigFontCharacter, 0, pixels_to_draw, 
		Active_16x16_Buffer, Next_Pixel_Column_Start);
		
	/* Screen Draw */
	DrawCharacterToLoRAM((unsigned char*)dstAddrBase, r6val, actualDrawSlot);
	lastSlotDrawn = actualDrawSlot;

	/* Move to the next draw location */
	Next_Pixel_Column_Start = Pixel_Column_End+1;
	if(!pixel_remainder){
		Next_Pixel_Column_Start += interword_spacing_pixels;
		/* Account for 1 pixel spacing */
		/* Also Fix for 'f' ascii character spacing */
		if((characterToDraw == 0x20) || (characterToDraw == (unsigned int)'f')){
			Next_Pixel_Column_Start -= interword_spacing_pixels;
		}
	}
	if(Next_Pixel_Column_Start > 15){
		Next_Pixel_Column_Start -= 16;
		eraseBuffer(Active_16x16_Buffer);
		actualDrawSlot++;
	}


	/* If the character is spilling into the second buffer */
	/* draw that buffer as well                            */
	if(pixel_remainder){

		/* VWF Buffer Draw */
		DrawToBuffer(pOrigFontCharacter, pixels_to_draw, pixel_remainder, 
		    Active_16x16_Buffer, Next_Pixel_Column_Start);

		/* Copy VWF Buffer to LORAM and make it 4bpp */
		DrawCharacterToLoRAM((unsigned char*)dstAddrBase, r6val, actualDrawSlot);
		lastSlotDrawn = actualDrawSlot;

		/* Move to the next draw location */
		/* Swap buffers if necessary      */
		Next_Pixel_Column_Start = pixel_remainder+interword_spacing_pixels;

		/* Account for 1 pixel spacing */
		/* Also Fix for 'f' ascii character spacing */
		if((characterToDraw == 0x20) || (characterToDraw == (unsigned int)'f')){
			Next_Pixel_Column_Start -= interword_spacing_pixels;
		}

		if(Next_Pixel_Column_Start > 15){
			Next_Pixel_Column_Start -= 16;
			eraseBuffer(Active_16x16_Buffer);
			actualDrawSlot++;
		}
	}
	
	/* Globals to keep track of draw cursor location */
	G_NEXT_LOCATION = Next_Pixel_Column_Start;
	G_ACTUAL_SLOT = actualDrawSlot;

	return;
}





void DrawToBuffer(unsigned char* pOrigFontCharacter, int character_pixel_start, int pixels_to_draw,
					unsigned char* active_16x16_buffer, int activeBufferIndexPixel){

	int x;
	unsigned int* pUintFontData = (unsigned int*)pOrigFontCharacter;
	unsigned int* pUintDrawBuffer = (unsigned int*)active_16x16_buffer;
	unsigned int mask = 0;
	
    /* Nothing to do if nothing to be drawn */
	if(pixels_to_draw <= 0)
		return;

    /* Pixel Draw Mask */
	for(x = 0; x < pixels_to_draw; x++){
        mask >>= 2;		
		mask |= 0xC0000000;
	}
	
    /* Determine Pixel Data to be drawn by row */	
	for(x = 0; x < 16; x++){
	    unsigned int data;
		data = pUintFontData[x];
#ifdef FILE_MODE
		swap32(&data);
#endif
		data = (data << character_pixel_start*2) & mask;
		data >>= (activeBufferIndexPixel*2);
#ifdef FILE_MODE
		swap32(&pUintDrawBuffer[x]);
#endif
		pUintDrawBuffer[x] |= data;
#ifdef FILE_MODE
		swap32(&pUintDrawBuffer[x]);
#endif
	}	
	
	return;
}







/*********************************************************/
/* Function: getWidth                                    */
/* Input is a pointer to a 2bpp 16x16 font character     */
/* (Every 4 bytes is one row, assume linear.)            */
/* Ors the bytes, finds the rightmost bit to             */
/* determine the width.  Remember to div by 2            */
/* Returns SPACE_WIDTH_PIXELS if the character is empty. */
/*********************************************************/
#define SPACE_WIDTH_PIXELS 5
int getWidth(unsigned char* pFontChar2bpp){

	int x;
	unsigned int orData;
	unsigned int* pUint = (unsigned int*)pFontChar2bpp;
	int index = -1;

    orData = 0;
	for(x=0;x<16;x++){
#ifdef FILE_MODE
        unsigned int tmpVal;
        tmpVal = pUint[x];
		swap32(&tmpVal);
		orData |= tmpVal;
#else
		orData |= pUint[x];	
#endif
	}

	
	for(x = 0; x < 16; x++){
		if(orData & 0x3){
			index = x;
			break;
		}
		orData >>= 2;
	}
	
	/* Space if no pixels found */
	if(index >= 0)
		index = 16-index;
	else
		index = SPACE_WIDTH_PIXELS;
	
	return index;
}




void DrawCharacterToLoRAM(unsigned char* activeBuffer, unsigned int dstAddr, int draw_slot){
	
	int offset;
	
	offset = draw_slot*FONT_CHARACTER_SIZE_BYTES;
//	printf("Writing to LORAM offset 0x%X\n",offset);
	memcpy(&outBuf[offset],/*activeBuffer*/ADDR_VWF_DRAW,FONT_CHARACTER_SIZE_BYTES);
	if((offset + FONT_CHARACTER_SIZE_BYTES) > (int)outBufSize)
		outBufSize = offset + FONT_CHARACTER_SIZE_BYTES;
	
	return;
}



void eraseBuffer(void* pBuffer){

	int x;
	unsigned int* pData = (unsigned int*)pBuffer;
	for(x=0;x<16;x++)
		pData[x] = 0;
	return;
}

/***********************************************************************/
/* Lunar2 Menu Graphics Replacer for Sega Saturn                       */
/*                                                                     */
/* Usage                                                               */
/* ========================================                            */
/* l2replaceMenuGraphics.exe inputFile                                 */
/*                                                                     */
/* This program will modify the orignal menu graphics file (1227)      */
/* according to the file listed in the input file                      */
/* Requires the original graphics file "1227" to be in the same folder */
/* as the executable.                                                  */
/***********************************************************************/
#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CMN_GRAPHIX_HDR_SIZE	8


/******************************************************************************/
/* printUsage() - Display command line usage of this program.                 */
/******************************************************************************/
void printUsage(){
    printf("Error in input arguments.\n");
    printf("Usage:\n=========\n");
	printf("l2replaceMenuGraphics.exe inputFile\n\n");
    return;
}



/****************************/
/* swap16                   */
/* Endian swap 16-bit data  */
/****************************/
void swap16(void* pWd){
    char* ptr;
    char temp;
    ptr = pWd;
    temp = ptr[1];
    ptr[1] = ptr[0];
    ptr[0] = temp;
    return;
}

void updateImgReferences(char* cmnFileBuffer, unsigned int oldOffset, unsigned int newOffset, int width, int height,
                         unsigned char paletteVal, unsigned short xval, unsigned short yval);




int main(int argc, char** argv){

    FILE* outFile, *tmpFile, *inFile;
	static char inputName[300], origFname[300], tmpName[300], 
	            outputName[300],line[1024];
	char* buffer;
	char* menu_file_buffer;
	unsigned int bufferSize, origFileSize;
	int numRead, numWritten, x;
	char* cmn_file_buffer;
	int cmnFileSize = 0;
	origFileSize = 0;
	
	memset(inputName,0,300);
	memset(tmpName,0,300);
	
	/* Check input args */
	if (argc != 2){
		printUsage();
		return -1;
	}

	strcpy(inputName,  argv[1]);
	strcpy(origFname,  "1227");
	strcpy(outputName, "1227_updated.bin");
	

	/**************/
	/* Open files */
	/**************/

	/*****************/
	/* Graphics File */
	/*****************/

	/* Allocate a buffer for the entire graphics file */
	bufferSize = 1024*1024;
	menu_file_buffer = (char*)malloc(bufferSize);
	if(menu_file_buffer == NULL){
		printf("Malloc failure\n");
		return -1;
	}
	memset(menu_file_buffer,0,1024*1024);

	/* Read Original Graphics File into Memory */	
	inFile = fopen(origFname,"rb");
	if(inFile == NULL){
		printf("Error opening file %s for reading\n",origFname);
		free(menu_file_buffer);
        return -1;
	}
	numRead = fread(menu_file_buffer,1,1024*1024,inFile);
	if(numRead <= 0){
		printf("Error copying original graphics file into memory.\n");
		free(menu_file_buffer);
		return -1;
	}
	origFileSize = numRead;
	fclose(inFile);


	/*****************/
	/* Common File   */
	/*****************/

	/* Allocate a buffer for the entire graphics file */
	bufferSize = 1024*1024;
	cmn_file_buffer = (char*)malloc(bufferSize);
	if(cmn_file_buffer == NULL){
		printf("Malloc failure\n");
		return -1;
	}
	memset(cmn_file_buffer,0,1024*1024);

	/* Read Original Graphics File into Memory */	
	inFile = fopen("1233","rb");
	if(inFile == NULL){
		printf("Error opening file %s for reading\n","1233");
		free(cmn_file_buffer);
        return -1;
	}
	numRead = fread(cmn_file_buffer,1,1024*1024,inFile);
	if(numRead <= 0){
		printf("Error copying original cmn file into memory.\n");
		free(cmn_file_buffer);
		return -1;
	}
	cmnFileSize = numRead;
	fclose(inFile);


	
	/* Input Script */
	inFile = fopen(inputName,"rb");
	if(inFile == NULL){
		printf("Error opening file %s for reading\n",inputName);
		free(menu_file_buffer);
        return -1;
	}
	
	/* Output File - Updated Menu Graphics */
	outFile = fopen(outputName,"wb");
	if(outFile == NULL){
		printf("Error opening file %s for writing data\n",outputName);
        fclose(inFile);
		free(menu_file_buffer);
		return -1;
	}
		
	/* Allocate a read buffer */
	bufferSize = 1024*1024;
	buffer = (char*)malloc(bufferSize);
	if(buffer == NULL){
		printf("Malloc failure\n");
		fclose(outFile);
		fclose(inFile);
		free(menu_file_buffer);
		return -1;
	}
	




	/*******************************************************************/
	/* Iterate through the input files, updating the output buffer     */
	/* and the common file that references the data in it              */
	/*******************************************************************/
	x = 0;
	while(!feof(inFile))
	{
		unsigned int fsize, oldOffset, newOffset, paletteVal, xval, yval;
		int width, height, rval, expectedSize;
		char tmpName[300];

		//Line #
        x = x+1;

		//Construct Input Filename
		memset(line,0,1023);
		fgets(line,1023,inFile);
		if(feof(inFile))
			break;		

		/* Comment, ignore line */
		if(line[0] == '#')
			continue;

		/* Parse Line */
		memset(tmpName,0,300);
		rval = sscanf(line,"%x %x %d %d %x %x %x %s", &oldOffset, &newOffset, &width, &height, 
			&paletteVal, &xval, &yval, tmpName);
		if(rval <= 0)
			continue;
		else if(rval < 8){
			printf("Warning, Ignoring malformed parameters on line %d.\n", x);
			continue;
		}

		//Check for erase (width is # bytes in this case)
		if(strcmp(tmpName,"0") == 0){
			printf("Erasing %d bytes starting at offset 0x%X\n", width, newOffset&0xFFFF);
			memset(&menu_file_buffer[CMN_GRAPHIX_HDR_SIZE + newOffset],0x00,width);
			continue;
		}
		
		//Open the input file for reading
		printf("Opening %s Old=0x%X New=0x%X w=%d h=%d\n",tmpName,oldOffset,newOffset,
			width, height);
		tmpFile = fopen(tmpName,"rb");
		if(tmpFile == NULL){
			printf("Error opening binary input file %s for reading\n",tmpName);
			fclose(outFile);
			fclose(inFile);
			free(buffer);free(menu_file_buffer);
			return -1;
		}

		//Determine the size of the input file in bytes
		fseek(tmpFile,0,SEEK_END);
		fsize = ftell(tmpFile);
		fseek(tmpFile,0,SEEK_SET);

		expectedSize = (int) (((width*height*4.0) / 8.0));
		if(fsize != expectedSize){
			printf("Error, input size != expected for %s",tmpName);
			fclose(outFile);
			fclose(inFile);
			free(buffer);free(menu_file_buffer);
			fclose(tmpFile);
			return -1;
		}

		//Verify buffer is large enough, resize if needed
		if(bufferSize < fsize){
			free(buffer);
			bufferSize = fsize;
			buffer = malloc(bufferSize);
			if(buffer == NULL){
				printf("Malloc failure\n");
				fclose(outFile);
				fclose(inFile);
				fclose(tmpFile);
				return -1;
			}
		}

		//Write the data to the output buffer
		numRead = fread(buffer,1,fsize,tmpFile);
		if(fsize != numRead){
				printf("File read failure\n");
				fclose(outFile);
				fclose(inFile);
				fclose(tmpFile);
				free(buffer);free(menu_file_buffer);
				return -1;			
		}
		memcpy(&menu_file_buffer[CMN_GRAPHIX_HDR_SIZE + newOffset] , buffer, fsize);

		//Close input file.
		fclose(tmpFile);

		/******************************************/
		/* Update entries in the Common Text File */
		/******************************************/
		updateImgReferences(cmn_file_buffer, oldOffset, newOffset, width, height,
		    (unsigned char)paletteVal, (unsigned short)xval, (unsigned short)yval);

	}
	fclose(inFile);
	
	//Write the data to the output files
	numWritten = fwrite(menu_file_buffer,1,origFileSize,outFile);
	if(origFileSize != numWritten){
		printf("ERROR, Output write to graphics file failure!\n");
	}
	fclose(outFile);

	outFile = fopen("1233_updated.bin","wb");
	if(outFile == NULL){
		printf("Error opening file %s for writing data\n","1233_updated.bin");
		return -1;
	}
	numWritten = fwrite(cmn_file_buffer,1,cmnFileSize,outFile);
	if(cmnFileSize != numWritten){
		printf("ERROR, Output write to cmn file failure!\n");
	}
	fclose(outFile);

	//Free resources
	free(buffer); free(menu_file_buffer);free(cmn_file_buffer);

	printf("\nLunar2 Graphics Recombining Complete!\n");

	return 0;
}






#define IMAGE_INFO_OFFSET        0x5A0
#define IMAGE_INFO_RECORD_SIZE   0x16


typedef struct {
	unsigned char palette;
	unsigned char b0;
	unsigned short imgOffset; /* VDP1 Offset / 8 ; Also File offset */
	unsigned char  width;     /* Pixel Width / 8 */
	unsigned char  height;    /* Pixel Height */
	unsigned short rel_x_posit;
	unsigned short rel_y_posit;
	unsigned short wd0;
	unsigned short wd1;
	unsigned short wd2;
	unsigned short wd3;
	unsigned short wd4;
	unsigned short wd5;
}imageInfo;




void updateImgReferences(char* cmnFileBuffer, unsigned int oldOffset, unsigned int newOffset, int width, int height,
                         unsigned char paletteVal, unsigned short xval, unsigned short yval){

	int currentOffset;
	unsigned char ucWidth, ucHeight;
	unsigned short swOldOffset, swNewOffset, swOrigOffset;

	currentOffset = IMAGE_INFO_OFFSET;

	/* Short word versions */
	swOldOffset = (unsigned short)oldOffset / 8;
	swNewOffset = (unsigned short)newOffset / 8;
	ucWidth =     (unsigned char)(width/8);
	ucHeight =    (unsigned char)(height);

	while(currentOffset < 0xBA44){
		imageInfo* pInfo;
		pInfo = (imageInfo*)&cmnFileBuffer[currentOffset];

		swOrigOffset = pInfo->imgOffset;
		swap16(&swOrigOffset);
		if(swOrigOffset == swOldOffset){
			printf("\tUpdating 0x%04X\n",currentOffset);
			
			pInfo->imgOffset = swNewOffset;
			swap16(&pInfo->imgOffset);      /* Swap Data */
			pInfo->width = ucWidth;
			pInfo->height = ucHeight;
			
			if(paletteVal != 0xFF){
				pInfo->palette = paletteVal;
			}
			if(xval != 0xFFFF){
				pInfo->rel_x_posit = xval;
				swap16(&pInfo->rel_x_posit);      /* Swap Data */
			}
			if(yval != 0xFFFF){
				pInfo->rel_y_posit = yval;
				swap16(&pInfo->rel_y_posit);      /* Swap Data */
			}
		}
		currentOffset += IMAGE_INFO_RECORD_SIZE;
	}

	return;
}

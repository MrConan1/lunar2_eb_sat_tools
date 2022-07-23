#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#define MAX_SIZE (1024*1024*8)


/*
File Format of Subtitle Files
==============================
# of sections in file:  4 bytes
Size of Section 1:      4 bytes
    Section 1 (bunch of 32-bit Words) 
	???
Size of Section 2:      4 bytes
    Section 2 
	    16 short words (to be ored with 0x8000), which are copied to 25C7FFC0
		Size of Subtitle Header section (32-bit LW)
		Subtitle Hdr Offset#1:  32-bit LW  (00000028) {Add 060902B0 to get absolute address = 060902D8}
		    0000002C:  byte offset to subtitle data from subtitle hdr offset.  060902D8+2C = 06090304
			00000009:  # of image data info structures that follow
			           Each structure is 32-bits and contains:
					       8-bit width/8  (mult by 8 to get width)
						   8-bit height
						   16-bit image offset in vram/8  (mult by 8 to get vram offset)
			00002DEB: length of compressed image data in bytes (At address 06090304)
			The image data follows here
		Subtitle Hdr Offset#2:  32-bit LW  (00002E44) {Add 060902B0 to get absolute address = 060930F4}
		    0000000C: byte offset to subtitle data from subtitle hdr offset.  060930F4+C = 06093100
			00000001: # of image data info structures that follow
			           Each structure is 32-bits and contains:
					       8-bit width/8  (mult by 8 to get width)
						   8-bit height
						   16-bit image offset in vram/8  (mult by 8 to get vram offset)
			00008F2E: length of compressed image data in bytes (At address 06093100)
End with Zero Pad to 2048 byte multiple
*/


typedef struct{
	unsigned char width_div_8;
	unsigned char height;
	unsigned short rel_vram_offset_div_8;
}subTImgInfo;


typedef struct{
	unsigned int rel_byteOffsetToData;
	unsigned int numImagesInHdr;
	subTImgInfo imageInfoArray[20];
}subTHdrType;


unsigned int createSubtitles(unsigned int maxLWCount, unsigned char* src, unsigned char* dst);


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



/****************************/
/* swap32                   */
/* Endian swap 32-bit data  */
/****************************/
void swap32(void* pWd){
    char* ptr;
    char temp[4];
    ptr = pWd;
    temp[3] = ptr[0];
    temp[2] = ptr[1];
    temp[1] = ptr[2];
    temp[0] = ptr[3];
    memcpy(ptr, temp, 4);

    return;
}




int main(int argc, char** argv){

	static char outfileName[300];
	FILE* infile, *outfile;
	int x,y;
	unsigned char* srcBuffer, *dstBuffer, *pStartSubtitleHdr;
	unsigned int sizeSection1, subtitleStartOffset;
	unsigned int bytesWritten = 0;
	
	if(argc != 2){
		printf("Error in arguments, Usage:  l2_subt_extract.exe inputFname\n\n");
		return -1;
	}

	/* Allocate memory for src/dst */
	srcBuffer = (unsigned char*)malloc(MAX_SIZE);
	dstBuffer = (unsigned char*)malloc(MAX_SIZE);
	if(srcBuffer == NULL)
		printf("malloc error srcBuffer\n");
	if(dstBuffer == NULL)
		printf("malloc error srcBuffer\n");
	memset(srcBuffer,0,MAX_SIZE);
	memset(dstBuffer,0,MAX_SIZE);

	/* Open and read the entire input file */
	infile = fopen(argv[1],"rb");
	if(infile == NULL){
		printf("Error opening input.\n");
		return -1;
	}
	fread(srcBuffer,1,MAX_SIZE,infile);
	fclose(infile);
	
	/* Find the start of the subtitle graphics header, */
	/* skip over the 32-bit # of sections header &     */
	/* skip over the first section of the file         */
	sizeSection1 = *((unsigned int*)(&srcBuffer[4]));
	swap32(&sizeSection1);
	pStartSubtitleHdr = (&srcBuffer[8+sizeSection1]);
	/* Skip over the size of the second section */
	/* Skip over the 16 short words that get written directly to VDP1 */
	pStartSubtitleHdr += 4;
	subtitleStartOffset = 12+sizeSection1;  //2B0
	pStartSubtitleHdr += (16*2); 
	
	
	/* Now Read in the header data associated with the subtitle images */
	for(x = 0; x < 2; x++){
		unsigned int offset, inputStreamBytes;
		subTHdrType* pSubtImageInfo;
		unsigned int subtImgsStartOffset;
		unsigned int subtitleSectionOffset = *((unsigned int*)pStartSubtitleHdr);  //2B0 +28 = 2D8
		swap32(&subtitleSectionOffset);
		if(subtitleSectionOffset == 0xFFFFFFFF) /* N/A */
			continue;
		subtitleSectionOffset += subtitleStartOffset;

		pSubtImageInfo = (subTHdrType*)(&srcBuffer[subtitleSectionOffset]);
		swap32(&pSubtImageInfo->rel_byteOffsetToData);
		swap32(&pSubtImageInfo->numImagesInHdr);
		
		/* Decompress & Extract All Subtitle Images associates with this subtImageInfo struct */
		subtImgsStartOffset = subtitleSectionOffset + pSubtImageInfo->rel_byteOffsetToData;  // 2D8 + 2C = 304
		offset = subtImgsStartOffset;//0x304;  
		inputStreamBytes = *((unsigned int*)(&srcBuffer[offset]));
		swap32(&inputStreamBytes);
		offset += 4;  //0308
		bytesWritten = createSubtitles(inputStreamBytes, &srcBuffer[offset], dstBuffer);
		printf("Uncompressed Image is %d bytes.\n", bytesWritten);

		for(y = 0; y < (int)pSubtImageInfo->numImagesInHdr; y++){
			
			unsigned int width, height, relVramOffsetUint;
			unsigned short relVramOffset;
			width = pSubtImageInfo->imageInfoArray[y].width_div_8 * 8;
			height = pSubtImageInfo->imageInfoArray[y].height;
			relVramOffset = pSubtImageInfo->imageInfoArray[y].rel_vram_offset_div_8;
			swap16(&relVramOffset);
			relVramOffsetUint = relVramOffset * 8;		
			sprintf(outfileName,"%s_subtimg%d_0x%08X_%dw_%dh.bin",argv[1],x,relVramOffset,width,height);

			/* Open and write the output files */
			outfile = fopen(outfileName,"wb");
			if(outfile == NULL){
				printf("Error opening output.\n");
				return -1;
			}
			fwrite(&dstBuffer[relVramOffsetUint],1,(width*height*4)/8,outfile);
			fclose(outfile);			
		}
		
		/* Advance by 4 bytes */
		pStartSubtitleHdr+=4;
	}


	/* Free resources */
	free(srcBuffer);
	free(dstBuffer);

	return 0;
}




// createSubtitles
// Some sort of LRU compression algorithm with a 4kB rolling history buffer
//Returns # of bytes copied to the destination buffer.
unsigned int createSubtitles(unsigned int maxInputStreamByteCount, unsigned char* pSrc, unsigned char* pDst){

	unsigned int rval = 0;
	unsigned int numBytesRead = 0;
	unsigned int selectionByte;
	unsigned char* pHistoryBuffer;
	int lruIndex;
	unsigned int numBytesCopied, numBytesToCopyFromHistory;
	int perFormDirectCopy, baseIndex;
	unsigned int x, y;
	unsigned char inputData, encodedData;

	//Allocate a history buffer of 4096 bytes and zero it
	pHistoryBuffer = (unsigned char*)malloc(0x1000);
	if(pHistoryBuffer == NULL){
		printf("Error allocating scratch buffer.\n");
	}
	memset(pHistoryBuffer,0,0x1000);


	/* Init */
	numBytesCopied = 0;
	lruIndex = 0xFEE;
	x = 0;

	while(1){

		//On every 8th iteration:
		//A byte is read whose bits are used to determine
		//whether a literal is copied directly from the input stream (bit is set)
		//or if the history buffer is to be used (bit is clear)
		if(x == 0){

			selectionByte = (*pSrc);
			pSrc++;

			//Check for Done
			if(numBytesRead > maxInputStreamByteCount){
				break;
			}
			numBytesRead++;
		}

		//Increment counter to determine when to read the next selection byte from the input
		x++;
		if(x == 8)
			x = 0;

		//Determine if a direct copy or history buffer copy will be taking place
		perFormDirectCopy = selectionByte & 0x1;
		selectionByte >>= 1;

		/* Read in a byte of data */
		inputData = *pSrc;
		pSrc++;

		//Check for Done
		if(numBytesRead > maxInputStreamByteCount){
			break;
		}
		numBytesRead++;

		
		if(perFormDirectCopy){
			//Direct Write of one byte to destination and history buffer
			
			//Copy one byte from src space to the destination
			*pDst = inputData;
			pDst++;

			//Copy the same byte to the next lru index in the history buffer 
			pHistoryBuffer[lruIndex] = inputData;
			lruIndex++;
			lruIndex &= 0xFFF; // Roll over, only index 0 to 4095

			numBytesCopied++;
		}

		else{
			// Perform an n-byte copy from the history buffer (3 or more bytes)
			
			//Read in one additional byte for the history buffer copy
			//Lower nibble + 3 is # of bytes to copy from the history buffer
			//Upper nibble is upperByte of 16-bit index into history buffer.
			//The previously read dataByte is the lower 16-bit portion of the history buffer index
			encodedData = *pSrc;
			pSrc++;
			
			//Check for Done
			if(numBytesRead > maxInputStreamByteCount){
				break;
			}
			numBytesRead++;

			//Determine the history buffer index to start reading from
			baseIndex = ((encodedData & 0xF0) << 4) | inputData;
			numBytesToCopyFromHistory = (encodedData & 0xF) + 3;

			for(y = 0; y < numBytesToCopyFromHistory; y++){
				int tmpIndex;

				//Copy the indexed byte from scratch space to the destination 
				tmpIndex = (baseIndex + y) & 0xFFF;
				*pDst = pHistoryBuffer[tmpIndex];
				pDst++;

				//Copy the same byte to the next lru index in the history buffer 
				pHistoryBuffer[lruIndex] = pHistoryBuffer[tmpIndex];
				lruIndex++;
				lruIndex &= 0xFFF; // Roll over, only index 0 to 4095

				numBytesCopied++;
			}
		}

	}

	//Free Buffer
	free(pHistoryBuffer);

	//Return # of bytes copied to the destination
	rval = numBytesCopied;
	return rval;
}

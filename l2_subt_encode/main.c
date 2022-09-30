/*****************************************************************************/
/* Lunar 2 Subtitle Re-Encoder                                               */
/* ---------------------------                                               */
/* Program to re-encode new subtitles into Lunar 2 Saturn files.             */
/* An input file is used to specify the properties of the replacement        */
/* subtitles.                                                                */
/*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>

#define MAX_SIZE (1024*1024*8)


/*
File Format of Subtitle Files
==============================
# of sections in file:  4 bytes
Size of Section 1:      4 bytes
    Section 1 (bunch of 32-bit Words) 
	This is a sequence of commands to draw the subtitles (delays, fade in, fade out, etc)
Size of Section 2:      4 bytes
    Section 2 
	    16 short words (to be ored with 0x8000), which are copied to 25C7FFC0 as the color lookup table (clut)
		Size of Subtitle Header section (32-bit LW)
		Subtitle Hdr Offset#1:  32-bit LW  (00000028) {Add 060902B0 to get absolute address = 060902D8}
		    0000002C:  byte offset to subtitle data from subtitle hdr offset.
			00000009:  # of image data info structures that follow
			           Each structure is 32-bits and contains:
					       8-bit width/8  (mult by 8 to get width)
						   8-bit height
						   16-bit image offset in vram/8  (mult by 8 to get vram offset)
			00002DEB: length of compressed image data in bytes (At address 06090304)
			The image data follows here
		Subtitle Hdr Offset#2:  32-bit LW  (00002E44) {Add 060902B0 to get absolute address = 060930F4}
		                        If the value is 0xFFFFFFFF, then the second set does not exist.
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


int findRun(unsigned short* hbufIndex, unsigned short* runSize, unsigned char* pSrc, unsigned char* hBuff, int currentIndex);
unsigned int encodeSubtitles(unsigned int uncomprSize, unsigned char* pSrc, unsigned char* pDst);


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

	static char origfileName[300];
	static char reconfigName[300];	
	static char outfileName[300];
	int subtWidth[50];
	int subtHeight[50];
	int subtVRAMSize[50];
	FILE* infile, *outfile;
	int fsize,y,numSubtitles,tmpOffset;
	unsigned char* srcBuffer, *dstBuffer, *pDst, *tmpBuffer, *pStartSubtitleHdr;
	unsigned int sizeSection1, subtitleStartOffset;
	unsigned int cmprSize;
	unsigned int* pSection2Sizebytes;
	unsigned int* pSubtSec2RelAddr;
	unsigned short subtVRAMOffset;
	unsigned int offset;
	subTHdrType* pSubtImageInfo;
	unsigned int subtImgsStartOffset;
	unsigned int subtitleSectionOffset;
	unsigned int subtHdrSize, secondCmprDataSize;
	int nRead = 0;
	fsize = 0;
	
	/* 4 Commandline Arguments */
	if(argc != 4){
		printf("Error in arguments, Usage:  l2_subt_encode.exe origFname reconfigFname outputFname\n\n");
		return -1;
	}
	memset(origfileName,0,300);
	memset(reconfigName,0,300);
	memset(outfileName,0,300);
	strncpy(origfileName, argv[1],299);
	strncpy(reconfigName, argv[2],299);
	strncpy(outfileName, argv[3],299);

	/* Allocate memory for src/dst */
	srcBuffer = (unsigned char*)malloc(MAX_SIZE);
	dstBuffer = (unsigned char*)malloc(MAX_SIZE);
	tmpBuffer = (unsigned char*)malloc(MAX_SIZE);
	if(srcBuffer == NULL)
		printf("malloc error srcBuffer\n");
	if(dstBuffer == NULL)
		printf("malloc error dstBuffer\n");
	if(tmpBuffer == NULL)
		printf("malloc error tmpBuffer\n");
	memset(srcBuffer,0,MAX_SIZE);
	memset(dstBuffer,0,MAX_SIZE);
	memset(tmpBuffer,0,MAX_SIZE);
	
	/* Open and read the entire original file into memory */
	infile = fopen(origfileName,"rb");
	if(infile == NULL){
		printf("Error opening original input file.\n");
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
	
	/* For the destination buffer: */
	/* Copy all data up to this point directly to the destination  */
	/* Since this is just the first section that controls subtitle */
    /* timing, and fadein/fadeout behavior.                        */
	memcpy(dstBuffer,srcBuffer,8+sizeSection1);
	fsize += 8+sizeSection1;
	
	
	/* For the destination buffer: */
	/* Retain a pointer to the size of the second section, this */
	/* will get filled in at the end.                           */
	pSection2Sizebytes = (unsigned int*)&dstBuffer[fsize];
	pStartSubtitleHdr += 4;
	fsize+=4;
	subtitleStartOffset = 12+sizeSection1;
	
	/* Copy over the palette data to the destination buffer */
    memcpy(&dstBuffer[fsize],&srcBuffer[fsize],(16*2));
	fsize += 32;
	pStartSubtitleHdr += (16*2); 	
	
	
	/********************************************************/
	/* Now read in all new replacement input subtitle files */
	/* into memory as one continuous image.                 */
	/********************************************************/


	/* Open and read the configuration file */
	infile = fopen(reconfigName,"rb");
	if(infile == NULL){
		printf("Error opening config file.\n");
		return -1;
	}
	numSubtitles = 0;
	tmpOffset = 0;
	while(1){
		FILE* tmpfile;
		static char tmpFilename[300];
		static char line[300];		
		int tmpSize, width, height,num,rval;

		memset(tmpFilename,0,300);
		fgets(line,299,infile);
		if(feof(infile))
			break;
		rval = sscanf(line,"%d %d %d %s",&num,&width,&height,tmpFilename);
		printf(line);

		/* Parameter #1 */
		if(line[0] == '#') //comment
			continue;
		else if(rval != 4)
			break;
		else if(num != (numSubtitles+1)){
			printf("Error in config file, ignoring line.\n");
			continue;
		}
		
		/* Width/8, Height, VRAM Size (round up to nearest 8 bytes) and div 8 */
		subtWidth[numSubtitles]  = width / 8;
		subtHeight[numSubtitles] = height;
		subtVRAMSize[numSubtitles] = (width*height*4/8);
		subtVRAMSize[numSubtitles] /= 8;
		if((subtVRAMSize[numSubtitles] % 4) != 0)
			subtVRAMSize[numSubtitles] += (4 - (subtVRAMSize[numSubtitles] % 4));

		
		/* Open and append the file to the temporary buffer */
		tmpfile = fopen(tmpFilename,"rb");
		if(tmpfile == NULL){
			printf("Error opening input file %s.\n",tmpFilename);
			return -1;
		}
		fseek(tmpfile,0,SEEK_END);
		tmpSize = ftell(tmpfile);
		fseek(tmpfile,0,SEEK_SET);
		nRead += fread(&tmpBuffer[tmpOffset],1,tmpSize,tmpfile);
		fclose(tmpfile);
		
		/* Update Offset/Size of Data             */
		/* & Round up (I think this is required?) */
//		printf("Temporary Buffer Start Offset = 0x%X\n",tmpOffset);
		tmpOffset += subtVRAMSize[numSubtitles] * 8;
//		if((subtVRAMSize[numSubtitles] % 4) != 0)
//			tmpOffset += (4 - (subtVRAMSize[numSubtitles]%4)) * 8;

		numSubtitles++;
	}
//	printf("Temporary Buffer Size = 0x%X\n",tmpOffset);
	fclose(infile);
	

	/* All subtitle data has been read into tmpBuffer */
	/* and tmpOffset holds the size of the data.      */
	/* Now its time to compress the data.             */
	pDst = (unsigned char*)malloc(tmpOffset*2);
	if(pDst == NULL){
		printf("Malloc error for compressed data.\n");
		return -1;
	}
	                      /* size_bytes , uncmpro_src, cmpr_dst*/
	cmprSize = encodeSubtitles(tmpOffset, tmpBuffer, pDst);


	/*********************************************************************/
	/* Now write out the header data associated with the subtitle images */
	/* Overwrite the existing subtitle data with the new data from the   */
	/* input file.                                                       */
	/*********************************************************************/
	subtitleSectionOffset = *((unsigned int*)pStartSubtitleHdr);
	
	/* Uint32 - Copy the Subtitle Section #1 Relative Start Address, wont change */
	/* This is an address relative to the previously stored subtitleStartOffset  */
	memcpy(&dstBuffer[fsize],&srcBuffer[fsize],4);
	fsize += 4;
	pStartSubtitleHdr += (4); 

	//Account for Subtitle Section #2 relative start address, fill in later.
	pSubtSec2RelAddr = (unsigned int*)(&dstBuffer[fsize]);
	*pSubtSec2RelAddr = 0xFFFFFFFF;
	fsize+=4;

	/* Convert to LE */
	swap32(&subtitleSectionOffset);
	if(subtitleSectionOffset != 0xFFFFFFFF) /* If 0xFFFFFFFF, section does not exist */
	{
		subtitleSectionOffset += subtitleStartOffset;

		/* Next two 32-bit words are the relative byte offset to the compressed data */
		/* And the # of subtitles images contained within the compressed data        */
		/* These should be copied verbatim.                                          */
		memcpy(&dstBuffer[subtitleSectionOffset], &srcBuffer[subtitleSectionOffset], 4*2);
		fsize += 8;

		/* Convert to LE */
		pSubtImageInfo = (subTHdrType*)(&srcBuffer[subtitleSectionOffset]);
		swap32(&pSubtImageInfo->rel_byteOffsetToData);
		swap32(&pSubtImageInfo->numImagesInHdr);
		
		/* Write out updated subtitle information */
		if((int)pSubtImageInfo->numImagesInHdr != numSubtitles)
			printf("Warning, # new subtitles does not match existing.\n");
		subtVRAMOffset = 0;
		for(y = 0; y < (int)pSubtImageInfo->numImagesInHdr; y++){
			
			memcpy(&dstBuffer[subtitleSectionOffset+8+y*4+0], &subtWidth[y],1);
			memcpy(&dstBuffer[subtitleSectionOffset+8+y*4+1], &subtHeight[y],1);

			//VRAM Offset
			swap16(&subtVRAMOffset);
			memcpy(&dstBuffer[subtitleSectionOffset+8+y*4+2], &subtVRAMOffset,2);
			swap16(&subtVRAMOffset);
			subtVRAMOffset += subtVRAMSize[y];

			fsize+=4;  //This loop iteration added 4 bytes to the output file
		}
		
		/* Write the 32-bit size of the compressed data, followed by the compressed data */
		subtImgsStartOffset = subtitleSectionOffset + pSubtImageInfo->rel_byteOffsetToData;
		offset = subtImgsStartOffset;
		swap32(&cmprSize);
		*((unsigned int*)(&dstBuffer[offset])) = cmprSize;
		swap32(&cmprSize);
		offset += 4; fsize += 4;

		/* Round up compressed data written to 4 byte boundary */
		if((cmprSize % 4) != 0)
			cmprSize += 4 -(cmprSize%4);
		memcpy(&dstBuffer[offset], pDst, cmprSize);
		fsize += cmprSize;
	}
	

	/*****************************************************************************************/
	/* The second set of data is for the optional background image and will be a direct copy */
	/* from the existing compressed data.                                                    */
	/*****************************************************************************************/
	subtitleSectionOffset = *((unsigned int*)pStartSubtitleHdr);
	
	/* Convert to LE */
	swap32(&subtitleSectionOffset);
	if(subtitleSectionOffset != 0xFFFFFFFF) /* If 0xFFFFFFFF, section does not exist */
	{
		subtitleSectionOffset += subtitleStartOffset;

		/* Uint32 - Update the Subtitle Section #2 Relative Start Address for the DST */
		/* This is an address relative to the previously stored subtitleStartOffset   */
		/* (0x20, pallete data) + (0x04, 1st subt section offset) + (0x04, 2nd subt section offset) + */
		/* (0x04, Size 1st subt hdr) + (0x04, # subt infos) + (0x04 * # subt infos) +  */
		/* (0x04, Size 1st subt compr data) + (size_1st_subt_compr_data)               */
		*pSubtSec2RelAddr = 0x20 + 0x8 + 0x4 + 0x4 + (pSubtImageInfo->numImagesInHdr * 0x4) + 0x4 + cmprSize;
		swap32(pSubtSec2RelAddr);


		/* Jump Forward to the Subtitle 2 Section now.  This will be copied verbatim */
		/* Next 32-bit word in the src buffer is the size in bytes of the 2nd subt header */
		memcpy(&dstBuffer[fsize], &srcBuffer[subtitleSectionOffset], 4);
		fsize += 4;

		/* Copy the whole header (# of subtitles, subtitle hdr info, size compr data) */
		subtHdrSize = *((unsigned int*)&srcBuffer[subtitleSectionOffset]);
		swap32(&subtHdrSize);
		subtitleSectionOffset += 4;
		memcpy(&dstBuffer[fsize], &srcBuffer[subtitleSectionOffset], subtHdrSize);
		fsize += subtHdrSize;
		subtitleSectionOffset += subtHdrSize;

		/* Grab the compressed data size */
		secondCmprDataSize = *((unsigned int*)&srcBuffer[subtitleSectionOffset-4]);
		swap32(&secondCmprDataSize);

		/* Copy the compressed data */
		memcpy(&dstBuffer[fsize], &srcBuffer[subtitleSectionOffset], secondCmprDataSize);
		fsize += secondCmprDataSize;
	}

	/* Update Size of File Section #2 */
	if((fsize % 4) != 0){
		fsize += 4 - (fsize % 4) ;
	}
	*pSection2Sizebytes = fsize - subtitleStartOffset;
	swap32(pSection2Sizebytes);


	//Add 2k padding
	if((fsize % 2048) != 0){
		fsize += 2048 - (fsize % 2048) ;
	}

	/* Open the output file to write out the data */
	outfile = fopen(outfileName,"wb");
	if(outfile == NULL){
		printf("Error opening output file.\n");
		return -1;
	}
	
	fwrite(dstBuffer,1, fsize,outfile);
	fclose(outfile);

	printf("File Re-Encoding Complete!\n");

	/* Free resources */
	free(srcBuffer);
	free(dstBuffer);
	free(tmpBuffer);
	free(pDst);

	return 0;
}

//findRun - Finds a run somewhere in the history buffer between 3 and 18
//Keeps searching until the largest such run is located
//Note that the history buffer wraps around after location 0xFFF
//Returns 0 if one is found and -1 otherwise
//hbufIndex and runSize are updated if one is found
#if 0
int findRun(unsigned short* hbufIndex, unsigned short* runSize, unsigned char* pSrc, unsigned char* hBuff, int currentIndex){
	
	int x,tRunSize,baseIndex, numTests;
	
	/* Init */
	*hbufIndex = 0;
	*runSize = 0;

	baseIndex = (currentIndex+18) & 0xFFF;
	numTests = 0x1000;
	for(; numTests > 0; baseIndex++){
		baseIndex &= 0xFFF;
		tRunSize = 0;
		for(x=0;x<18;x++){

			int testLocation = (baseIndex+x)&0xFFF;
			//Ran into next history buffer slot, will not work
			if(testLocation == currentIndex)
				break;

			if(pSrc[x] == hBuff[(baseIndex+x) & 0xFFF]){
				tRunSize++;

				//Copying the current run will cause a problem
				if(((currentIndex + tRunSize)&0xFFF) == baseIndex){
					tRunSize = 0;
					break;
				}
			}
			else
				break;
		}
		
		if((tRunSize >= 3) && (tRunSize >= *runSize)){
			*runSize = tRunSize;
			*hbufIndex = baseIndex;
		}
		numTests--;
	}

	//Return -1 if no runs >= 3 were located
	if(*runSize == 0)
		return -1;
	
	return 0;
}
#endif
int findRun(unsigned short* hbufIndex, unsigned short* runSize, unsigned char* pSrc, unsigned char* hBuff, int currentIndex){
	
	int x,tRunSize,baseIndex, numTests;
	
	/* Init */
	*hbufIndex = 0;
	*runSize = 0;

	baseIndex = currentIndex & 0xFFF;
	numTests = 0x1000;
	for(; numTests > 0; baseIndex++){
		baseIndex &= 0xFFF;
		tRunSize = 0;
		for(x=0;x<18;x++){

			int testLocation = (baseIndex+x)&0xFFF;
			
			//Ran into next history buffer slot, will not work
			if((x!=0) && (testLocation == currentIndex))
				break;

			if(pSrc[x] == hBuff[(baseIndex+x) & 0xFFF]){
				tRunSize++;

				//Copying the current run will cause a problem
//				if(((currentIndex + tRunSize)&0xFFF) == baseIndex){
//					tRunSize = 0;
//					break;
//				}
			}
			else
				break;
		}
		
		if((tRunSize >= 3) && (tRunSize >= *runSize)){
			*runSize = tRunSize;
			*hbufIndex = baseIndex;
		}
		numTests--;
	}

	//Return -1 if no runs >= 3 were located
	if(*runSize == 0)
		return -1;
	
	return 0;
}





// encodeSubtitles
// LRU compression algorithm with a 4kB rolling history buffer
// Returns # of valid bytes copied to the destination buffer (the compressed data).
unsigned int encodeSubtitles(unsigned int uncomprSize, unsigned char* pSrc, unsigned char* pDst){

	unsigned int comprSizeBytes = 0;
	unsigned char* pHistoryBuffer, *pSelectionByte, *pEnd;
	int lruIndex;
	unsigned int x, y;
//	unsigned char tmpBuff[32];

	//Allocate a history buffer of 4096 bytes and zero it
	pHistoryBuffer = (unsigned char*)malloc(0x1000);
	if(pHistoryBuffer == NULL){
		printf("Error allocating scratch buffer.\n");
	}
	memset(pHistoryBuffer,0,0x1000);

//	printf("uncomprSize=0x%X\n",uncomprSize);

	/* Init */
	lruIndex = 0xFEE;
	x = 0;

//	printf("Start is 0x%X\n",(unsigned int)pSrc);
	
	pEnd = pSrc + uncomprSize;
	while(pSrc < pEnd){
		int rval;
		unsigned char selectionMask;

		/////////////////////////////////////////////////////////////////////////////
		//For the next 8 sets of compressed data, check for a run of at 
		//least 3 bytes and at most 18 bytes that matches a location in the history buffer.
		//If a run is found: 
		//  Write out a two-byte header containing the following information, 
		//    Byte 0 (07-00): Lower 8-bits of the 12-bit history buffer index
		//    Byte 1 (07-04): Upper 4 bits of 12-bit index into history buffer.
		//    Byte 1 (03-00): (# of bytes to copy from the history buffer) - 3
		//  Then copy this data to the LRU position in the history buffer.
		//If a run is not found:
		//  Copy 1 byte from the input stream to the destination and add it to 
		//  the history buffer at the LRU position.
		//
		//Finally update the selection bit in the selection byte. (x=0 is bit 0, x=1 is bit 1, etc)
		//The selection bit is 1 for a direct 1-byte copy and 0 for a History Buffer n-byte copy
		/////////////////////////////////////////////////////////////////////////////
		
		pSelectionByte = pDst;
		*pSelectionByte = 0x00; /* Init to zero */
		pDst++;
		comprSizeBytes++;
		selectionMask = 0x1;
		for(x = 0; x < 8; x++){

			unsigned short hbufIndex, runSize;
			
			//Search for longest history buffer run >= 3 bytes
			rval = findRun(&hbufIndex, &runSize, pSrc, pHistoryBuffer, lruIndex);

			//Update based on # bytes left to encode
			if((rval >= 0) && (runSize > (int)(pEnd-pSrc))){
				runSize = pEnd - pSrc;
				if(runSize < 3)
					rval = -1;
			}

			//Determine Single vs. Direct Byte Copy
			if(rval < 0){
				//Not Found, Single Direct Byte Copy
				*pSelectionByte = *pSelectionByte | selectionMask;
				*pDst = *pSrc;
				comprSizeBytes++;
//				printf("DST = %2X\n",*pDst);

				//Copy the same byte to the next lru index in the history buffer 
				pHistoryBuffer[lruIndex] = *pSrc;
				lruIndex++;
				lruIndex &= 0xFFF; // Roll over, only index 0 to 4095
			
				//Update Src/Dst Pointers
				pDst++; pSrc++;
			}
			else{
				//Found, History Buffer Copy

				//Write history buffer index & (runSize-3)
				*pDst = (hbufIndex & 0xFF);
				pDst++;
				comprSizeBytes++;
				*pDst = ((hbufIndex & 0x0F00) >> 4) | ((runSize-3) & 0xF);
//				printf("HDST = %2X%2X\n",*(pDst-1),*pDst);
				pDst++;
				comprSizeBytes++;

				//Verification
//				for(y = 0; y < runSize; y++){
//					tmpBuff[y] = pHistoryBuffer[(hbufIndex+y)&0xFFF];
//					printf("tmpBuff = 0x%2X\n",tmpBuff[y]);
//				}
				
				//Update History Buffer
				for(y = 0; y < runSize; y++){
					//Copy the same byte to the next lru index in the history buffer 
					pHistoryBuffer[lruIndex] = pHistoryBuffer[hbufIndex];
//					printf("pHistoryBuffer[%d] = pHistoryBuffer[%d] = 0x%2X\n",lruIndex,hbufIndex,pHistoryBuffer[hbufIndex]);
					lruIndex++;
					lruIndex &= 0xFFF; // Roll over, only index 0 to 4095
					hbufIndex++;
					hbufIndex &= 0xFFF;
				}

//              Verification
//				for(y = 0; y < runSize; y++){
//					if(tmpBuff[y] != pHistoryBuffer[(hbufIndex-runSize+y)&0xFFF])
//						printf("Potential Issue Detected. pSrc = 0x%X\n",(unsigned int)pSrc);
//				}

				//Update Src Location
				pSrc += runSize;
			}
						
			//Update Mask for Selection Bit
			selectionMask <<= 1;

			//Check for Exit
			if(pSrc >= pEnd)
				break;
		}
	}

	//Free Buffer
	free(pHistoryBuffer);

	//Make compressed bytes a multiple of 4 bytes by zeroing some extra ones. 
	//Dont update # compressed bytes though since these arent part of the decompression stream	
	if((comprSizeBytes%4) != 0){
		int remainderBytes = 4 -(comprSizeBytes%4);
		for(x = 0; x < (unsigned int)remainderBytes; x++){
			*pDst = 0x00;
			pDst++;
		}
	}

	//Return # of bytes copied to the destination
	return comprSizeBytes;
}

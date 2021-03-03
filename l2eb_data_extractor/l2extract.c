/***********************************************************************/
/* Lunar2 Extractor for Sega Saturn                                    */
/*                                                                     */
/* Usage                                                               */
/* ========================================                            */
/* l2extract.exe file1_name file2_name outputDirectory                 */
/*                                                                     */
/* The two input files should be "1" and "2" on the CD.                */
/*                                                                     */
/* Credit on the format goes to suppertails66, who figured it out for  */
/* PSX.  Happens to be the same on Saturn, but with no file to specify */
/* any sort of filenames...                                            */
/***********************************************************************/
#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BYTES_PER_SECTOR 0x800

void swap32(char* ptr){

	char temp[4];
	temp[0] = ptr[3];
	temp[1] = ptr[2];
	temp[2] = ptr[1];
	temp[3] = ptr[0];

	memcpy(ptr,temp,4);
}



/******************************************************************************/
/* printUsage() - Display command line usage of this program.                 */
/******************************************************************************/
void printUsage(){
    printf("Error in input arguments.\n");
    printf("Usage:\n=========\n");
	printf("l2extract.exe file1_name file2_name outputDirectory\n\n");
    return;
}



int main(int argc, char** argv){

    FILE* inFile1, *inFile2, * outfile;
	char* buffer;
	static char fname1[300];
	static char fname2[300];
	static char outname[300], outpath[300];
	unsigned int startOffset, length, numSec;
	int numRead = 0;
	int x = 0;
    memset(fname1,0,300);
	memset(fname2,0,300);
	
	/* Check input args */
	if (argc != 4){
		printUsage();
		return -1;
	}

	strcpy(fname1,argv[1]);
	strcpy(fname2,argv[2]);
	strcpy(outpath,argv[3]);
	if(outpath[strlen(outpath)-1] != '\\'){
		strcat(outpath,"\\");
	}
	
	/* Open file 1, file with offsets */
	inFile1 = fopen(fname1,"rb");
	if(inFile1 == NULL){
		printf("Error opening %s\n",fname1);
        return -1;
	}
	
	/* Open file 2, file with data */
	inFile2 = fopen(fname2,"rb");
	if(inFile2 == NULL){
		printf("Error opening %s\n",fname2);
        return -1;
	}

	while(1){
		char tmpName[32];
		
		//Construct Output Filename
		memset(outname,0,300);
		memset(tmpName,0,32);
		x++;
		sprintf(tmpName,"%d",x);
		sprintf(outname,"%s%s",outpath,tmpName);

		//Read 3 bytes to get Start Offset (Big Endian)
		startOffset = 0;
		numRead = fread(&startOffset,1,3,inFile1);
		if(numRead != 3) //Exit condition
			break;
		startOffset <<= 8;
		swap32((char*)&startOffset);
		startOffset *= BYTES_PER_SECTOR;
		
		//Read 2 bytes to get Num Sectors  (Big Endian)
		numSec = 0;
		fread(&numSec,1,2,inFile1);
		numSec <<= 16;
		swap32((char*)&numSec);

		//Length
		length = numSec * BYTES_PER_SECTOR;

		//Create File
		printf("Creating %s\n\t(Start=0x%X,Nsec=0x%X(%fkB),Length=0x%X(%f kB))\n",outname,startOffset,numSec,numSec*0x800/1024.0,length,(float)length/1024.0);

		buffer = NULL;
		buffer = (char*)malloc(length);
		if(buffer == NULL){
			printf("Buffer allocation error.\n");
			return -1;
		}
		outfile = fopen(outname,"wb");
		if(outfile == NULL){
		    printf("Error creating %s\n",outname);
			return -1;
		}
		fseek(inFile2,startOffset,SEEK_SET);
		fread(buffer,sizeof(char),length,inFile2);
		fwrite(buffer,sizeof(char),length,outfile);
		fclose(outfile);
		free(buffer);
	}
	printf("Lunar2 Extraction Complete!\n");

	return 0;
}

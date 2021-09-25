/***********************************************************************/
/* Lunar2 Recombiner for Sega Saturn                                   */
/*                                                                     */
/* Usage                                                               */
/* ========================================                            */
/* l2combine.exe inputPath num_files outputPath                        */
/*                                                                     */
/* The input files should be located in "inputDirectory" and should be */
/* numbered 1 to n.                                                    */
/* The two output files will be names "1" and "2", just as they are    */
/* on the cd.                                                          */
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
	printf("l2combine.exe inputPath numFiles outputPath\n\n");
    return;
}



int main(int argc, char** argv){

    FILE* outFile1, *outFile2, *log, *inFile;
	static char outpath[300], inpath[300], infname[300],
		outname1[300], outname2[300];
	char* buffer;
	unsigned int offset;
	int numFiles;
	int x = 0;
	
	memset(outname1,0,300);
	memset(outname2,0,300);
	memset(outpath,0,300);
	memset(inpath,0,300);
	
	/* Check input args */
	if (argc != 4){
		printUsage();
		return -1;
	}

	strcpy(inpath,argv[1]);
	if(inpath[strlen(inpath)-1] != '\\'){
		strcat(inpath,"\\");
	}
	numFiles = atoi(argv[2]);
	strcpy(outpath,argv[3]);
	if(outpath[strlen(outpath)-1] != '\\'){
		strcat(outpath,"\\");
	}
	
	
	/*********************/
	/* Open output files */
	/*********************/
	
	/* Open file 1, file with offsets */
	sprintf(outname1,"%s1",outpath);
	outFile1 = fopen(outname1,"wb");
	if(outFile1 == NULL){
		printf("Error opening file \"1\" for writing offsets\n");
        return -1;
	}
	
	/* Open file 2, file with data */
	sprintf(outname2,"%s2",outpath);
	outFile2 = fopen(outname2,"wb");
	if(outFile2 == NULL){
		printf("Error opening file \"2\" for writing data\n");
        fclose(outFile1);
		return -1;
	}
	
	/* Open the log */
	log = fopen("log.txt","w");
	if(log == NULL){
		printf("Error creating log.txt\n");
		fclose(outFile1);
		fclose(outFile2);
		return -1;
	}

	buffer = malloc(1024*1024);
	if(buffer == NULL){
		printf("Malloc failure\n");
		fclose(outFile1);
		fclose(outFile2);
		fclose(log);
		return -1;
	}


	/*******************************************************************/
	/* Iterate through the input files, updating the offsets in file 1 */
	/* and writing the data to file 2                                  */
	/*******************************************************************/
	offset = 0;
	for(x=0; x < numFiles; x++){
		unsigned char tval;
		unsigned short numSectors;
		unsigned int fsize, length, sectorOffset;
		int numRead;

		//Construct Input Filename
		memset(infname,0,300);
		sprintf(infname,"%s%d", inpath,x+1);
		
		//Open the input file
		inFile = fopen(infname,"rb");
		if(inFile == NULL){
			printf("Error opening input file %s for reading\n",infname);
			fclose(outFile1);
			fclose(outFile2);
			fclose(log);
			free(buffer);
			return -1;
		}
		
		//Determine the size of the input file in bytes, convert to sectors
		fseek(inFile,0,SEEK_END);
		fsize = ftell(inFile);
		fseek(inFile,0,SEEK_SET);
		numSectors = fsize / BYTES_PER_SECTOR;
		if((numSectors*BYTES_PER_SECTOR) != fsize){
			numSectors++;
			printf("Note: Adding a sector remainder.\n");
		}
		length = numSectors*BYTES_PER_SECTOR;

		//For File#1:  Write the 3-byte start offset, followed by the 2 byte #Sectors
		sectorOffset = offset / BYTES_PER_SECTOR;
		tval = (sectorOffset >> 16) & 0xFF;
		fwrite(&tval,1,1,outFile1);
		tval = (sectorOffset >> 8) & 0xFF;
		fwrite(&tval,1,1,outFile1);
		tval = (sectorOffset) & 0xFF;
		fwrite(&tval,1,1,outFile1);

		tval = (numSectors >> 8) & 0xFF;
		fwrite(&tval,1,1,outFile1);
		tval = (numSectors) & 0xFF;
		fwrite(&tval,1,1,outFile1);


		//For File#2:  Write out all of the actual data
		fseek(outFile2,offset,SEEK_SET);
		numRead = 1024*1024;
		while(numRead == (1024*1024)){
			numRead = fread(buffer,1,1024*1024,inFile);
			fwrite(buffer,1,numRead,outFile2);
		}

		//Close input file.
		fclose(inFile);

		//Print Status
		printf("Added %s\n\t(Start=0x%X,Nsec=0x%X(%fkB),Length=0x%X(%f kB))\n",infname,offset,numSectors,numSectors*BYTES_PER_SECTOR/1024.0,length,(float)length/1024.0);
		fprintf(log,"Added %s\n\t(Start=0x%X,Nsec=0x%X(%fkB),Length=0x%X(%f kB))\n",infname,offset,numSectors,numSectors*BYTES_PER_SECTOR/1024.0,length,(float)length/1024.0);

		//Update for next offset
		offset += length;
	}
	free(buffer);
	fclose(log);
	fclose(outFile1);
	fclose(outFile2);
	printf("Lunar2 Recombining Complete!\n");

	return 0;
}

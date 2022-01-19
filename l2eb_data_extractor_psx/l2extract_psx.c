/***********************************************************************/
/* Lunar2 Extractor for PSX                                            */
/*                                                                     */
/* Usage                                                               */
/* ========================================                            */
/* l2extract_psx.exe file1_name file2_name file3_name outputDirectory  */
/*                                                                     */
/* The three input files should be "data.idx", "data.pak", and         */
/* "data.upd" on the CD.                                               */
/*                                                                     */
/* Credit on the format goes to suppertails66, who figured it out for  */
/* PSX.  Some code taken from his extraction routine.                  */
/***********************************************************************/
#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif
#if defined(__linux__) || defined(__APPLE__)
  #include <sys/stat.h>
#elif _WIN32
  #include <windows.h>
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

void swap16(char* ptr){

	char temp[2];
	temp[0] = ptr[1];
	temp[1] = ptr[0];

	memcpy(ptr,temp,2);
}



/******************************************************************************/
/* printUsage() - Display command line usage of this program.                 */
/******************************************************************************/
void printUsage(){
    printf("Error in input arguments.\n");
    printf("Usage:\n=========\n");
	printf("l2extract_psx.exe file1_name file2_name file3_name outputDirectory\n\n");
    return;
}

#include <io.h>     // For access().
#include <sys/types.h>  // For stat().
#include <sys/stat.h>   // For stat().

void createDirectory(char* name);
int DirectoryExists( const char* absolutePath );
void createDirectoriesForFile(char* name);

void createDirectoriesForFile(char* name) {
	
    char tmpname[300];
	int x = 0;
	int namelen = strlen(name);
	if(namelen == 0)
		return;
	
	memset(tmpname,0,300);
	tmpname[x] = name[x];
	while(tmpname[x] != '\0'){
		x++;
		
		if(name[x] == '/'){
			//printf("%s\n",tmpname);
			if(!DirectoryExists(tmpname)){
				createDirectory(tmpname);
			}
		}
		tmpname[x] = name[x];
	}

	return;
}

int DirectoryExists( const char* absolutePath ){

    if( _access( absolutePath, 0 ) == 0 ){

        struct stat status;
        stat( absolutePath, &status );

        return (status.st_mode & S_IFDIR) != 0;
    }
    return 0;
}


void createDirectory(char* name) {
  if (strlen(name) != 0) {
    // first, create every parent directory, since mkdir
    // will not create subdirectories
  //createDirectory(getDirectory(name));
    
#if defined(__linux__) || defined(__APPLE__)
      mkdir(name, S_IRWXU|S_IRWXG|S_IRWXO);
#elif _WIN32
	  CreateDirectoryA(name, NULL);
#endif
  }
}




int main(int argc, char** argv){

    FILE* inFile1, *inFile2, *inFile3, * outfile, *log;
	char* buffer, *pStartOffset;
	static char fname1[300];
	static char fname2[300];
	static char fname3[300];
	static char outname[300], outpath[300];
	unsigned int startOffset, length, name_offset;
	unsigned short numSec;
	int numRead = 0;
	int x = 0;
    memset(fname1,0,300);
	memset(fname2,0,300);
	memset(fname3,0,300);
		
	/* Check input args */
	if (argc != 5){
		printUsage();
		return -1;
	}

	strcpy(fname1,argv[1]);
	strcpy(fname2,argv[2]);
	strcpy(fname3,argv[3]);
	strcpy(outpath,argv[4]);
	if(outpath[strlen(outpath)-1] == '\\'){
		outpath[strlen(outpath)-1] = '/';
	}
	if(outpath[strlen(outpath)-1] != '/'){
		strcat(outpath,"/");
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
	
	/* Open file 3, file with file names */
	inFile3 = fopen(fname3,"rb");
	if(inFile3 == NULL){
		printf("Error opening %s\n",fname3);
        return -1;
	}
	
	/* Open the log */
	log = fopen("log.txt","w");
	if(log == NULL){
		printf("Error creating log.txt\n");
		return -1;
	}
	
	/* Jump to the filenames section in the UPD file */
	fseek(inFile3,0x10,SEEK_SET);
	fread(&name_offset,4,1,inFile3);
	fseek(inFile3,name_offset,SEEK_SET);

	while(1){
		static char tmpName[300];
		
		//Construct Output Filename
		memset(outname,0,300);
		memset(tmpName,0,300);
		
		/* Read in filename */
		x = 0;
		do{
			int rval;
			rval = fread(&tmpName[x],1,1,inFile3);
			if(rval != 1){
				//printf("Name read error detected\n");
				break;
			}
			if(tmpName[x] == '\\') /* Backslash replacement */
				tmpName[x] = '/';
			x++;
		}while(tmpName[x-1] != '\0');
		//sprintf(tmpName,"%d",x);
		sprintf(outname,"%s%s",outpath,tmpName);

		//Read 3 bytes to get Start Offset (Big Endian)
		startOffset = 0;
		pStartOffset = (char*)&startOffset;
		pStartOffset++;
		numRead = fread(pStartOffset,1,3,inFile1);
		if(numRead != 3) //Exit condition
			break;
//		startOffset <<= 8;
		swap32((char*)&startOffset);
		startOffset *= BYTES_PER_SECTOR;

		//Read 2 bytes to get Num Sectors  (Big Endian)
		numSec = 0;
		fread(&numSec,1,2,inFile1);
		swap16((char*)&numSec);

		//Length
		length = numSec * BYTES_PER_SECTOR;

		//Create File
		printf("Creating %s\n\t(Start=0x%X,Nsec=0x%X(%fkB),Length=0x%X(%f kB))\n",outname,startOffset,numSec,numSec*0x800/1024.0,length,(float)length/1024.0);
		fprintf(log,"Creating %s\n\t(Start=0x%X,Nsec=0x%X(%fkB),Length=0x%X(%f kB))\n",outname,startOffset,numSec,numSec*0x800/1024.0,length,(float)length/1024.0);

        //Make Any Required Directories
		createDirectoriesForFile(outname);
#if 1
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
		fread(buffer,1,length,inFile2);
		fwrite(buffer,1,length,outfile);
		fclose(outfile);
		free(buffer);
#endif
	}

	fclose(log);
	fclose(inFile1);
	fclose(inFile2);
	fclose(inFile3);
	printf("Lunar2 PSX Extraction Complete!\n");

	return 0;
}

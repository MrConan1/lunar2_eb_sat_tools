/******************************************************************************/
/* main.c - Main execution file for preliminary Lunar 2 Monster Decoder       */
/*          l2_monster_decode.exe                                             */
/*          For use with Sega Saturn Lunar 2 Files only.                      */
/******************************************************************************/

/***********************************************************************/
/* Lunar 2 Monster Decoder (l2_mnst_decode.exe) Usage                  */
/* ==================================================                  */
/* l2_txt_decode.exe decode InputFname                                 */
/*                                                                     */
/* InputFname should have a listing of files to be decoded, one per    */
/* line                                                                */
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
    printf("l2_mnst_decode.exe InputFname\n");
    printf("\n\n");
    return;
}


/******************************************************************************/
/* main()                                                                     */
/******************************************************************************/
int main(int argc, char** argv){

    FILE *inFile, *iFile,*outFile;
    static char inFileName[300];
	static char finputName[300];
    static char outFileName[300];
	static char inputname[300];
	unsigned short dlogWord;
	char data[10];
	int x;
    int rval, numFiles;
	unsigned int textOffset;
    rval = -1;
	
    printf("Lunar2 Monster Decoder v%d.%02d\n", VER_MAJ, VER_MIN);

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
	strcpy(outFileName,"monster_names.txt");


    
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
        printf("Error occurred while opening input file %s for reading\n", inFileName);
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
	fscanf(inFile,"%d",&numFiles);
	
	for(x = 0; x < numFiles; x++){
		int monsterNameLength, textSizeBytes, numMonsterNames,monsterNum;

		/* Read the filename */
		fscanf(inFile,"%s",finputName);
		
		/* Open the file */
		iFile = NULL;
		sprintf(inputname,"input/%s",finputName);
		iFile = fopen(inputname, "rb");
		if (iFile == NULL){
			printf("Error occurred while opening input file %s for reading\n", inputname);
			return -1;
		}
		fprintf(outFile,"File: %s\n",finputName);
		printf("File: %s\n",finputName);
		
		/* Read the offset to start of the Monster Text Section */
		fseek(iFile,0x4,SEEK_SET);
		fread(&textOffset,1,4,iFile);
		swap32(&textOffset);
		textOffset += 8;    /* Add 8 since its a relative offset from here */
		fprintf(outFile,"Monster Name Offset: 0x%X\n",textOffset);
		printf("Monster Name Offset: 0x%X\n",textOffset);
		
		/* Read in the # of bytes for monster names */
		fseek(iFile,textOffset,SEEK_SET);
		fread(&textSizeBytes,1,4,iFile);
		swap32(&textSizeBytes);
		fprintf(outFile,"Size (Bytes): %d\n",textSizeBytes);
		printf("Size (Bytes): %d\n",textSizeBytes);
		
		/* Calculate # of monsters */
		numMonsterNames = textSizeBytes / 12;
		fprintf(outFile,"%d Monsters\n",numMonsterNames);
		printf("%d Monsters\n",numMonsterNames);
		
		monsterNameLength = 0;
		monsterNum = 0;

		while(textSizeBytes > 0){
			if(monsterNameLength == 0){
				fprintf(outFile,"\tMonster %d: \"",monsterNum);
				printf("\tMonster %d: \"",monsterNum);
			}

			fread(&dlogWord,1,2,iFile);
			if(feof(iFile)){
				fprintf(outFile,"\nError, EOF Detected.\n");
				printf("\nError, EOF Detected.\n");
				break;
			}
			swap16(&dlogWord);
			
			if(dlogWord == 0x9010){
				printf(" ");
				fprintf(outFile," ");
			}
			else{
				memset(data,0,10);
				if(getUTF8character((int)dlogWord, data) >= 0){
					printf("%s",data);
					fprintf(outFile,"%s",data);
				}
			}

			monsterNameLength++;
			if(monsterNameLength == 12){
				monsterNameLength = 0;
				fprintf(outFile,"\"\n");
				printf("\"\n");
				monsterNum++;
			}
			textSizeBytes--;
			textSizeBytes--;
		}
		printf("\n");
		fprintf(outFile,"\n");

		fclose(iFile);
	}
	

    /* Close files */
    fclose(inFile);
	fclose(outFile);

    return 0;
}

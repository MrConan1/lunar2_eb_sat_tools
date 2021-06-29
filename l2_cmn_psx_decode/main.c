/******************************************************************************/
/* main.c - Main execution file for Lunar 2 PSX Common Msg Decoder            */
/******************************************************************************/

/***********************************************************************/
/* Lunar 2 PSX Common Msg Decoder Usage                                */
/* ==================================================                  */
/* l2_cmn_psx_decode.exe                                               */
/*                                                                     */
/* Input File  (SYSSPR.PCK renamed US_SYSSPR.PCK) should be in same    */
/* directory as this exe.  Yeah, this code is sloppy.                  */
/***********************************************************************/


/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define VER_MAJ    0
#define VER_MIN    1

/******************************************************************************/
/* printUsage() - Display command line usage of this program.                 */
/******************************************************************************/
void printUsage(){
    printf("Error in input arguments.\n");
    printf("Usage:\n===============================\n");
    printf("l2_cmn_psx_decode.exe\n");
    printf("\n\n");
    return;
}


/******************************************************************************/
/* main()                                                                     */
/******************************************************************************/
int main(int argc, char** argv){

    FILE *inFile, *outFile;
    static char inFileName[300];
    static char outFileName[300];
	unsigned char buffer[2];
    int rval;
	unsigned int textStart;
    rval = -1;
	textStart = 0;
	
    printf("Lunar2 PSX Common Text Decoder v%d.%02d\n", VER_MAJ, VER_MIN);

    /**************************/
    /* Check input parameters */
    /**************************/



    //Handle Generic Input Parameters
    memset(inFileName, 0, 300);
	memset(outFileName, 0, 300);
	strcpy(inFileName,"US_SYSSPR.PCK");
	strcpy(outFileName,"L2EB_US_Common_Text.txt");


    

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
	fseek(inFile,0x2AA6C,SEEK_SET);

	while(ftell(inFile) < 0x3158C){
		
		/* Read the file */
		rval = fread(&buffer[0],1,1,inFile);
		if(rval <= 0)
			break;
		if(buffer[0] == 0xFF)
			;
		else if(buffer[0] != 0x6){
			buffer[0] &= 0x7F;
			buffer[0] += 0x1F;
		}
		rval = fread(&buffer[1],1,1,inFile);
		if(rval <= 0)
			break;
		if(buffer[1] == 0xFF)
			;
		else if(buffer[1] != 0x6){
			buffer[1] &= 0x7F;
			buffer[1] += 0x1F;
		}
		
		/* Look for start of text */
		if((buffer[1] == 0x6) && (textStart == 0x0))
			textStart = 1;
		
		if(!textStart)
			continue;
		
		/* End of Phrase, newline */
		if((buffer[0] == 0xFF) && (buffer[1] == 0xFF)){
			fprintf(outFile,"\n");
			textStart = 0;
		}
		else{
			/* Text */
			if(buffer[1] == 0x6)
				; //Do nothing, start of text block
			else if(buffer[1] != 0xFF)
				fprintf(outFile,"%c",buffer[1]);
			if(buffer[0] == 0x6)
				; //Do nothing, start of text block
			else if(buffer[0] != 0xFF)
				fprintf(outFile,"%c",buffer[0]);
		}
	}
	printf("Done\n");

    /* Close files */
    fclose(inFile);
	fclose(outFile);

    return 0;
}

/******************************************************************************/
/* main.c - Main execution file for Lunar 2 Action Script Extractor           */
/*                                                                            */
/*          Extracts action related portion of Sat or PSX scripts.            */
/******************************************************************************/

/***********************************************************************/
/* Usage                                                               */
/* ==================================================                  */
/* l2_actionExtr.exe fileName psxFlag outputFolder                     */
/*                                                                     */
/* Input File should be a packed script file                           */
/* psxFlag is 0 for saturn, or 1 for a psx file                        */
/* outputFolder gives the path for output.                             */
/***********************************************************************/


/* Includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "util.h"


#define VER_MAJ    1
#define VER_MIN    1





/* Packed File with ES Script embedded within it */
typedef struct{
    unsigned int zeroPadOffset;     /* In Saturn, offset to zero padding */
                                    /* to 2048 byte alignment at EOF     */
								    /* Need to modify if ES Section altered.  */
								    /* PSX holds something different.         */
    unsigned int numActionScripts;  /* # of Action Scripts in this file       */
	unsigned int hdr1LenBytes;      /* First Header Length in bytes */
	unsigned int esHdrOffset;       /* Offset to ES Header */
	unsigned int postActionOffset;  /* Points to the offset for the setion */
                                    /* of the file that occurs immediately */
									/* after the Action script, but before */
									/* the ES Header section.              */
	unsigned int updateOffset4;     /* Points to some section offset AFTER ES Header */
}packedHeaderType;


typedef struct{
    unsigned int actionScriptID;    /* Unique ID corresponding to this action script */
    unsigned int actionScriptOffset;/* Offset in bytes measured from end of first header */
}actionScriptHdrType;




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
    printf("l2_actionExtr.exe fileName psxFlag outputFolder\n");
    printf("\tpsxFlag is 0 for saturn, or 1 for a psx file\n\n");
    return;
}


/******************************************************************************/
/* main()                                                                     */
/******************************************************************************/
int main(int argc, char** argv){

    packedHeaderType* pPackedHdr;
	actionScriptHdrType* pActionScriptOffsetArray;
    FILE *inFile, *outFile;
	unsigned int* pBuf32, *pActionScript;//, * pEstActionScriptData;
	unsigned short* pBuf16;
	static char inFileName[300];
    static char outFileName[300];
    int x, y, z, index, outputFsize, i, numActionScripts;
    int psxFlg;
	unsigned int actionScriptOffset, scriptDataOffset, preambleSize;
	unsigned char* pPreData;
	char* outputBuf;

    printf("Lunar2 Action Script Extractor v%d.%02d\n", VER_MAJ, VER_MIN);

    /**************************/
    /* Check input parameters */
    /**************************/

    /* Check for valid # of args */
    if (argc != 4){
        printUsage();
        return -1;
    }

    //Handle Input Parameters
    memset(inFileName, 0, 300);
	memset(outFileName, 0, 300);
	strcpy(inFileName,argv[1]);
	psxFlg = atoi(argv[2]);
	strcpy(outFileName,argv[3]);
	if(outFileName[strlen(outFileName)-1] != '\\')
		strcat(outFileName,"\\");
	/* Find name of the input file */
	index = 0;
	for(x=0;x<strlen(inFileName); x++){
		if((inFileName[x] == '\\') || (inFileName[x] == '/')) 
			index = x+1;
	}
	strcat(outFileName,&inFileName[index]);
	strcat(outFileName,"_out.txt");

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

    /********************************/
    /* Parse the Input Script Files */
    /********************************/
    printf("Parsing input file: %s\n",inFileName);
	fprintf(outFile,"Input file: %s\n\n",inFileName);
	
    /***********************************************************************/
	/* Read the entire file into memory                                    */
	/***********************************************************************/
	outputBuf = (char*)malloc(1024*1024);   /* 1MB, overkill */
	if(outputBuf == NULL){
		printf("Error allocating room.\n");
		return -1;
	}
	memset(outputBuf,0,(1024*1024));
	outputFsize = fread(outputBuf,1,(1024*1024),inFile);
    if(outputFsize  <= 0x0){
		printf("Error occured while reading input file.\n");
		fclose(inFile);
		fclose(outFile);
		return -1;
	}
	fclose(inFile);

	/* Retrieve Number of Action Scripts */
    pPackedHdr = (packedHeaderType*)outputBuf;
	if(!psxFlg){
		swap32(&pPackedHdr->numActionScripts);
		swap32(&pPackedHdr->hdr1LenBytes);
	}
	numActionScripts = pPackedHdr->numActionScripts;
	fprintf(outFile,"NumSections: %d\n\n",numActionScripts);

    /* Locate the ID / Offset to each "action script" */
	/* And Print out the details                      */
	pActionScriptOffsetArray = (actionScriptHdrType*)(outputBuf+pPackedHdr->hdr1LenBytes);
	for(i = 0; i < numActionScripts; i++){
		unsigned int id, as_offset, temp;

		id = pActionScriptOffsetArray[i].actionScriptID;
		as_offset = pActionScriptOffsetArray[i].actionScriptOffset;

		if(!psxFlg){
			swap32(&id);
			swap32(&as_offset);
		}

		fprintf(outFile,"Action Script ID: 0x%X\n",id);
		fprintf(outFile,"Relative Offset: 0x%X\n",as_offset);
		fprintf(outFile,"========================\n\n");

		/* Determine the Size of and Print the Preamble Data */
		/* I have no clue what this data is, but is appears it wont need to be changed */
		fprintf(outFile,"Preamble Data\n-----------------\n");

		/* Take the offset and move to next 32-bit boundary */
		actionScriptOffset = (unsigned int)(as_offset + pPackedHdr->hdr1LenBytes);
		if(actionScriptOffset % 4 != 0){
			printf("Error, should not get here, script does not begin on 32-bit boundary; Moving to next boundary.\n");
			fflush(stdin);
			getchar();

//			/* Correct for issue and move on */
			actionScriptOffset += 4 - (actionScriptOffset % 4);
		}
		
		/* Old rudimentary method to determine start of data, use as an extra check */
//		pEstActionScriptData = (unsigned int*)(outputBuf + actionScriptOffset);
//		while(*pEstActionScriptData != 0xFFFFFFFF){
//			pEstActionScriptData++;
//		}
//		while(*pEstActionScriptData == 0xFFFFFFFF){
//			pEstActionScriptData++;
//		}


		/* Use the first 3 bytes as flags to determine the size of the data */
		/* This works for all cases except for 6 and 7 */
		/* Cases 6 and 7 were too annoying to trace so i didnt bother... */
		/* Just took some guesses based on staring at hex data */
		pActionScript = (unsigned int*)(outputBuf + actionScriptOffset);
		pPreData = (unsigned char*)pActionScript;
		preambleSize = 0x18;
		pActionScript += 6;
		while(1){
			temp = *pActionScript;
			if(!psxFlg)
				swap32(&temp); //character data, swap regardless for this

			if((temp == 0x0) || (temp == 0x30))
				break;
			pActionScript++;
			preambleSize += 4;
		}


		fprintf(outFile,"Preamble Size: 0x%X bytes\n",preambleSize);
		fprintf(outFile,"Preamble Data: ");
		for(z = 0; z < preambleSize; z++){
			fprintf(outFile,"%02X ",pPreData[z] & 0xFF);
		}
		fprintf(outFile, "\n\n");


//		printf("actionScript Data Offset=0x%X\n",actionScriptOffset);

		/* Should now be pointing at the entries */
		/* There are always 24 of them           */
		fprintf(outFile,"Action Script Data\n----------------------------\n\n");
		pBuf32 = (unsigned int*)pActionScript;
		for(x = 0; x < 24; x++){
			int cmdSync = 1;
		
			if(!psxFlg)
				swap32(pBuf32);
			fprintf(outFile,"Entry %d: 0x%08X\n",x,*pBuf32);

			if(*pBuf32 == 0x0){
				fprintf(outFile,"NO_DATA\n\n");
			}
			else{
			
				/* Determine the offset to the script data */
				scriptDataOffset = *pBuf32;
	//			if(!psxFlg)
	//				swap32(&scriptDataOffset);
				scriptDataOffset *= 2;
	//			scriptDataOffset += actionScriptOffset;
				pBuf16 = (unsigned short*)((char*)pActionScript+scriptDataOffset);
				printf("scriptDataOffset [%d][%d] = %X\n", i,x,scriptDataOffset);
			
				fprintf(outFile,"Script_DATA\n");
			
				/* Print out the Script Data */
				while(*pBuf16 != 0xFFFF){
				
					if(!psxFlg){
						swap16(pBuf16);
					}
					fprintf(outFile,"0x%04X",*pBuf16);
	#if 1
					if(cmdSync){
						unsigned short command;
						int numArgs = 0;
						command = *pBuf16;
						fprintf(outFile,"\tCmd: ");
						switch(command){
						
							case 0x0000:
							case 0x0001:
							case 0x0009:
							case 0x000A:
							case 0x0011:
								fprintf(outFile,"V-blank SW Timer Delay\n");
								numArgs = 1;
								break;
						
							case 0x0002:
								fprintf(outFile,"TBD\n");
								if(x <= 0x5)
									numArgs = 3;
								else
									numArgs = 4;
								break;
							
							case 0x0003:
								fprintf(outFile,"TBD\n");
								if((x == 0x4) || (x == 0x5))
									numArgs = 2;
								else
									numArgs = 3;
								break;
							
							case 0x0004:
								fprintf(outFile,"TBD\n");
								numArgs = 1;
								break;				

							case 0x0005:
								fprintf(outFile,"TBD - 6 args\n");
								numArgs = 6;
								break;

							case 0x0006:
								fprintf(outFile,"TBD\n");
								numArgs = 6;
								break;
							
							case 0x0007:
								fprintf(outFile,"Video Effect\n");
								numArgs = 3;
								break;	
						
							case 0x0008:
								fprintf(outFile,"TBD, assumed\n");
								numArgs = 3;
								break;

							case 0x000B: //3 args when entry = 2,6
								fprintf(outFile,"Got Item\n");
//								if((x == 0x2) || (x == 0x6))
									numArgs = 3;
//								else
//									numArgs = 1;
								break;

							case 0x000C:
								fprintf(outFile,"TBD\n");
								numArgs = 1;
								break;

							case 0x000D:
								fprintf(outFile,"TBD\n");
								numArgs = 1;
								break;

							case 0x000E:
								fprintf(outFile,"TBD - assumption\n");
								numArgs = 4;
								break;

							case 0x000F:
								fprintf(outFile,"TBD\n");
								numArgs = 1;
								break;

							case 0x0010:
								fprintf(outFile,"TBD\n");
								numArgs = 1;
								break;
						
							case 0x0012:
								fprintf(outFile,"TBD\n");
								numArgs = 1;
								break;

							case 0x0013:
								fprintf(outFile,"TBD\n");
								numArgs = 1;
								break;

							case 0x0014:
								fprintf(outFile,"TBD\n");
//								numArgs = 4; //4 when x = 6

								if(x <= 0x5)
									numArgs = 5;
								else
									numArgs = 6;

								break;


							case 0x0015:
								fprintf(outFile,"TBD\n");
								numArgs = 2;
								break;

							case 0x0016:
								fprintf(outFile,"Enter Boss Battle (arg1)\n");
								numArgs = 1;
								break;

							case 0x0017:
								fprintf(outFile,"Background Music\n");
								numArgs = 1;
								break;
							
							case 0x0018:
								fprintf(outFile,"TBD\n");
								//numArgs = 4; /* had 3, assumed 4 */
								numArgs = 2;
								break;
							
							case 0x0019:
								fprintf(outFile,"Play Sound Effect\n");
								numArgs = 1;
								break;
						
							case 0x001A:
								fprintf(outFile,"TBD\n");
								numArgs = 2;
								break;
						
							case 0x001B:
								fprintf(outFile,"TBD\n");
								numArgs = 0; //Had 2, assumed a timer always followed.
								break;
						
							case 0x001C:
								fprintf(outFile,"Start Timed Dialog\n");
								numArgs = 1;  /* technically this is 3, but the next 2 args are "1 timervalue", so i interpret as a timer cmd */
								break;
							
							case 0x001D:
								fprintf(outFile,"Advance to Next Part of Timed Dialog\n");
								//numArgs = 2;
								numArgs = 0;
								break;		
						
							case 0x001E:
								fprintf(outFile,"TBD\n");
								numArgs = 2;
								break;	
						
							case 0x001F:
								fprintf(outFile,"Play Movie\n");
								if(psxFlg)
									numArgs = 2;
								else
									numArgs = 3;
								break;	

							/* psx only */
							case 0x0020:
								fprintf(outFile,"TBD\n");
								numArgs = 1;
								break;
							case 0x0021:
								fprintf(outFile,"TBD\n");
								numArgs = 3;
								break;
							case 0x0022:
								fprintf(outFile,"TBD\n");
								numArgs = 1;
								break;
							case 0x0023:
								fprintf(outFile,"Saturn Unimplemented\n");
								numArgs = 0;
								break;


							default: 
								fprintf(outFile,"Unknown, SYNC LOST\n");
								cmdSync = 0;
								numArgs = 0;
								break;
						}
					
						if(numArgs >= 0){
							fprintf(outFile,"\tArgs: %d ",numArgs);
							for(y = 0; y < numArgs; y++){
								pBuf16++;
								if(!psxFlg){
									swap16(pBuf16);
								}
								fprintf(outFile,"0x%04X ",*pBuf16);
							}
							fprintf(outFile,"\n");
						}
					}
					else{
						fprintf(outFile,"\n");
					}
	#endif				
					pBuf16++;
				}
				fprintf(outFile,"0x%04X\n",*pBuf16);
				fprintf(outFile,"End_Of_DATA\n\n");
				pBuf16++;
			}
		
			pBuf32++;
		}	

		/* Add some lines for the next section */
		fprintf(outFile,"\n\n");
	}

    /* Done with Buffer */
    free(outputBuf);
	
    /* Close files */
	fclose(outFile);

    return 0;
}

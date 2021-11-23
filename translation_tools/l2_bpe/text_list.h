/**********************************************************************/
/* text_list.h - Linked list routines for text compression.           */
/**********************************************************************/
#ifndef TEXT_LIST_H
#define TEXT_LIST_H

#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif



/* Defines */
typedef struct textNode textNode;

/* Script Node Datatype */
struct textNode
{
	unsigned char* origText;	//Stored Text
	unsigned int origNumBytes;
	unsigned char* modifiedText;	//8-Bit Coded Text, no NULL terminator
	unsigned int modNumBytes;

	int associatedFile;

	//Linked List Pointers
	textNode* pNext;
	textNode* pPrev;
};

/***********************/
/* Function Prototypes */
/***********************/
int initTextList();
int destroyTextList();
textNode* getHeadPtr();
int createTextNode(textNode** node);
int addTextNode(textNode* node);



#endif

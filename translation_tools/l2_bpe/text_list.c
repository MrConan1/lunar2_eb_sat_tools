/**********************************************************************/
/* text_list.c - Linked list routines for text compression.           */
/**********************************************************************/


/************/
/* Includes */
/************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "text_list.h"


/***********************/
/* Function Prototypes */
/***********************/
int initTextList();
int destroyTextList();
textNode* getHeadPtr();
int createTextNode(textNode** node);
int addTextNode(textNode* node);


/* Globals */
static textNode *pHead = NULL;    /* List Ptr  */


/*******************************************************************/
/* initTextList                                                    */
/* Creates an empty list to hold the script node information.      */
/*******************************************************************/
int initTextList()
{
    if(pHead != NULL)
        destroyTextList();
    pHead = NULL;

    return 0;
}




/*******************************************************************/
/* getHeadPtr - Returns the list's head pointer.                   */
/*******************************************************************/
textNode* getHeadPtr(){
    return pHead;
}




/*******************************************************************/
/* destroyTextList                                                 */
/* Destroys all items in the list.                                 */
/*******************************************************************/
int destroyTextList()
{
    textNode *pCurrent = NULL;
    textNode *pItem = pHead;


    while (pItem != NULL){
        pCurrent = pItem;

        /* Free internal data */
		if (pCurrent->origText != NULL)
			free(pCurrent->origText);
		if (pCurrent->modifiedText != NULL)
			free(pCurrent->modifiedText);

        pItem = pItem->pNext;
        free(pCurrent);
    }

    pHead = NULL;

    return 0;
}


/*******************************************************************/
/* createTextNode                                                  */
/* Creates and initializes a new text node.                        */
/* Returns 0 on success, -1 on failure.                            */
/*******************************************************************/
int createTextNode(textNode** node){

    textNode* pNode;
    *node = (textNode*)malloc(sizeof(textNode));
    pNode = *node;
    if(pNode == NULL){
        printf("Error creating new script node\n");
        return -1;
    }
    memset(pNode,0,sizeof(textNode));
    pNode->pNext = pNode->pPrev = NULL;
    pNode->origText = NULL;
    pNode->modifiedText = NULL;
	pNode->origNumBytes = 0;
	pNode->modNumBytes = 0;

    return 0;
}


/*******************************************************************/
/* addTextNode                                                     */
/* Inserts an element in the list.                                 */
/* Returns 0 on success, -1 on failure.                            */
/*******************************************************************/
int addTextNode(textNode* node){

    textNode * newItem, *pCurrent, *pNext;

    /* Create a new item */
    newItem = (textNode*)malloc(sizeof(textNode));
    if(newItem == NULL){
        printf("Error allocing memory for new item in addNode.\n");
        return -1;
    }
    memcpy(newItem, node, sizeof(textNode));
    newItem->pNext = NULL;

    /* Head is Empty */
    if(pHead == NULL){
        pHead = newItem;
        return 0;
    }

    /* First item is not empty and insertion will not take place at head */
    pCurrent = pHead;
    while(pCurrent != NULL){
        pNext = pCurrent->pNext;

        /* Insert at end of list */
        if(pNext == NULL){
            pCurrent->pNext = newItem;
            return 0;
        }

        pCurrent = pCurrent->pNext;
    }

    printf("Error, insertion failed.\n");
    return -1;
}

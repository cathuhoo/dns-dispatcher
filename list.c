/*****************************************************************************
*                                                                            *
*  -------------------------------- list.c --------------------------------  *
*                                                                            *
*****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "list.h"

void list_init(List *list, void (*destroy)(void *data), void (*display)(void *data)) 
{

    list->size = 0;
    list->destroy = destroy;
    list->display = display;
    list->head = NULL;
    list->tail = NULL;
    return;
}

void list_travel( List * list)
{
    ListElmt * ptr;
    ptr=list->head;
    while (ptr)
    {
        list->display (ptr->data);
        ptr=ptr->next;
    }
}

void list_destroy(List *list) 
{
    void               *data;
    while (list_size(list) > 0) 
    {

       if (list_rem_next(list, NULL, (void **)&data) == 0 && list->destroy != NULL) 
       {
          //  Call a user-defined function to free dynamically allocated data.    
          list->destroy(data);

       }

    }

    //*  No operations are allowed now, but clear the structure as a precaution.   *

    memset(list, 0, sizeof(List));

    return;

}

int list_ins_next(List *list, ListElmt *element, const void *data) 
{

    ListElmt           *new_element;

    //*  Allocate storage for the element.                                         

    if ((new_element = (ListElmt *)malloc(sizeof(ListElmt))) == NULL)
       return -1;

    //*  Insert the element into the list.                                         

    new_element->data = (void *)data;

    if (element == NULL) 
    {

       //*  Handle insertion at the head of the list.                              
       if (list_size(list) == 0)
          list->tail = new_element;

       new_element->next = list->head;
       list->head = new_element;

    }
    else 
    {

       //*  Handle insertion somewhere other than at the head.                     *

       if (element->next == NULL)
          list->tail = new_element;

       new_element->next = element->next;
       element->next = new_element;

    }

    //  Adjust the size of the list to account for the inserted element.          *

    list->size++;

    return 0;

}

int list_rem_next(List *list, ListElmt *element, void **data) {

    ListElmt           *old_element;

    //  Do not allow removal from an empty list.                                  

    if (list_size(list) == 0)
       return -1;

    //*  Remove the element from the list.                                       
    if (element == NULL) 
    {
       //  Handle removal from the head of the list.                              

       *data = list->head->data;
       old_element = list->head;
       list->head = list->head->next;

       if (list_size(list) == 1)
          list->tail = NULL;

     }
    else 
    {
       //  Handle removal from somewhere other than the head.                     

       if (element->next == NULL)
          return -1;

       *data = element->next->data;
       old_element = element->next;
       element->next = element->next->next;

       if (element->next == NULL)
          list->tail = element;

    }

    //  Free the storage allocated by the abstract data type.                     
    free(old_element);
    //  Adjust the size of the list to account for the removed element.           

    list->size--;

    return 0;
}

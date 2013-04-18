/*****************************************************************************
*                                                                            *
*  Define a structure for linked list elements.                              *
*                                                                            *
*****************************************************************************/

typedef struct ListElmt_ {

    //void               *data;
    Resolver               *data;
    struct ListElmt_   *next;

} ListElmt;

/*
 *  Copyright (C) 2014 ThingWorx Inc.
 *
 *  Doubly linked list utilities
 */

#ifndef TW_LIST_H
#define TW_LIST_H

#include "twOSPort.h"

#ifdef __cplusplus
extern "C" {
#endif

/******************************/
/*       Dynamic Lists        */
/* Thread safe implementation */
/*   of a dynamically sized   */
/*   list of untyped entries  */
/******************************/
struct ListEntry;

/* 
Signature of a function called
to delete the values in a ListEntry
*/
typedef void (*del_func) (void * item);

/**************************/
/*   Generic List Entry   */
/**************************/
typedef struct ListEntry {
    struct ListEntry *next;
    struct ListEntry *prev;
    void *value;
} ListEntry;

/**************************/
/*    List Structure      */
/**************************/
typedef struct twList {
	int count;
	struct ListEntry *first;
	struct ListEntry *last;
	TW_MUTEX mtx;
	del_func delete_function;
} twList;

/*
twList_Create - Create a list.  The list becomes owner of all ListEntries and
value pointers in the list entry.
Parameters:
    delete_function - pointer to function to call when deleting a ListEntry value.  If
		this is NULL, then the value is simple deleted using 'free'
Return:
	twList * - pointer to the allocated twList or a NULL if an error occurred.
*/
twList * twList_Create(del_func delete_function);

/*
twList_Delete - Deletes a list.  All entries and associated values are deleted as well.
Parameters:
    list - pointer to function to the list to delete
Return:
	int - zero if successfull, non-zero if an error occurred
*/
int twList_Delete(struct twList *list);

/*
twList_Clear - Clears a list.  All entries and associated values are deleted.
Parameters:
    list - pointer to function to the list to clear
Return:
	int - zero if successfull, non-zero if an error occurred
*/
int twList_Clear(struct twList *list);

/*
twList_Add - Creates a ListEntry and adds it to a list.
Parameters:
    list - pointer to the list to be added to
	value -the value to assigned to the ListEntry
Return:
	int - zero if successful, non-zero if an error occurred
*/
int twList_Add(twList *list, void *value);

/*
twList_Remove - Removes an entry from a list.  The ListEntry structure is deleted, the
	value contained in the ListEntry is optionally deleted.
Parameters:
    list - pointer to the list to operate on
	entry - pointer to the entry to remove
	deleteValue - boolean, if TRUE deletes the value using the function supplied when the 
		list was created, if FALSE the value is not deleted
Return:
	int - zero if successful, non-zero if an error occurred
*/
int twList_Remove(struct twList *list, struct ListEntry * entry, char deleteValue);

/*
twList_Next - Used to iterate through a list. Returns ListEntry that is the next entry 
	after the supplied ListEntry.  If entry is 0 the the first ListEntry is returned.
Parameters:
    list - pointer to the list to operate on
	entry - pointer to the current entry.  May be NULL in which case the first ListEntry is returned. 
Return:
	ListEntry * - the next entry in the list or a NULL if we are at the end of the list or an error
		occurred.  The list still owns this pointer so do NOT delete it.
*/
ListEntry * twList_Next(struct twList *list, struct ListEntry * entry);

/*
twList_GetByIndex - Gets the Nth entry from the list.
Parameters:
    list - pointer to the list to operate on
	index - the zero based index in the list to retrieve 
Return:
	ListEntry * - the entry at the specified index or zero if the index was invalid.
		 The list still owns this pointer so do NOT delete it.
*/
ListEntry * twList_GetByIndex(struct twList *list, int index);

/*
twList_GetCount - returns the number of entries in the list.
Parameters:
    list - pointer to the list to operate on
Return:
	int - the number of entries in the list
*/
int twList_GetCount(struct twList *list);

#ifdef __cplusplus
}
#endif

#endif

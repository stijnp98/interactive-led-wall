#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "dplist.h"
#include <limits.h>

/*
 * definition of error codes
 * */
#define DPLIST_NO_ERROR 0
#define DPLIST_MEMORY_ERROR 1 // error due to mem alloc failure
#define DPLIST_INVALID_ERROR 2 //error due to a list operation applied on a NULL list 

#ifdef DEBUG
	#define DEBUG_PRINTF(...) 									         \
		do {											         \
			fprintf(stderr,"\nIn %s - function %s at line %d: ", __FILE__, __func__, __LINE__);	 \
			fprintf(stderr,__VA_ARGS__);								 \
			fflush(stderr);                                                                          \
                } while(0)
#else
	#define DEBUG_PRINTF(...) (void)0
#endif


#define DPLIST_ERR_HANDLER(condition,err_code)\
	do {						            \
            if ((condition)) DEBUG_PRINTF(#condition " failed\n");    \
            assert(!(condition));                                    \
        } while(0)

        
/*
 * The real definition of struct list / struct node
 */

struct dplist_node {
  dplist_node_t * prev, * next;
  void * element;
};

struct dplist {
  dplist_node_t * head;
  void * (*element_copy)(void * src_element);			  
  void (*element_free)(void ** element);
  int (*element_compare)(void * x, void * y);
};


dplist_t * dpl_create (// callback functions
			  void * (*element_copy)(void * src_element),
			  void (*element_free)(void ** element),
			  int (*element_compare)(void * x, void * y)
			  )
{
  dplist_t * list;
  list = malloc(sizeof(struct dplist));
  DPLIST_ERR_HANDLER(list==NULL,DPLIST_MEMORY_ERROR);
  list->head = NULL;  
  list->element_copy = element_copy;
  list->element_free = element_free;
  list->element_compare = element_compare; 
  return list;
}

void dpl_free(dplist_t ** list, bool free_element)
{
  dplist_node_t* list_node;
  if(list != NULL)
  {
    int size_of_list = dpl_size(*list);
    if(size_of_list > 0)
    {
    //request the size of the list
    list_node = dpl_get_reference_at_index(*list,size_of_list);
    size_of_list--;
    //remove all the nodes of an list
    if(size_of_list > 1){
    for(int i = 0;i < size_of_list;i++){
      //if copies where made remove first the copies to avoid memory leaks.
      if(free_element){
        (*list)->element_free(list_node->element);
      }
      list_node = list_node->prev;
      free(list_node->next);
     }
    }
    //if copies where made remove first the copies to avoid memory leaks.
    if(free_element){
        (*list)->element_free(list_node->element);
    }
     //free the last node as well
     free(list_node);
    } 
    free(*list);
    *list = NULL;
  }
}

dplist_t * dpl_insert_at_index(dplist_t * list, void * element, int index, bool insert_copy)
{
    //make pointers for the new created list node
  //and the value of the previous list item.
  dplist_node_t * index_list_node, * new_list_node;
  //check if the given list exists
  if(list == NULL)return NULL;
  if(element == NULL)return list;
  //make a new list element and store the pointer in new_list_node
  new_list_node = malloc(sizeof(dplist_node_t));
  //check if the allocation was succesfull
  if(new_list_node == NULL)return NULL;
  
  if(insert_copy)
  {
    element = list->element_copy(element);
  }

  //insert the given value in the new list element
  new_list_node->element = element;
  // check if this is not the first element of the list
  if (list->head == NULL)  
  { // covers case 1 
    //there is no previous element
    new_list_node->prev = NULL;
    //there is no next element
    new_list_node->next = NULL;
    //store the pointer of the new list item in the head
    list->head = new_list_node;
  }
  //check if the index is smaller of equal to zero.
  //insert than the item at index 0 and move the index of zero to 1.
   else if (index <= 0)  
  { 
    //make the previous element NULL
    new_list_node->prev = NULL;
    //the next element is equal to the the first element now
    new_list_node->next = list->head;
    //the previous of the originalis equal to the new list node pointer
    list->head->prev = new_list_node;
    //change the head pointer location to the new first node 
    list->head = new_list_node;
  } 
  else 
  {
    //request the pointer locatation of the element on that place 
    index_list_node = dpl_get_reference_at_index(list, index); 
    //check the returned pointer was valid 
    if( index_list_node == NULL)
    {
      printf("unexpected behaviour\r\n");
      return list;
    } 
    // if the index is within the list insert a element in the list
    if (index < dpl_size(list))
    { // covers case 4
      //copy the previous pointer adres to the new element
      new_list_node->prev = index_list_node->prev;
      new_list_node->next = index_list_node;
      index_list_node->prev->next = new_list_node;
      index_list_node->prev = new_list_node;
      // pointer drawing breakpoint
    } 
    else
    { // if the index is out of range make a new list element at the end of the list.
      if( index_list_node == NULL) 
      {
        printf("unexpected behaviour\r\n");
        return list;
      }
      new_list_node->next = NULL;
      new_list_node->prev = index_list_node;
      index_list_node->next = new_list_node;    
      // pointer drawing breakpoint
    }
  }
  return list;
}

dplist_t * dpl_remove_at_index( dplist_t * list, int index, bool free_element)
{
    //make pointers for the new created list node
  //and the value of the previous list item.
  dplist_node_t * prev_list_node, * next_list_node, * list_node;
  //check if the given list exists
  if(list == NULL)return NULL;

  // check if this is not the first element of the list
  if (list->head == NULL)  
  { // no elements in the list so nothing to remove
    return list;
  }
  //check if the index is smaller or equal to zero.
  //remove item at zero and replace header
   else if (index <= 0)  
  { 
    list_node = list->head;

    //if a copy was made remove it before removing the list node.
    if(free_element)
    {
      list->element_free(list_node->element);
    }

    if(list_node->next != NULL)
    {
      //make the head equal to the new first element.
      list->head = list_node->next;
      list_node->next->prev = NULL;
      free(list_node);
    }
    else
    {
      list->head = NULL;
      free(list_node);
    }
  }
  //if not the first elemennt and the list is not empty than 
  else 
    {
    //request the pointer locatation of the element on that place 
    list_node = dpl_get_reference_at_index(list, index); 

    //if a copy was made remove it before removing the list node.
    if(free_element)
    {
      list->element_free(list_node->element);
    }

    next_list_node = list_node->next;
    prev_list_node = list_node->prev;
    
    //check if the last element is the first of the list
    if(prev_list_node == NULL && next_list_node == NULL)
    {
      list->head = NULL;
      free(list_node);
    }
    else if(list_node->next == NULL)
    {
      prev_list_node->next = NULL;
      free(list_node);
    }
    else
    {
      prev_list_node->next = next_list_node;
      next_list_node->prev = prev_list_node;
      free(list_node);
    }

  }
  return list;
}

int dpl_size( dplist_t * list )
{
  int count;
  dplist_node_t * dummy;
  if (list == NULL || list->head == NULL ) return 0;
  //increase the counte as long as next is not NULL
  //start of with 1 because there is now at least 1 element in the list 
  for ( dummy = list->head, count = 1; dummy->next != NULL ; dummy = dummy->next, count++); 
  //return the number of elements
  return count; 
}

void * dpl_get_element_at_index( dplist_t * list, int index )
{
  dplist_node_t * dummy;
  if(index >= dpl_size(list)) index = dpl_size(list) - 1;
  if(index <= 0) index = 0;
  dummy = dpl_get_reference_at_index(list,index);
  if (list == NULL || list->head == NULL || dummy == NULL) return NULL;
  return dummy->element; 
}

int dpl_get_index_of_element( dplist_t * list, void * element )
{
  int count;
  dplist_node_t * dummy;
  if (list == NULL || list->head == NULL ) return -1;
  for ( dummy = list->head, count = 0; dummy->next != NULL ; dummy = dummy->next, count++) 
  { 
    if (0 == list->element_compare(dummy->element,element)) return count;
  }  
  if (0 == list->element_compare(dummy->element,element)) return count;
  return -1; 
}

dplist_node_t * dpl_get_reference_at_index( dplist_t * list, int index )
{
  int count;
  dplist_node_t * dummy;
  if (list == NULL || list->head == NULL ) return NULL;
    for ( dummy = list->head, count = 0; dummy->next != NULL ; dummy = dummy->next, count++) 
    { 
      if (count >= index) return dummy;
    }  
  return dummy;
}
 
void * dpl_get_element_at_reference( dplist_t * list, dplist_node_t * reference )
{
    //check for valid situations
    if(list == NULL || list->head == NULL) return NULL;
    dplist_node_t * dummy;
    int size = dpl_size(list);
    int count = 0;
    bool found = false;
    if(reference == NULL)
    {
      dummy =  dpl_get_last_reference(list)->element;
      return dummy;
    }
    for(dummy = list->head,count = 0; count < size && !found; dummy = dummy->next, count++)
    {
      if(reference == dummy) found = true;
    }
    if(!found)
    {
      return NULL;
    }
    //if valid return the element
    return reference->element;
}

dplist_node_t * dpl_get_first_reference( dplist_t * list)
{
  if(list == NULL)return NULL;
  return list->head;
}

dplist_node_t * dpl_get_last_reference( dplist_t * list)
{
  dplist_node_t * dummy;
  if(list == NULL)return NULL;
  dummy = list->head;
    while (dummy->next != NULL)
    {
      dummy = dummy->next;
    }
  return dummy;
}

dplist_node_t * dpl_get_reference_of_element( dplist_t * list, void * element)
{
  int count;
  dplist_node_t * dummy;
  if (list == NULL || list->head == NULL || element == NULL) return NULL;
  for ( dummy = list->head, count = 0; dummy->next != NULL ; dummy = dummy->next, count++) 
  { 
    if (0 == list->element_compare(dummy->element,element)) return dummy;
  }  
  if (0 == list->element_compare(dummy->element,element)) return dummy;
  return NULL; 
}

dplist_node_t * dpl_get_next_reference( dplist_t * list, dplist_node_t * reference)
{
  if(list == NULL || list->head == NULL) return NULL;
  dplist_node_t * dummy;
  int count = 0;
  int size = dpl_size(list);
  bool found = false;
  for(dummy = list->head, count = 0; count < size && !found; dummy = dummy->next, count++)
    {
      if(reference == dummy) found = true;
    }
    if(!found)
    {
      return NULL;
    }
    //this is the index of the end element so just return that element.
    if(count >= size)
    {
      return reference;
    }

  return dpl_get_reference_at_index(list,count);
}

dplist_node_t * dpl_get_previous_reference( dplist_t * list, dplist_node_t * reference)
{
  if(list == NULL || list->head == NULL) return NULL;
  dplist_node_t * dummy;
  int size = dpl_size(list);
  int count = 0;
  bool found = false;
  if(reference == NULL){
    dummy = dpl_get_last_reference(list);
    return dummy;
  } 
  for(dummy = list->head, count = 0; count < size && !found; dummy = dummy->next, count++)
    {
      if(reference == dummy) found = true;
    }
    if(!found)
    {
      return NULL;
    }
    //this is the index of the begin element so just return that element.
    if(reference == list->head)
    {
     return reference;
    }
  return dpl_get_reference_at_index(list,count - 2);
}

//insert at reference
dplist_t * dpl_insert_at_reference(dplist_t * list, void * element, dplist_node_t* reference, bool insert_copy)
{
  if(list != NULL && list->head != NULL && element != NULL)
  {
    dplist_node_t * dummy;
    int count = 0;
  int size = dpl_size(list);
  bool found = false;
  if(reference == NULL) count = size + 1;
  else
  {
  for(dummy = list->head, count = 0; count < size && !found; dummy = dummy->next, count++)
    {
      if(reference == dummy) found = true;
    }
    count--;
    if(!found)
    {
      return list;
    }
  }
  dpl_insert_at_index(list, element, count, insert_copy);
  }
  return list;
}

//insert at reference
dplist_t * dpl_insert_sorted( dplist_t * list, void * element, bool insert_copy )
{
  int index = -1;
  int count = 0;
  if(list != NULL && list->head != NULL && element != NULL)
  {
    dplist_node_t * dummy;
    for ( dummy = list->head, count = 0; dummy->next != NULL && index == -1; dummy = dummy->next, count++) 
    { 
     if (1 == list->element_compare(dummy->element,element)) 
      {
        index = count;
      }
    } 
    //check last element as well
    if (1 == list->element_compare(dummy->element,element)) 
      {
        index = count;
      }
    if(index > -1) dpl_insert_at_index(list, element, index, insert_copy);
  }
  return list;
}

//remove at reference
dplist_t * dpl_remove_at_reference( dplist_t * list, dplist_node_t * reference, bool free_element )
{
  if(list != NULL || list->head == NULL)
  {
    int count = 0;
    int size = dpl_size(list);
    dplist_node_t * dummy;
    bool found = false;
    if(reference == NULL) count = size + 1;
    else
    {
    for(dummy = list->head, count = 0; count < size && !found; dummy = dummy->next, count++)
    {
      if(reference == dummy) found = true;
    }
    count--;
    if(!found)
    {
      return list;
    }
    }
    dpl_remove_at_index(list, count, free_element);
  }
  return list;
}


dplist_t * dpl_remove_element( dplist_t * list, void * element, bool free_element )
{
  if(list != NULL && list->head != NULL && element != NULL)
  {
    int index = dpl_get_index_of_element(list,element);
    if(index > -1)
    {
      dpl_remove_at_index(list,index,free_element);
    }
  }
 return list;
}
/*
 * list.h
 *
 *  Created on: Jul 25, 2013
 *      Author: guru
 */

#ifndef LIST_H_
#define LIST_H_

struct node {
	void *data;
	struct node *next;
};

struct list {
	struct node *head;
	struct node *last_ptr;
};

struct node *__node;

#define init_list(list_name) \
	list_name = malloc(sizeof(struct list));

#define append(list,element) \
{ \
	typeof(element) *ptr = malloc(sizeof(element)); \
	*ptr = element; \
	\
	struct node *new_node = malloc(sizeof(struct node)); \
	new_node->next = NULL; \
	new_node->data = ptr; \
	\
	struct node *node = list->head; \
	if(node != NULL) { \
		list->last_ptr->next = new_node; \
		list->last_ptr = list->last_ptr->next; \
	} \
	else { \
		/* HEAD is NULL */ \
		list->head = new_node; \
		list->last_ptr = list->head; \
	} \
}

#define for_each_entry(entry, list) \
	entry = list->head->data; \
	for(__node = list->head; \
	__node != NULL; \
	__node = __node->next, entry = __node->data \
	)


#endif /* LIST_H_ */

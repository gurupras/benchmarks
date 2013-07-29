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
	int size;
};

struct node *__node;
int __list_index;

#define init_list(list_name) \
	list_name = malloc(sizeof(struct list)); \
	list_name->size = 0;

#define free_list(list_name) \
{ \
	struct node *node = list->head; \
	while(node != list->last_ptr) { \
		struct node *tmp = node; \
		node = node->next; \
		free(tmp->data); \
		free(tmp); \
	} \
	free(list->last_ptr); \
	free(list); \
}



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
	list->size++; \
}

#define for_each_entry(entry, list) \
	entry = list->head->data; \
	for(__node = list->head, __list_index = 0; \
	__list_index < list->size; \
	__list_index++, __node = __list_index < list->size ? __node->next : __node, entry = __node->data  \
	)


#endif /* LIST_H_ */

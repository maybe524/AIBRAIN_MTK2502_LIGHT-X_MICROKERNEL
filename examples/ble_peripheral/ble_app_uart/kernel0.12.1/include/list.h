#pragma once

#include "include/types.h"

#define HEAD_INIT(list) \
	{.next = &list, .prev = &list}

#define LIST_HEAD(list) \
	struct list_head list = HEAD_INIT(list)

struct list_head {
	struct list_head *next, *prev;
};

#define list_for_each(iter, head) \
	for (iter = (head)->next; iter != (head); iter = iter->next)

#define list_for_each_reverse(iter, head) \
	for (iter = (head)->prev; iter != (head); iter = iter->prev)

#define list_for_each_safe(pos, n, head) \
			for (pos = (head)->next, n = pos->next; pos != (head); \
				pos = n, n = pos->next)

#define list_for_each_entry(pos, head, type, member) \
	for (pos = container_of((head)->next, type, member); \
		&pos->member != (head); \
		pos = container_of(pos->member.next, type, member))

#define list_entry container_of

static inline void INIT_LIST_HEAD(struct list_head *head)
{
	head->next = head;
	head->prev = head;
}

static inline void __list_add(struct list_head *new_node,
					struct list_head *prev, struct list_head *next)
{
	next->prev = new_node;
	new_node->next  = next;
	new_node->prev  = prev;
	prev->next = new_node;
}

static inline void list_add(struct list_head *new_node, struct list_head *head)
{
	__list_add(new_node, head, head->next);
}

static inline void list_add_tail(struct list_head *new_node, struct list_head *head)
{
	__list_add(new_node, head->prev, head);
}

static inline void __list_del(struct list_head *prev, struct list_head *next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void list_del(struct list_head *node)
{
	__list_del(node->prev, node->next);
}

static inline void list_del_init(struct list_head *node)
{
	__list_del(node->prev, node->next);
	INIT_LIST_HEAD(node);
}

static inline int list_empty(const struct list_head *head)
{
	return head->next == head;
}

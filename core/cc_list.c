/* Licensed under BSD-MIT - see LICENSE file for details */
#include <stdio.h>
#include <stdlib.h>
#include "cc_list.h"

static void *corrupt(const char *abortstr,
		     const struct cc_list_node *head,
		     const struct cc_list_node *node,
		     unsigned int count)
{
	if (abortstr) {
		fprintf(stderr,
			"%s: prev corrupt in node %p (%u) of %p\n",
			abortstr, node, count, head);
		abort();
	}
	return NULL;
}

struct cc_list_node *cc_list_check_node(const struct cc_list_node *node,
				  const char *abortstr)
{
	const struct cc_list_node *p, *n;
	int count = 0;

	for (p = node, n = node->next; n != node; p = n, n = n->next) {
		count++;
		if (n->prev != p)
			return corrupt(abortstr, node, n, count);
	}
	/* Check prev on head node. */
	if (node->prev != p)
		return corrupt(abortstr, node, node, 0);

	return (struct cc_list_node *)node;
}

struct cc_list_head *list_check(const struct cc_list_head *h, const char *abortstr)
{
	if (!cc_list_check_node(&h->n, abortstr))
		return NULL;
	return (struct cc_list_head *)h;
}

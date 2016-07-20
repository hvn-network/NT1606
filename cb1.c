#include <stdio.h>

typedef struct node_
{
	int data;
	struct node_ *next;
} node;

node *createnode(node* head, int data)
{
	//add node to last position 
	node* p = NULL;
	node* t = NULL;
	p = (node*) malloc(sizeof(node));
	
	p->data = data;
	p->next = NULL;
		
	if (head == NULL)
	return p;
	else
	{
		for (t=head; t->next != NULL; t=t->next)
		{};
	    t->next = p;
	}
	
	return head;
}

void showlist(node* head)
{
	node *p = head;
	while (p!= NULL)
	{
		printf("%d ",p->data);
		p = p->next;
	}
}

void freelist(node* head)
{
	node* p = head;
	while (p!= NULL)
	{
		p = p->next;
		free(p);
	}
	printf("HEAD:%d",head->data);
	free(head);
	//printf("HEAD:%d",head->data);
}
#if 0
void apply(node* phead, void (*fp)(void*, void*), void* arg)
{
	node* p = phead;
	while (p!= NULL)
	{
		fp(p,arg);
		p = p->next;
	}
}
#endif

void apply(node* phead, void (*fp)(node*, void*), void* arg)
{
	node* p = phead;
	while (p!= NULL)
	{
		fp(p,arg);
		p = p->next;
	}
}
#if 0
void print(void* p, void* arg)
{
	node* np = (node*) p;
	printf("kaka %d\n", np->data);
}
#endif
void print(node* p, void* arg)
{
	//node* np = (node*) p;
	printf("ka %d\n", p->data);
}

void dototal(void*p, void* arg)
{
	node* np=(node*) p;
	int *ptotal = (int*)arg;
	*ptotal += np->data;
}

int main()
{
	node* head = NULL;
	int total =0;
	head = createnode(head,10);
	head = createnode(head,20);
	head = createnode(head,0);
	//showlist(head);
	apply(head,print,NULL);
	apply(head, dototal,&total);
	printf("total=%d\n",total);
	//freelist(head);
}

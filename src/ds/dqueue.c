
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "dqueue.h"


typedef struct
{
    container	*C;
    value	*elem;
    int		size, head, tail, num_elems;
} queue_container_priv;


inline alloc_data queue_ap_allocate( container *C, va_list ap )
{
    container	*N = C->clone(C);
    alloc_data	x;

    x.v.C = N;
    va_copy(x.ap, ap);

    return x;
}


inline void queue_deallocate( container *C, value a )
{
    destroy(a.C);
}


void queue_destroy( container *C )
{
    queue_container_priv	*Q = C->priv;
//    int				i;

    while (Q->num_elems>0)
	queue_pop(C);
/*
    for (i=0; i<Q->size; i++)
	deallocate( Q->C, Q->elem[i] );
*/
    free(Q->elem);
    destroy(Q->C);
    free(Q);
    free(C);
}


inline void queue_clear( container *C )
{
    queue_container_priv	*Q = C->priv;

    while (Q->num_elems>0)
	queue_pop(C);

    free(Q->elem);

    Q->head = Q->tail = Q->num_elems = 0;
    Q->size = 64;
    Q->elem = malloc(sizeof(value) * Q->size);
}


void queue_push( container *C, ... )
{
    va_list			ap;
    alloc_data			ad;
    queue_container_priv	*Q = C->priv;

    if (Q->num_elems >= Q->size)
	{
	    value	*old_elem = Q->elem;
	    int		old_size = Q->size;

	    //assert( Q->tail == Q->head );
//	    printf(" (oldsize:%i, newsize:%i) ", Q->size, Q->size*2); fflush(stdout);

	    Q->size*= 2;
	    Q->elem = malloc(sizeof(value) * Q->size);
	    memcpy( Q->elem, old_elem, sizeof(value) * Q->tail );
	    memcpy( Q->elem + Q->tail + old_size, old_elem + Q->head, sizeof(value) * (old_size - Q->head) );
	    Q->head = Q->tail + old_size;
	    free(old_elem);
	}

//    printf(" (push;num_elems:%i) ", Q->num_elems); fflush(stdout);

    va_start(ap, C);

    ad = Q->C->ap_allocate( Q->C, ap );
    Q->elem[Q->tail] = ad.v;
    Q->tail++;
    if (Q->tail >= Q->size) Q->tail = 0;
    Q->num_elems++;

    va_end(ad.ap);
}


void queue_pop( container *C )
{
    queue_container_priv	*Q = C->priv;
//    value			v;

    if (Q->num_elems==0)
	{
	    fprintf(stderr, "queue_pop: Error! Attempting to pop value from empty queue!\n");
//	    return (value)NULL;
	    return;
	}

    deallocate(Q->C, Q->elem[Q->head]);

    Q->head++;
    if (Q->head >= Q->size) Q->head = 0;

    Q->num_elems--;

//    printf(" (pop;num_elems:%i) ", Q->num_elems); fflush(stdout);
//    return v;
}


value queue_peak( container *C )
{
    queue_container_priv	*Q = C->priv;

    if (Q->num_elems==0)
	{
	    fprintf(stderr, "queue_peak: Error! Attempting to peak value from empty queue!\n");

	    return (value)NULL;
	}

    return Q->elem[Q->head];
}


value queue_peak_front( container *C, int index_from_first )
{
    queue_container_priv	*Q = C->priv;
    int				index = Q->head + index_from_first;

    if (index >= Q->size) index-= Q->size;

    if (Q->num_elems<index_from_first+1)
	{
	    fprintf(stderr, "queue_peak: Error! Attempting to peak value outside of boundaries!\n");

	    return (value)NULL;
	}

    return Q->elem[index];
}


value queue_peak_end( container *C, int index_from_last )
{
    queue_container_priv	*Q = C->priv;
    int				index = Q->tail -1 - index_from_last;

    if (index < 0) index+= Q->size;

    if (Q->num_elems<index_from_last+1)
	{
	    fprintf(stderr, "queue_peak: Error! Attempting to peak value outside of boundaries!\n");

	    return (value)NULL;
	}

    return Q->elem[index];
}


int queue_size( container *C )
{
    return ((queue_container_priv*)C->priv)->num_elems;
}


inline container* queue_clone( container *C )
{
    queue_container_priv	*LP = C->priv;
    container			*N = LP->C->clone(LP->C);
    return queue_container(N);
}

inline value queue_copy( container *C, value a )
{
    fprintf(stderr, "Function not implemented yet: queue_copy\n");
    exit(-1);
}


inline void queue_print( container *C, value a )
{
    int		i, p;
    queue_container_priv	*Q = a.C->priv;

    p = 0;

    printf("(");
    if (Q->head >= Q->tail)
	{
	    for (i=Q->head; i<Q->tail; i++)
		{
		    if (p==0) p = 1;
		    else printf(" ");

		    printv(Q->C, Q->elem[i]);
		}
	}
    else
	{
	    for (i=Q->head; i<Q->size; i++)
		{
		    if (p==0) p = 1;
		    else printf(" ");

		    printv(Q->C, Q->elem[i]);
		}
	    for (i=0; i<Q->tail; i++)
		{
		    if (p==0) p = 1;
		    else printf(" ");

		    printv(Q->C, Q->elem[i]);
		}
	}
    printf(")");
}


container* queue_container( container *C )
{
    container			*QX = malloc(sizeof(container));
    queue_container_priv	*QP = malloc(sizeof(queue_container_priv));

    QX->compare = NULL;
    QX->ap_allocate = queue_ap_allocate;
    QX->deallocate = queue_deallocate;
    QX->destroy = queue_destroy;
    QX->clear = queue_clear;
    QX->clone = queue_clone;
    QX->copy = queue_copy;
    QX->print = queue_print;
    QX->priv = QP;

    QP->C = C;
    QP->head = QP->tail = QP->num_elems = 0;
    QP->size = 64; // We start the queue with a size of 64 units and increase it if necessary.
    QP->elem = malloc(sizeof(value) * QP->size);

    return QX;
}

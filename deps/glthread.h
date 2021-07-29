#ifndef GLTHREAD_H
#define GLTHREAD_H

/* Macros */

#define IS_GLTHREAD_EMPTY(glthreadptr) ((glthreadptr)->next == 0 && (glthreadptr)->prev == 0)

#define BASE(glthreadptr) ((glthreadptr)->next)

#define ITERATE_GLTHREAD_BEGIN(glthreadbegin, glthreadptr) {                                      \
	glthread_t* _glthread_ptr = NULL;                                                               \
	glthreadptr = BASE(glthreadbegin);                                                              \
	                                                                                                \
	for (; glthreadptr; glthreadptr = _glthread_ptr) {                                              \
		_glthread_ptr = (glthreadptr)->next;                                                          \

#define ITERATE_GLTHREAD_END(glthreadend, glthreadptr)                                            \
	}}

#define GET_DATA_AT_OFFSET(glthreadptr, offset) (void*)((char*)(glthreadptr) - offset)

/**
 * @brief glthread meta container
 */
typedef struct glthread {
	struct glthread* prev;
	struct glthread* next;
} glthread_t;

void glthread_init(glthread_t* glthread);

void glthread_insert_after(glthread_t* mark, glthread_t* next);

void glthread_insert_before(glthread_t* mark, glthread_t* next);

void glthread_remove(glthread_t* mark);

void glthread_push(glthread_t* head, glthread_t* next);

void glthread_del_list(glthread_t* head);

unsigned int glthread_size(glthread_t* head);

void glthread_priority_insert(
	glthread_t* head,
	glthread_t* glthread,
	int(*comparator)(void*, void*),
	int offset
);

glthread_t* glthread_dequeue_first(glthread_t* head);

#endif /* GLTHREAD_H */

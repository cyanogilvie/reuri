#include "reuriInt.h"
#error Stack

void* stack_push(struct stack_entry** stack_top, size_t len) //<<<
{
	struct stack_entry*	new_top = (struct stack_entry*)ckalloc(sizeof *new_top);

	new_top->prev = *stack_top;
	new_top->thing = ckalloc(len);
	*stack_top = new_top;

	return new_top->thing;
}

//>>>
void* stack_pop(struct stack_entry** stack_top) //<<<
{
	void*	thing = NULL;

	if (*stack_top) {
		struct stack_entry*	old_top = *stack_top;

		thing = (*stack_top)->thing;
		*stack_top = (*stack_top)->prev;

		old_top->thing = NULL;
		ckfree(old_top);
		old_top = NULL;
	}

	return thing;
}

//>>>
size_t stack_size(struct stack_entry* stack_top) //<<<
{
	size_t				count = 0;
	struct stack_entry*	e = stack_top;

	while (e) {
		count++;
		e = e->prev;
	}

	return count;
}

//>>>
void stack_discard(struct stack_entry** stack_top) //<<<
{
	struct stack_entry* e = *stack_top;

	while (e) {
		void*	d = stack_pop(&e);
		ckfree(d);
		d = NULL;
	}

	*stack_top = NULL;
}

//>>>

// vim: ft=c foldmethod=marker foldmarker=<<<,>>>

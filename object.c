#include <assert.h>
#include <stdlib.h>

#include <stdio.h>

#include "debug.h"
#include "object.h"

Derp_obj** heap = NULL;
size_t heap_size = 8;
int next_free_slot = 0;

void object_init(Derp_obj* obj) {
	obj->type = TYPE_OBJECT;

	obj->flags = 0;
}

void object_heap_dump() {
	debug("heap: %p %p %p %p", heap[0], heap[1], heap[2], heap[3]);
}

Derp_obj* object_create(Derp_vm* vm) {
	if (heap == NULL) {
		debug("allocating heap of size %zd (each object %zd bytes)", heap_size, sizeof(Derp_obj));
		heap = malloc(sizeof(Derp_obj*) * heap_size);
		for (int i = 0; i < heap_size; ++i)
		{
			heap[i] = NULL;
		}
	}

	if (next_free_slot >= heap_size) {
		debug("OUT OF HEAP, performing GC");
		vm_gc_mark(vm);
		object_sweep();

		if (next_free_slot >= heap_size) {
			log_err("GC failed to free up space!");
			return NULL;
		}
	}

	Derp_obj *obj = malloc(sizeof(Derp_obj));
	check_mem(obj);

	debug("allocating object in slot %d", next_free_slot);
	heap[next_free_slot] = obj;
	next_free_slot++;
	object_heap_dump();

	object_init(obj);

	return obj;

error:
	return NULL;
}

void object_sweep() {
	debug("sweeping heap of %d", next_free_slot);
	object_heap_dump();
	for (int i = 0; i < heap_size; i++)
	{
		Derp_obj* obj = heap[i];
		if (obj == NULL) continue;
		if (obj->marked) {
			debug("skipping marked object");
			obj->marked = false;
		} else {
			debug("found unmarked object! (%p)", obj);
			object_destroy(obj);

			// recompact the heap
			next_free_slot--;
			heap[i] = heap[next_free_slot];
			heap[next_free_slot] = NULL;
			debug("heap after removal:");
			object_heap_dump();
		}
	}
	debug("sweep finished, heap now (%d/%zd)", next_free_slot, heap_size);
	object_heap_dump();
}

void derp_obj_freeze(Derp_obj *obj) {
	obj->flags |= FLAG_FROZEN;
}

void object_destroy(Derp_obj *obj) {
	// puts("derp obj destroyed");
	assert(obj != NULL);

	if (obj->type == TYPE_STRING && obj->str_val != NULL) {
		free(obj->str_val);
	}

	free(obj);
}

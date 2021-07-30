#include "libchron.h"

/* MACROS (opaque) */

/* Getters */
#define CHRON_TW_GET_SLOT(tw, idx) (&(tw->slots[idx]))

#define CHRON_TW_GET_SLOT_HEAD(tw, idx) (&(tw->slots[idx].linked_list))

#define CHRON_TW_GET_SLOT_MUTEX(tw, idx) (&(tw->slots[idx].mutex))

#define CHRON_TW_GET_UPTIME(tw) (CHRON_TW_GET_ABS_SLOT_N(tw) * tw->tick_interval)

#define CHRON_TW_GET_SLOTS_AT_IDX(tw, idx) (&(tw->slots[idx]))

// the *absolute* slot number since the wheel routine began
#define CHRON_TW_GET_ABS_SLOT_N(tw)	((tw->n_revolutions * tw->ring_size) + tw->current_tick)

#define CHRON_TW_GET_WAITLIST(tw) (&(tw->waitlist))

#define CHRON_TW_GET_WAITLIST_HEAD(tw) (&((CHRON_TW_GET_WAITLIST(tw))->linked_list))

#define CHRON_TW_GET_SLOT_EMPTY(slot) (IS_GLTHREAD_EMPTY(&(slot->linked_list)))

/* Setters */
#define CHRON_TW_SET_LOCK_SLOT(slot) pthread_mutex_lock(&(slot->mutex))

#define CHRON_TW_SET_LOCK_SLOT(slot) pthread_mutex_unlock(&(slot->mutex))

/* HELPERS */

void __reschedule_slot(chron_timer_wheel_t* tw) {
	glthread_t* current_node;
	chron_tw_slot_el_t* el;

	// lock the waitlist
	CHRON_TW_SET_LOCK_SLOT(CHRON_TW_GET_WAITLIST(tw));

	if (CHRON_TW_GET_SLOT_EMPTY(CHRON_TW_GET_WAITLIST(tw))) {
		CHRON_TW_SET_UNLOCK_SLOT(CHRON_TW_GET_WAITLIST(tw));
		return;
	}

	ITERATE_GLTHREAD_BEGIN(CHRON_TW_GET_WAITLIST_HEAD(tw), current_node) {
		// TODO
	} ITERATE_GLTHREAD_END(CHRON_TW_GET_WAITLIST_HEAD(tw), current_node);
}

int __insert_el_in_slot(void* data1, void* data2) {
   chron_tw_slot_el_t* el1 = (chron_tw_slot_el_t* )data1;
   chron_tw_slot_el_t* el2 = (chron_tw_slot_el_t* )data2;

   if (el1->r < el2->r) return -1;

   if (el1->r > el2->r) return 1;

   return 0;
}

/**
 * @brief Opaque helper. The thread routine on which the timer wheel runs
 *
 * @param arg
 * @return void*
 */
void* __timer_routine(void* arg) {
	chron_timer_wheel_t* tw = (chron_timer_wheel_t*)arg;
	chron_tw_slot_el_t* el = NULL;

	int abs_slot_n = 0,
					 i = 0;

	chron_tw_slot* slot = NULL;
	glthread_t* current_node;

	while (true) {
		tw->current_tick++;

		// start a new revolution, if necessary
		if (tw->current_tick == tw->ring_size) {
			tw->current_tick = 0;
			tw->n_revolutions++;
		}

		sleep(tw->tick_interval);

		// retrieve the current slot (linked list of event els)
		slot = CHRON_TW_GET_SLOT(tw, tw->current_tick);

		// and the absolute slot number
		abs_slot_n = CHRON_TW_GET_ABS_SLOT_N(tw);

		/* now, we iterate the slot's linked list and
			1) invoke those events which satisfy the criterion (the *r* value)
			2) reschedule events that are marked as interval, or periodic
				a) this process entails calculating the next slot for said events
		*/
		ITERATE_GLTHREAD_BEGIN(&slot->linked_list, current_node) {
			el = __glthread_to_el(current_node); // TODO

			// evaluate whether the current slot element's r variable == n revolutions
			if (tw->n_revolutions == el->r) {
				// if so, invoke the event...
				el->callback(el->callback_arg, el->arg_size);

				// ...and check whether it needs to be rescheduled
				if (el->is_recurring) {
					// reschedule in the next slot

					// get the next available slot
					int next_abs_slot_n = abs_slot_n + (el->interval / tw->tick_interval);

					// determine the next revolution
					int next_revolution_n = next_abs_slot_n / tw->ring_size;

					// finally, the next slot number
					int next_slot_n = next_abs_slot_n % tw->ring_size;

					el->r = next_revolution_n;

					// remove old linked list node and insert a new one
					glthread_remove(&el->linked_list_node);
					glthread_priority_insert(
						CHRON_TW_GET_SLOT_HEAD(tw, next_slot_n),
						&el->linked_list_node,
						__insert_el_in_slot,
						(unsigned long)&((chron_tw_slot_el_t*)0)->linked_list_node
					);

					// assign head of the slot to which the element belongs
					el->slot_head = CHRON_TW_GET_SLOT(tw, next_slot_n);
					el->slot_n = next_slot_n;

					el->n_scheduled++;
				}
			} else break;
		} ITERATE_GLTHREAD_END(slot, current_node);

	__reschedule_slot(tw);
	}

	return NULL;
}

/* PUBLIC API */

/**
 * @brief Initialize a timer wheel, allocating memory for its slots
 *
 * @param size Size of the internal ring buffer aka num of slots
 * @param tick_interval
 * @return chron_timer_wheel_t*
 */
chron_timer_wheel_t* chron_timer_wheel_init(int size, int tick_interval) {
	chron_timer_wheel_t* wt = malloc(
		// we want to calculate using the size of the linked list (slot) pointer
		sizeof(chron_timer_wheel_t) + size * sizeof(chron_tw_slot*)
	);

	if (!wt) return NULL;

	wt->tick_interval = tick_interval;
	wt->ring_size = size;
	wt->n_revolutions = 0;

	memset(&(wt->thread), 0, sizeof(chron_timer_wheel_t));

	// for each slot in the ring buffer...
	for (int i = 0; i < size; i++) {
		glthread_init(CHRON_TW_GET_SLOT_HEAD(wt, i)); // initialize the slot's linked list
		pthread_mutex_init(CHRON_TW_GET_SLOT_MUTEX(wt, i), NULL);
	}

	wt->n_slots = 0;

	return wt;
}

/**
 * @brief Start the timer wheel on a separate thread
 *
 * @param tw
 * @return bool
 */
bool chron_timer_wheel_start(chron_timer_wheel_t* tw) {
	if (pthread_create(&tw->thread, NULL, __timer_routine, (void*)tw)) {
		return false;
	}

	return true;
}

#ifndef LIB_CHRON_H
#define LIB_CHRON_H

#include "glthread.h"

#include <signal.h>
#include <time.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum {
	TIMER_INIT,
	TIMER_RUNNING,
	TIMER_CANCELLED,
	TIMER_DELETED,
	TIMER_PAUSED,
	TIMER_RESUMED
} chron_timer_state;

typedef struct chron_timer {
	/* A POSIX timer */
	timer_t timer;

	/* Timer callback function */
	void(*callback)(struct chron_timer*, void*);

	/* Timer callback argument */
	void* callback_arg;

	/* Expiration time in ms */
	unsigned long exp_time;

	/* Interval in ms */
	unsigned long exp_interval;

	/* Threshold at which we no longer invoke */
	uint32_t threshold;

	/* Indicates whether the timer is exponential */
	bool is_exponential;

	/* Persists time remaining  */
	unsigned long time_remaining;

	/* Counts callback invocations */
	uint32_t invocation_count;

	/* Expiration time */
	struct itimerspec ts;

	/* Time val for exponent p*/
	unsigned long exponential_backoff_time;

	/* Current state of timer */
	chron_timer_state timer_state;
} chron_timer_t;

chron_timer_t* chron_timer_init(
	void (*callback)(chron_timer_t* timer, void* arg),
	void* callback_arg,
	unsigned long expiry_ms,
	unsigned long interval_ms, // subsequent to initial expiry
	uint32_t max_expirations, // 0 for infinite
	bool is_exponential
);

bool chron_timer_toggle(chron_timer_t* timer);

void chron_timer_start(chron_timer_t* timer);

unsigned long chron_timer_get_ms_remaining(chron_timer_t* timer);

bool chron_timer_pause(chron_timer_t* timer);

bool chron_timer_resume(chron_timer_t* timer);

bool chron_timer_restart(chron_timer_t* timer);

bool chron_timer_cancel(chron_timer_t* timer);

void chron_timer_print(chron_timer_t* timer);

bool chron_timer_reschedule(
	chron_timer_t* timer,
	unsigned long exp_time,
	unsigned long exp_interval
);

bool chron_timer_delete(chron_timer_t* timer);

#endif /* LIB_CHRON_H */

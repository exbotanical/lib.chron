#include "libchron.h"

#include <stdlib.h>
#include <memory.h>

/**
 * @brief Opaque helper. Set the itimerspec ms and ns
 *
 * @param ts
 * @param ms
 */
void __set_itimerspec(struct timespec* ts, unsigned long ms) {
	memset(ts, 0, sizeof(struct timespec));

	if (!ms) return;

	unsigned long s = ms / 1000;
	ts->tv_sec = s;

	unsigned long remaining_ms = ms % 1000;

	ts->tv_nsec = remaining_ms * (1000000);
}

/**
 * @brief Opaque helper. Convert timespec to ms
 *
 * @param time
 * @return unsigned
 */
unsigned long __timespec_to_ms(struct timespec* time) {
	unsigned long ms = 0;
	ms = time->tv_sec * 1000;

  ms += time->tv_nsec / 1000000;

	return ms;
}

/**
 * @brief Opaque helper. Timer callback wrapper
 *
 * @param arg
 */
void __callback_wrapper(union sigval arg) {
	chron_timer_t* timer = (chron_timer_t*)(arg.sival_ptr);

	// if the state is TIMER_RESUMED, the timer was *just* resumed, so
	// we can set the state to TIMER_RUNNING...
	// provided the timer still has running time (otherwise, we would be stuck in a loop)
	if (chron_timer_get_state(timer) == TIMER_RESUMED && timer->exp_time != 0) {
		chron_timer_set_state(timer, TIMER_RUNNING);
	}

	(timer->callback)(timer, timer->callback_arg);
}

/**
 * @brief Initialize a new chron_timer
 *
 * @param callback
 * @param callback_arg
 * @param expiry_ms
 * @param interval_ms
 * @param max_expirations
 * @param is_exponential
 * @return chron_timer_t*
 */
chron_timer_t* chron_timer_init(
	void (*callback)(chron_timer_t* timer, void* arg),
	void* callback_arg,
	unsigned long expiry_ms,
	unsigned long interval_ms, // subsequent to initial expiry
	uint32_t max_expirations, // 0 for infinite
	bool is_exponential
) {
	chron_timer_t* timer = malloc(sizeof(chron_timer_t));

	if (!timer) return NULL;

	timer->callback = callback;
	timer->callback_arg = callback_arg;
	timer->exp_time = expiry_ms;
	timer->exp_interval = interval_ms;
	timer->is_exponential = is_exponential;
	timer->threshold = max_expirations;

	chron_timer_set_state(timer, TIMER_INIT);

	struct sigevent evp;
	memset(&evp, 0, sizeof(struct sigevent));

	evp.sigev_value.sival_ptr = (void*)(timer);
	evp.sigev_notify = SIGEV_THREAD;
	evp.sigev_notify_function = __callback_wrapper;

	if (timer_create(CLOCK_REALTIME, &evp, &timer->timer) < 0) {
		return NULL;
	}

	__set_itimerspec(&timer->ts.it_value, timer->exp_time);

	if (!timer->is_exponential) {
		__set_itimerspec(&timer->ts.it_interval, timer->exp_interval);
		timer->exponential_backoff_time = 0;
	} else {
		timer->exponential_backoff_time = __timespec_to_ms(&timer->ts.it_value);
		__set_itimerspec(&timer->ts.it_interval, 0);
	}

	return timer;
}

/**
 * @brief Resurrect a timer that is not running; stop a running timer.
 * To stop the timer, the itimerspec members (ts)
 * must be set to zero.
 *
 * @param timer
 * @return bool true only if toggle succeeded
 */
bool chron_timer_toggle(chron_timer_t* timer) {
	if (timer_settime(timer->timer, 0, &timer->ts, NULL) < 0) {
		return false;
	}

	return true;
}

/**
 * @brief Start a stopped timer
 *
 * @param timer
 */
void chron_timer_start(chron_timer_t* timer) {
	chron_timer_toggle(timer);
	chron_timer_set_state(timer, TIMER_RUNNING);
}

/**
 * @brief Get the remaining time of the given chron_timer in ms
 *
 * @param timer
 * @return unsigned long
 */
unsigned long chron_timer_get_ms_remaining(chron_timer_t* timer) {
	struct itimerspec time_remaining;

	memset(&time_remaining, 0, sizeof(struct itimerspec));

	timer_gettime(timer->timer, &time_remaining);

	return __timespec_to_ms(&time_remaining.it_value);
}

/**
 * @brief Pause a running timer
 *
 * @param timer
 * @return bool true only if pause succeeded
 */
bool chron_timer_pause(chron_timer_t* timer) {
	if (chron_timer_get_state(timer) == TIMER_PAUSED) return false;

	timer->time_remaining = chron_timer_get_ms_remaining(timer);

	__set_itimerspec(&timer->ts.it_value, 0);
	__set_itimerspec(&timer->ts.it_interval, 0);

	if (!chron_timer_toggle(timer)) return false;

	chron_timer_set_state(timer, TIMER_PAUSED);

	return true;
}

/**
 * @brief Resume a paused timer
 *
 * @param timer
 * @return bool true only if resume succeeded
 */
bool chron_timer_resume(chron_timer_t* timer) {
	if (chron_timer_get_state(timer) == TIMER_RESUMED) return false;

	__set_itimerspec(&timer->ts.it_value, timer->time_remaining);
	__set_itimerspec(&timer->ts.it_interval, timer->exp_interval);
	timer->time_remaining = 0;

	if (!chron_timer_toggle(timer)) return false;

	chron_timer_set_state(timer, TIMER_RESUMED);

	return true;
}

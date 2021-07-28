#include "libchron.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static void callback(chron_timer_t* this, void* data) {
	printf("data = %s\n", (char*)data);
}

int main(int argc, char* argv[]) {
	char *str = "demo";

	chron_timer_t* timer = chron_timer_init(
		callback,
		str,
		1000,
		1000,
		0,
		false
	);

	assert(timer != NULL);

	chron_timer_start(timer);

	pause();
	return EXIT_SUCCESS;
}

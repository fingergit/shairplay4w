/*
 * Starts the AirPlay server (name "FakePort") and dumps the received contents
 * to files:
 * - audio.pcm : decoded audio content
 * - metadata.bin : meta data
 * - covertart.jpg : cover art
 *
 * Requires avahi-daemon to run. Also requires file "airplay.key" in the same directory.
 *
 * Compile with: gcc -o example -g -I../../include/shairplay example.c -lshairplay 
 */

#include <stdlib.h>
#include <stdio.h>
// #include <unistd.h>

#ifdef WIN32
#include <windows.h>
#endif

#include "dnssd.h"
#include "raop.h"

#include "callback.h"

static int running;

#ifndef WIN32

#include <signal.h>
static void
signal_handler(int sig)
{
	switch (sig) {
	case SIGINT:
	case SIGTERM:
		running = 0;
		break;
	}
}
static void
init_signals(void)
{
	struct sigaction sigact;

	sigact.sa_handler = signal_handler;
	sigemptyset(&sigact.sa_mask);
	sigact.sa_flags = 0;
	sigaction(SIGINT, &sigact, NULL);
	sigaction(SIGTERM, &sigact, NULL);
}

#endif

int
main(int argc, char *argv[])
{
	const char *name = "FakePort";
	unsigned short raop_port = 5000;
	const char hwaddr[] = { 0x48, 0x5d, 0x60, 0x7c, 0xee, 0x22 };

	dnssd_t *dnssd;
	raop_t *raop;
	raop_callbacks_t raop_cbs;
	memset(&raop_cbs, 0, sizeof(raop_callbacks_t));

	int error;

	raop_cbs.cls = NULL;
	raop_cbs.audio_init = audio_init;
	raop_cbs.audio_set_volume = audio_set_volume;
	raop_cbs.audio_set_metadata = audio_set_metadata;
	raop_cbs.audio_set_coverart = audio_set_coverart;
	raop_cbs.audio_process = audio_process;
	raop_cbs.audio_flush = audio_flush;
	raop_cbs.audio_destroy = audio_destroy;

	raop = raop_init_from_keyfile(10, &raop_cbs, "airport.key", NULL);
	if (raop == NULL) {
		fprintf(stderr, "Could not initialize the RAOP service (airport.key missing?)\n");
		return -1;
	}

	raop_set_log_level(raop, RAOP_LOG_DEBUG);
	raop_set_log_callback(raop, &raop_log_callback, NULL);
	raop_start(raop, &raop_port, hwaddr, sizeof(hwaddr), NULL);

	error = 0;
	dnssd = dnssd_init(&error);
	if (error) {
		fprintf(stderr, "ERROR: Could not initialize dnssd library!\n");
		fprintf(stderr, "------------------------------------------\n");
		fprintf(stderr, "You could try the following resolutions based on your OS:\n");
		fprintf(stderr, "Windows: Try installing http://support.apple.com/kb/DL999\n");
		fprintf(stderr, "Debian/Ubuntu: Try installing libavahi-compat-libdnssd-dev package\n");
		raop_destroy(raop);
		return -1;
	}

	dnssd_register_raop(dnssd, name, raop_port, hwaddr, sizeof(hwaddr), 1);

	printf("Startup complete... Kill with Ctrl+C\n");

	running = 1;
	while (running != 0) {
#ifndef WIN32
		sleep(1);
#else
		Sleep(1000);
#endif
	}

	dnssd_unregister_raop(dnssd);
	dnssd_destroy(dnssd);

	raop_stop(raop);
	raop_destroy(raop);

	return 0;
}


#include <stdio.h>

#include <noch/log.h>
#include <noch/log.c>

int main(void) {
	logSetFile(stdout);
	logSetFlags(LOG_TIME_DATE | LOG_LOCATION);

/* Custom logs */
#define logCustom(...) logGeneric(LOG_GREEN, "HELLO", __FILE__, __LINE__, __VA_ARGS__)

	logCustom("Hello, this is my custom log");
	logInfo("Info log");
	logWarn("Warning log");
	logError("Error log");
	logFatal("Fatal error log (exiting)");
	/* logFatal calls exit(EXIT_FAILURE) */
}

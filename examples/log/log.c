#include <stdio.h>

#include <noch/log.h>
#include <noch/log.c>

int main(void) {
	set_log_file(stdout);
	set_log_flags(LOG_TIME_DATE | LOG_LOCATION);

/* Custom logs */
#define LOG_CUSTOM(...) log_generic(LOG_GREEN, "HELLO", __FILE__, __LINE__, __VA_ARGS__)

	LOG_CUSTOM("Hello, this is my custom log");

	LOG_INFO("Info log");
	LOG_WARN("Warning log");
	LOG_ERROR("Error log");
	LOG_FATAL("Fatal error log (exiting)");
	/* LOG_FATAL calls exit() */
}

#define MAX_CHANNELS 64
struct event_data
{
	char events[MAX_CHANNELS][128];
	int channels[MAX_CHANNELS];
	int timeout;
	int emactivity;
};

struct event_data eventData;

extern void *eventMonitor();

#define MAX_CHANNELS    64
struct event_data
{
  int channels[MAX_CHANNELS];
  pthread_t tid[MAX_CHANNELS];
  int timeout;
  int *activity;
};

struct event_data eventData;

extern void *eventMonitor();


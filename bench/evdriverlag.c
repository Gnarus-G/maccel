#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#define MOUSE_DEVICE "/dev/input/event2"

static __suseconds_t to_us(struct timeval time) {
  return time.tv_sec * 1000000L + time.tv_usec;
}

#define EVENT_TIME_PAIR_CNT 1000 * 2

int main(void) {
  // Open the source device
  int fd_source = open(MOUSE_DEVICE, O_RDONLY);
  if (fd_source < 0) {
    perror("Failed to open source device");
    return 1;
  }

  __suseconds_t times[EVENT_TIME_PAIR_CNT] = {0};
  int tidx = 0;

  // Main loop to read, modify, and write events
  struct input_event ev;
  while (tidx < EVENT_TIME_PAIR_CNT && read(fd_source, &ev, sizeof(ev)) > 0) {
    struct timeval now;
    gettimeofday(&now, NULL);

    times[tidx++] = to_us(ev.time);
    times[tidx++] = to_us(now);
  }

  // Cleanup
  close(fd_source);

  tidx = 0;

  printf("event_time,read_time,diff\n"); // eve'ry is in us
  while (tidx < EVENT_TIME_PAIR_CNT) {
    __suseconds_t event_time = times[tidx++];
    __suseconds_t read_time = times[tidx++];
    /* __suseconds_t diff =  ; */

    /* printf("event time: %luus, read time: %luus\n", event_time, read_time);
     */
    /* printf("time diff: %luus\n", read_time - event_time); */
    printf("%lu,%lu,%lu\n", event_time, read_time, read_time - event_time);
  }

  return 0;
}

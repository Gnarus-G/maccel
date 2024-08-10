#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#define MOUSE_DEVICE "/dev/input/event2"

#define VIRTUAL_DEVICE "/dev/input/event9"

static __suseconds_t to_us(struct timeval time) {
  return time.tv_sec * 1000000L + time.tv_usec;
}

#define EVENT_TIME_PAIR_CNT 1000 * 3

/**
 * NOTE: This benchmark depends on temporarily setting
 * maccel_filter from input_handler.h to return false
 * on EV_REL instead of true. So we can measure the original
 * event time from the physical device's event.
 *
 */

int main(void) {
  // Open the source device
  int fd_source = open(MOUSE_DEVICE, O_RDONLY);
  if (fd_source < 0) {
    perror("Failed to open source device");
    return 1;
  }

  int vfd_source = open(VIRTUAL_DEVICE, O_RDONLY);
  if (fd_source < 0) {
    perror("Failed to open virtual device");
    return 1;
  }

  __suseconds_t times[EVENT_TIME_PAIR_CNT] = {0};
  int tidx = 0;

  // Main loop to read, modify, and write events
  struct input_event sev;
  struct input_event vev;
  while (tidx < EVENT_TIME_PAIR_CNT &&
         read(vfd_source, &vev, sizeof(vev)) > 0) {
    struct timeval now;
    gettimeofday(&now, NULL);

    if (read(fd_source, &sev, sizeof(sev)) <= 0) {
      break;
    }

    times[tidx++] = to_us(sev.time);
    times[tidx++] = to_us(vev.time);
    times[tidx++] = to_us(now);
  }

  // Cleanup
  close(fd_source);

  tidx = 0;

  printf("event_time,virtual_event_time,read_time,diff\n"); // eve'ry is in us
  while (tidx < EVENT_TIME_PAIR_CNT) {
    __suseconds_t event_time = times[tidx++];
    __suseconds_t virtual_event_time = times[tidx++];
    __suseconds_t read_time = times[tidx++];

    printf("%lu,%lu,%lu,%lu\n", event_time, virtual_event_time, read_time,
           read_time - event_time);
  }

  return 0;
}

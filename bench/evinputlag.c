#include <fcntl.h>
#include <linux/input.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#define VIRTUAL_DEVICE "/dev/input/event7"

static __suseconds_t to_us(struct timeval time) {
  return time.tv_sec * 1000000L + time.tv_usec;
}

#define EVENT_TIME_PAIR_CNT 1000 * 3

/**
 * NOTE: This benchmark depends on temporarily setting
 * maccel_filter from input_handler.h to inject ktime_t diff (in us)
 * between when the original event was received and the modified event was
 * reported, as the value of the REL_Z REL_ABS event. So we can measure the
 * extra lag from the input_handler. Assuming REL_Z, REL_ABS are only produced
 * as carriers of the extra lag measurement.
 *
 */

int main(void) {
  int fd_source = open(VIRTUAL_DEVICE, O_RDONLY);
  if (fd_source < 0) {
    perror("Failed to open virtual device");
    return 1;
  }

  __suseconds_t times[EVENT_TIME_PAIR_CNT] = {0};
  int tidx = 0;

  // Main loop to read, modify, and write events
  struct input_event ev;
  while (tidx < EVENT_TIME_PAIR_CNT && read(fd_source, &ev, sizeof(ev)) > 0) {
    struct timeval now;
    gettimeofday(&now, NULL);

    if (ev.type == EV_REL && (ev.code == 2 || ev.code == 3)) {
      /* fprintf(stderr, "EVENT: type %d, code %d, value %d\n", ev.type,
       * ev.code, */
      /*         ev.value); */
      times[tidx++] = to_us(ev.time);
      times[tidx++] = to_us(now);
      times[tidx++] = ev.value;
    }
  }

  // Cleanup
  close(fd_source);

  tidx = 0;

  printf("event_time,read_time,naive_diff,extra_lag,diff\n"); // eve'ry is in us
  while (tidx < EVENT_TIME_PAIR_CNT) {
    __suseconds_t event_time = times[tidx++];
    __suseconds_t read_time = times[tidx++];
    __suseconds_t extra_lag_measured_by_input_handler = times[tidx++];

    __suseconds_t diff = read_time - event_time;
    printf("%lu,%lu,%lu,%lu,%lu\n", event_time, read_time, diff,
           extra_lag_measured_by_input_handler,
           diff + extra_lag_measured_by_input_handler);
  }

  return 0;
}

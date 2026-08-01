#include <errno.h>
#include <linux/limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#define COLOR_RESET "\033[0m"
#define COLOR_BOLD "\033[1m"
#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_CYAN "\033[36m"

static int colors_enabled(FILE *stream) {
  return isatty(fileno(stream)) && getenv("NO_COLOR") == NULL;
}

/* These prefixes track format strings emitted by Greatest 1.5.0. */
static const char *test_output_color(const char *format) {
  if (strcmp(format, ".") == 0 || strncmp(format, "PASS ", 5) == 0)
    return COLOR_GREEN;
  if (strcmp(format, "F") == 0 || strncmp(format, "FAIL ", 5) == 0)
    return COLOR_RED;
  if (strcmp(format, "s") == 0 || strncmp(format, "SKIP ", 5) == 0)
    return COLOR_YELLOW;
  if (strncmp(format, "\n* Suite ", 9) == 0)
    return COLOR_CYAN;
  if (strncmp(format, "\nTotal: ", 8) == 0 || strncmp(format, "Pass: ", 6) == 0)
    return COLOR_BOLD;
  return NULL;
}

static int test_fprintf(FILE *stream, const char *format, ...) {
  const char *color = colors_enabled(stream) ? test_output_color(format) : NULL;
  va_list args;
  int result;

  if (color != NULL)
    fputs(color, stream);
  va_start(args, format);
  result = vfprintf(stream, format, args);
  va_end(args);
  if (color != NULL)
    fputs(COLOR_RESET, stream);
  return result;
}

#define GREATEST_FPRINTF test_fprintf
#include "vendor/greatest.h"

#define TEST_MAIN_BEGIN()                                                      \
  do {                                                                         \
    GREATEST_MAIN_BEGIN();                                                     \
    if (greatest_get_verbosity() == 0)                                         \
      greatest_set_verbosity(1);                                               \
  } while (0)

static void print_diff(const char *content, const char *filename) {
  int pipe_fd[2];
  pid_t child_pid;
  struct sigaction ignore_sigpipe = {.sa_handler = SIG_IGN};
  struct sigaction previous_sigpipe;

  fflush(stdout);
  if (pipe(pipe_fd) == -1) {
    perror("failed to create snapshot diff pipe");
    return;
  }

  if ((child_pid = fork()) == -1) {
    perror("failed to fork snapshot diff");
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    return;
  }

  if (child_pid == 0) {
    close(pipe_fd[1]);
    dup2(pipe_fd[0], STDIN_FILENO);
    close(pipe_fd[0]);
    if (colors_enabled(stdout))
      execlp("diff", "diff", "-u", "--color=always", filename, "-", NULL);
    else
      execlp("diff", "diff", "-u", filename, "-", NULL);
    perror("failed to execute diff");
    exit(EXIT_FAILURE);
  }

  close(pipe_fd[0]);
  sigemptyset(&ignore_sigpipe.sa_mask);
  if (sigaction(SIGPIPE, &ignore_sigpipe, &previous_sigpipe) == -1) {
    perror("failed to ignore SIGPIPE while writing snapshot diff");
  } else {
    size_t content_length = strlen(content);
    size_t bytes_written = 0;

    while (bytes_written < content_length) {
      ssize_t result = write(pipe_fd[1], content + bytes_written,
                             content_length - bytes_written);
      if (result > 0) {
        bytes_written += result;
      } else if (result == -1 && errno == EINTR) {
        continue;
      } else {
        perror("failed to write snapshot content to diff");
        break;
      }
    }
    if (sigaction(SIGPIPE, &previous_sigpipe, NULL) == -1)
      perror("failed to restore SIGPIPE handler");
  }
  close(pipe_fd[1]);
  waitpid(child_pid, NULL, 0);
}

static int get_current_working_dir(char *buf, size_t buf_size) {
  if (getcwd(buf, buf_size) != NULL) {
    return 0;
  }
  perror("getcwd() error");
  return 1;
}

static char *create_snapshot_file_path(const char *filename) {
  char cwd[PATH_MAX];
  if (get_current_working_dir(cwd, PATH_MAX)) {
    return NULL;
  };

  static char filepath[PATH_MAX];
  if (snprintf(filepath, sizeof(filepath), "%s/tests/snapshots/%s", cwd,
               filename) >= (int)sizeof(filepath)) {
    fprintf(stderr, "snapshot path is too long: %s\n", filename);
    return NULL;
  }
  return filepath;
}

static int assert_snapshot(const char *__filename, const char *content) {
  char *filename = create_snapshot_file_path(__filename);
  if (filename == NULL) {
    return 1;
  }

  int snapshot_file_exists = access(filename, F_OK) != -1;
  FILE *snapshot_file;

  if (snapshot_file_exists) {
    snapshot_file = fopen(filename, "r");
  } else {
    snapshot_file = fopen(filename, "w");
  }

  if (snapshot_file == NULL) {
    fprintf(stderr, "failed to open or create the snapshot file: %s\n",
            filename);
    return 1;
  }

  if (snapshot_file_exists) {
    struct stat stats;
    size_t file_size;

    if (stat(filename, &stats) != 0) {
      perror("failed to stat snapshot file");
      fclose(snapshot_file);
      return 1;
    }
    file_size = (size_t)stats.st_size;
    char *snapshot = (char *)malloc(file_size + 1);

    if (snapshot == NULL) {
      fprintf(stderr,
              "failed to allocate %zd bytes of a string for the snapshot "
              "content in file: %s\n",
              stats.st_size, filename);
      fclose(snapshot_file);
      return 1;
    }

    size_t bytes_read = fread(snapshot, 1, file_size, snapshot_file);
    if (bytes_read != file_size) {
      fprintf(stderr, "failed to read a snapshot file %s\n", filename);
      free(snapshot);
      fclose(snapshot_file);
      return 1;
    }
    snapshot[file_size] = 0; // null byte terminator

    int snapshot_differs = strcmp(snapshot, content);
    free(snapshot);
    if (snapshot_differs)
      print_diff(content, filename);
    fclose(snapshot_file);
    return snapshot_differs != 0;
  } else {
    fprintf(snapshot_file, "%s", content);
    printf("NEW  %s\n", filename);
  }

  fclose(snapshot_file);
  return 0;
}

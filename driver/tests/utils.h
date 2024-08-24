#include "../accel.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

typedef int bool;

static int diff(const char *content, const char *filename) {

  int pipe_fd[2];
  pid_t child_pid;

  // Create a pipe for communication
  if (pipe(pipe_fd) == -1) {
    perror("Pipe creation failed");
    exit(EXIT_FAILURE);
  }

  // Fork the process
  if ((child_pid = fork()) == -1) {
    perror("Fork failed");
    exit(EXIT_FAILURE);
  }

  if (child_pid == 0) { // Child process
    // Close the write end of the pipe
    close(pipe_fd[1]);

    // Redirect stdin to read from the pipe
    dup2(pipe_fd[0], STDIN_FILENO);

    // Execute a command (e.g., "wc -l")
    execlp("diff", "diff", "-u", "--color", filename, "-", NULL);

    // If execlp fails
    perror("Exec failed");
    exit(EXIT_FAILURE);
  } else { // Parent process
    // Close the read end of the pipe
    close(pipe_fd[0]);

    // Write data to the child process
    if (write(pipe_fd[1], content, strlen(content)) == -1) {
      perror("failed to write content to the pipe for diff");
    }

    close(pipe_fd[1]);

    // Wait for the child process to finish
    wait(NULL);
  }

  return 0;
}

static void assert_snapshot(const char *filename, const char *content) {
  bool snapshot_file_exists = access(filename, F_OK) != -1;
  FILE *snapshot_file;

  if (snapshot_file_exists) {
    snapshot_file = fopen(filename, "r");
  } else {
    snapshot_file = fopen(filename, "w");
  }

  if (snapshot_file == NULL) {
    fprintf(stderr, "failed to open or create the snapshot file: %s\n",
            filename);
    exit(1);
  }

  if (snapshot_file_exists) {
    struct stat stats;
    int file_size;

    stat(filename, &stats);
    file_size = stats.st_size;
    char *snapshot = malloc(stats.st_size + 1);

    if (snapshot == NULL) {
      fprintf(stderr,
              "failed to allocate %zd bytes of a string for the snapshot "
              "content in file: %s\n",
              stats.st_size, filename);
      exit(1);
    }

    size_t bytes_read = fread(snapshot, 1, file_size, snapshot_file);
    if (bytes_read != file_size) {
      fprintf(stderr, "failed to read a snapshot file %s\n", filename);
      exit(1);
    }

    int string_test = strcmp(snapshot, content);

    diff(content, filename);

    assert(string_test == 0);
  } else {
    fprintf(snapshot_file, "%s", content);
    printf("created a snapshot file %s\n", filename);
  }

  fclose(snapshot_file);
}

static int test_acceleration(const char *filename, fixedpt param_sens_mult,
                             fixedpt param_accel,
                             fixedpt param_motivity, fixedpt param_gamma, fixedpt param_sync_speed, 
                             fixedpt param_offset, fixedpt param_output_cap) {
  const int LINE_LEN = 26;
  const int MIN = -128;
  const int MAX = 127;

  char content[256 * 256 * LINE_LEN + 1];
  strcpy(content, ""); // initialize as an empty string

  AccelResult result;
  for (int x = MIN; x < MAX; x++) {
    for (int y = MIN; y < MAX; y++) {

      result = f_accelerate(x, y, 1, param_sens_mult, param_accel, 
      param_motivity, param_gamma, param_sync_speed,
      param_offset,param_output_cap);
      char curr_debug_print[LINE_LEN];

      sprintf(curr_debug_print, "(%d, %d) => (%d, %d)\n", x, y, result.x,
              result.y);

      strcat(content, curr_debug_print);
    }
  }

  assert_snapshot(filename, content);

  return 0;
}

#define test(sens_mult, accel, offset, cap)                                    \
  assert(test_acceleration("tests/snapshots/"                                  \
                           "SENS_MULT-" #sens_mult "-ACCEL-" #accel            \
                           "-OFFSET" #offset "-OUTPUT_CAP-" #cap ".snapshot",  \
                           fixedpt_rconst(sens_mult), fixedpt_rconst(accel),   \
                           fixedpt_rconst(offset), fixedpt_rconst(cap)) == 0);

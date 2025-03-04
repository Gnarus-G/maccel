#include <assert.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

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
  sprintf(filepath, "%s/tests/snapshots/%s", cwd, filename);
  return filepath;
}

static void assert_snapshot(const char *__filename, const char *content) {
  char *filename = create_snapshot_file_path(__filename);
  if (filename == NULL) {
    fprintf(stderr, "failed to create snapshot file: %s\n", filename);
    exit(1);
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
    exit(1);
  }

  if (snapshot_file_exists) {
    struct stat stats;
    int file_size;

    stat(filename, &stats);
    file_size = stats.st_size;
    char *snapshot = (char *)malloc(stats.st_size + 1);

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
    snapshot[file_size] = 0; // null byte terminator

    int string_test_diff = strcmp(snapshot, content);

    diff(content, filename);

    /* dbg("diff in content = %d: snapshot '%s' vs now '%s'", string_test, */
    /*     snapshot, content); */
    assert(string_test_diff == 0);
  } else {
    fprintf(snapshot_file, "%s", content);
    printf("created a snapshot file %s\n", filename);
  }

  fclose(snapshot_file);
}

#define print_success printf("[%s]\t\tAll tests passed!\n", __FILE_NAME__)

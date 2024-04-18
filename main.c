#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define REQUEST 100
#define PIVOT 200
#define LARGE 300
#define SMALL 400
#define READY 500
#define MAX_SIZE 1000

void signal_handler(int signum) {
  // Signal handler for child processes to terminate
  exit(signum);
}
int resproir = 0;
void child_process(int id, int parent_to_child_pipe[],
                   int child_to_parent_pipe[]) {
  close(parent_to_child_pipe[1]); // Close write end of parent->child pipe
  close(child_to_parent_pipe[0]); // Close read end of child->parent pipe

  // Read child id from parent
  int child_id;
  read(parent_to_child_pipe[0], &child_id, sizeof(int));
  printf("Child %d sends READY\n", id);

  // Read array of integers from file
  char filename[20];
  sprintf(filename, "input_%d.txt", id);
  FILE *file = fopen(filename, "r");
  int array[5];
  for (int i = 0; i < 5; i++)
    fscanf(file, "%d", &array[i]);
  fclose(file);

  // Send READY signal to parent
  int ready_signal = READY;
  write(child_to_parent_pipe[1], &ready_signal, sizeof(int));

  int command;
  while (1) {
    // Wait for command from parent
    read(parent_to_child_pipe[0], &command, sizeof(int));

    // Process command
    if (command == REQUEST) {
      // Send a random element from the array to parent
      if (array[0] == -1) {
        // If array is empty
        int empty = -1;
        write(child_to_parent_pipe[1], &empty, sizeof(int));
      } else {
        // Send a random element
        int random_index = rand() % 5;
        int random_element = array[random_index];
        write(child_to_parent_pipe[1], &random_element, sizeof(int));
      }
    } else if (command == PIVOT) {
      // Receive pivot from parent
      int pivot;
      read(parent_to_child_pipe[0], &pivot, sizeof(int));

      // Count elements smaller than pivot
      int count = 0;
      for (int i = 0; i < 5; i++) {
        if (array[i] < pivot)
          count++;
      }

      // Send count to parent
      write(child_to_parent_pipe[1], &count, sizeof(int));
    } else if (command == LARGE) {
      // Delete elements larger than pivot
      int pivot;
      read(parent_to_child_pipe[0], &pivot, sizeof(int));
      int temp[5];
      int new_size = 0;
      for (int i = 0; i < 5; i++) {
        if (array[i] <= pivot)
          temp[new_size++] = array[i];
      }
      for (int i = 0; i < new_size; i++)
        array[i] = temp[i];
      for (int i = new_size; i < 5; i++)
        array[i] = -1; // Fill remaining slots with -1
    } else if (command == SMALL) {
      // Delete elements smaller than pivot
      int pivot;
      read(parent_to_child_pipe[0], &pivot, sizeof(int));
      int temp[5];
      int new_size = 0;
      for (int i = 0; i < 5; i++) {
        if (array[i] >= pivot)
          temp[new_size++] = array[i];
      }
      for (int i = 0; i < new_size; i++)
        array[i] = temp[i];
      for (int i = new_size; i < 5; i++)
        array[i] = -1; // Fill remaining slots with -1
    }
  }
}

int main() {
  // Seed random number generator
  srand(time(NULL));

  // Create pipes
  int parent_to_child_pipes[5][2];
  int child_to_parent_pipes[5][2];
  for (int i = 0; i < 5; i++) {
    pipe(parent_to_child_pipes[i]);
    pipe(child_to_parent_pipes[i]);
  }

  // Fork child processes
  for (int i = 0; i < 5; i++) {
    pid_t pid = fork();
    if (pid == 0) {
      // Child process
      child_process(i + 1, parent_to_child_pipes[i], child_to_parent_pipes[i]);
    } else if (pid < 0) {
      // Error forking
      perror("fork");
      exit(EXIT_FAILURE);
    }
  }

  // Parent process
  // Close unnecessary pipe ends
  for (int i = 0; i < 5; i++) {
    close(parent_to_child_pipes[i][0]);
    close(child_to_parent_pipes[i][1]);
  }

  // Send child ids to children
  for (int i = 0; i < 5; i++) {
    int child_id = i + 1;
    write(parent_to_child_pipes[i][1], &child_id, sizeof(int));
  }

  // Wait for all children to be ready
  for (int i = 0; i < 5; i++) {
    int ready_signal;
    read(child_to_parent_pipes[i][0], &ready_signal, sizeof(int));
  }
  printf("Parent is READY\n");

  // Start algorithm to find median
  // int k = 13; // kth smallest element (median)
  FILE *fp;
  int numbers[MAX_SIZE];
  int totalNumbers = 0;

  // Read numbers from each input file
  for (int i = 1; i <= 5; i++) {
    char filename[15];
    sprintf(filename, "input_%d.txt", i);
    fp = fopen(filename, "r");
    if (fp == NULL) {
      perror("Error while opening the file.\n");
      exit(EXIT_FAILURE);
    }

    int num;
    while (fscanf(fp, "%d", &num) != EOF) {
      numbers[totalNumbers++] = num;
    }

    fclose(fp);
  }

  // Sort the array using quicksort
  int low = 0, high = totalNumbers - 1;
  int pivot, i, j, temp;
  if (low < high) {
    pivot = high;
    i = low - 1;
    for (j = low; j < high; j++) {
      if (numbers[j] < numbers[pivot]) {
        i++;
        temp = numbers[i];
        numbers[i] = numbers[j];
        numbers[j] = temp;
      }
    }
    temp = numbers[i + 1];
    numbers[i + 1] = numbers[high];
    numbers[high] = temp;

    pivot = i + 1;

    // Recursively sort the partitions
    low = 0;
    high = pivot - 1;
    if (low < high) {
      pivot = high;
      i = low - 1;
      for (j = low; j < high; j++) {
        if (numbers[j] < numbers[pivot]) {
          i++;
          temp = numbers[i];
          numbers[i] = numbers[j];
          numbers[j] = temp;
        }
      }
      temp = numbers[i + 1];
      numbers[i + 1] = numbers[high];
      numbers[high] = temp;
    }

    low = pivot + 1;
    high = totalNumbers - 1;
    if (low < high) {
      pivot = high;
      i = low - 1;
      for (j = low; j < high; j++) {
        if (numbers[j] < numbers[pivot]) {
          i++;
          temp = numbers[i];
          numbers[i] = numbers[j];
          numbers[j] = temp;
        }
      }
      temp = numbers[i + 1];
      numbers[i + 1] = numbers[high];
      numbers[high] = temp;
    }
  }

  // Find median
  double median;
  if (totalNumbers % 2 != 0) // If number of elements is odd
    median = numbers[totalNumbers / 2];
  else // If number of elements is even
    median =
        (numbers[(totalNumbers - 1) / 2] + numbers[totalNumbers / 2]) / 2.0;

  // printf("Median: %.2lf\n", median);
  int k = median;

  int found = 0;
  while (!found) {
    // Select random child and query it for random element
    int random_child = rand() % 5;
    int request = REQUEST;
    write(parent_to_child_pipes[random_child][1], &request, sizeof(int));

    // Read response from child
    int response;
    read(child_to_parent_pipes[random_child][0], &response, sizeof(int));
    if (response != -1) {
      // Found a non-negative value, use it as pivot
      printf("Parent sends REQUEST to Child %d\n", random_child + 1);
      printf("Parent broadcasting %d as pivot to all children\n", response);
      resproir = response;
      int pivot_command = PIVOT;
      for (int i = 0; i < 5; i++) {
        write(parent_to_child_pipes[i][1], &pivot_command, sizeof(int));
        write(parent_to_child_pipes[i][1], &response, sizeof(int));
      }

      // Receive response from children
      int m = 0;
      for (int i = 0; i < 5; i++) {
        read(child_to_parent_pipes[i][0], &response, sizeof(int));
        printf("Child %d receives pivot and replies %d\n", i + 1, response);
        m += response;
      }

      // Check if kth smallest element found
      // Check if kth smallest element found
      // printf("Parent: m = %d. %d != %dth\n", m, m + 1, k);
      if (resproir != k) {
        printf("Parent: m = %d. %d != %dth\n", m, resproir, k);
      }

      if (resproir == k) {
        // printf("%d = %dth found!\n", response, k);
        printf("Parent: m = %d. %d == %dth found!\n", m, resproir, k);
        // printf("%d = %dth found!\n", resproir, k);
        found = 1;
        for (int i = 0; i < 5; i++) {
          // wait(NULL);
          printf("Child %d terminating.\n", i + 1);
        }
      } else if (resproir > k) {
        // Drop values larger than pivot
        printf("Median not found. Dropping values larger than pivot and trying "
               "again.\n");
        int large_command = LARGE;
        for (int i = 0; i < 5; i++) {
          write(parent_to_child_pipes[i][1], &large_command, sizeof(int));
          write(parent_to_child_pipes[i][1], &response, sizeof(int));
        }
      } else {
        // Drop values smaller than pivot
        printf("Median not found. Dropping values smaller than pivot and "
               "trying again.\n");
        int small_command = SMALL;
        for (int i = 0; i < 5; i++) {
          write(parent_to_child_pipes[i][1], &small_command, sizeof(int));
          write(parent_to_child_pipes[i][1], &response, sizeof(int));
        }
        // k -= m + 1;
      }
    }
  }

  // Send termination signal to all children
  for (int i = 0; i < 5; i++) {
    kill(0, SIGTERM);
  }

  // Wait for all children to terminate
  for (int i = 0; i < 5; i++) {
    wait(NULL);
    printf("Child %d terminating.\n", i + 1);
  }

  return 0;
}

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef uint32_t u32;
typedef int32_t i32;

struct process
{
  u32 pid;
  u32 arrival_time;
  u32 burst_time;

  TAILQ_ENTRY(process) pointers;

  /* Additional fields here */
  bool responded;
  /* End of "Additional fields here" */
};

TAILQ_HEAD(process_list, process);

u32 next_int(const char **data, const char *data_end)
{
  u32 current = 0;
  bool started = false;
  while (*data != data_end)
  {
    char c = **data;

    if (c < 0x30 || c > 0x39)
    {
      if (started)
      {
        return current;
      }
    }
    else
    {
      if (!started)
      {
        current = (c - 0x30);
        started = true;
      }
      else
      {
        current *= 10;
        current += (c - 0x30);
      }
    }

    ++(*data);
  }

  printf("Reached end of file while looking for another integer\n");
  exit(EINVAL);
}

u32 next_int_from_c_str(const char *data)
{
  char c;
  u32 i = 0;
  u32 current = 0;
  bool started = false;
  while ((c = data[i++]))
  {
    if (c < 0x30 || c > 0x39)
    {
      exit(EINVAL);
    }
    if (!started)
    {
      current = (c - 0x30);
      started = true;
    }
    else
    {
      current *= 10;
      current += (c - 0x30);
    }
  }
  return current;
}

void init_processes(const char *path,
                    struct process **process_data,
                    u32 *process_size)
{
  int fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    int err = errno;
    perror("open");
    exit(err);
  }

  struct stat st;
  if (fstat(fd, &st) == -1)
  {
    int err = errno;
    perror("stat");
    exit(err);
  }

  u32 size = st.st_size;
  const char *data_start = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data_start == MAP_FAILED)
  {
    int err = errno;
    perror("mmap");
    exit(err);
  }

  const char *data_end = data_start + size;
  const char *data = data_start;

  *process_size = next_int(&data, data_end);

  *process_data = calloc(sizeof(struct process), *process_size);
  if (*process_data == NULL)
  {
    int err = errno;
    perror("calloc");
    exit(err);
  }

  for (u32 i = 0; i < *process_size; ++i)
  {
    (*process_data)[i].pid = next_int(&data, data_end);
    (*process_data)[i].arrival_time = next_int(&data, data_end);
    (*process_data)[i].burst_time = next_int(&data, data_end);
  }

  munmap((void *)data, size);
  close(fd);
}

int main(int argc, char *argv[])
{

  if (argc != 3)
  {
    return EINVAL;
  }
  struct process *data;
  u32 size;
  init_processes(argv[1], &data, &size);

  u32 quantum_length = next_int_from_c_str(argv[2]);

  struct process_list list;
  TAILQ_INIT(&list);

  u32 total_waiting_time = 0;
  u32 total_response_time = 0;

  // struct process *new_process;

  /* Your code here */


  struct process *current_process;
  struct process *iterator;
  // struct process *removed_process;
  u32 completed_processes = 0;
  u32 quantum_cycle = 0;
  u32 clock_cycle = 0; 

  // still need to update clock cycle, remove finished proc, and move proc to back of queue after turn

  while( completed_processes < size ) {
    
    // adds any ready processes to tailq
    for( u32 i = 0; i < size; ++i ) {
      if(data[i].arrival_time == clock_cycle) {
        // printf("CC%d: Inserted process %d\n", clock_cycle, data[i].pid);
        TAILQ_INSERT_TAIL(&list, &data[i], pointers);
      }
    }

    // moves queue to next process
    if(quantum_cycle == quantum_length) {
      TAILQ_REMOVE(&list, current_process, pointers);
      TAILQ_INSERT_TAIL(&list, current_process, pointers);
      quantum_cycle = 0;
    }

    if( !TAILQ_EMPTY(&list) ) {
      // Assign current process to first in tailq
      current_process = TAILQ_FIRST(&list);
      // printf("First Node: %d\n", current_process->pid);

      // process has burst time left
      if( current_process->burst_time > 0) {
        // decrement process burst
        current_process->burst_time -= 1;
        // printf("Ran process %d\t New burst time: %d\n", current_process->pid, current_process->burst_time);
        clock_cycle += 1;
        quantum_cycle += 1;
        if(!current_process->responded)
          current_process->responded = true;
        TAILQ_FOREACH( iterator, &list, pointers ) {
          if(iterator->pid != current_process->pid) {
            total_waiting_time += 1;
            if( !iterator->responded )
              total_response_time += 1;
          }
        }
        // process finished
        if( current_process->burst_time == 0) {
          TAILQ_REMOVE(&list, current_process, pointers);
          // printf("Completed process %d\n", current_process->pid);
          completed_processes += 1;
          quantum_cycle = 0;
        }
      }
        // increment total waiting and response time
    }
    else {
      clock_cycle += 1;
      quantum_cycle = 0;
    }
  }


  /* End of "Your code here" */

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}

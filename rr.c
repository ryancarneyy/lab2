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

  struct process *new_process;

  /* Your code here */
  for( u32 i = 0; i < size; ++i ) {
    new_process = &data[i];
    TAILQ_INSERT_TAIL(&list, new_process, pointers);
  }

  struct process *current_process = TAILQ_FIRST(&list);
  struct process *removed_process;
  u32 time_running = 0; 

  while(!TAILQ_EMPTY(&list)) {

    if(current_process->arrival_time <= time_running) { // Process has arrived

      // current process has <= time than quant
      if( current_process->burst_time <= quantum_length ) {
        
        // add time to overall time ran
        time_running += current_process->burst_time;
        // make burst time 0
        current_process->burst_time = 0;
        // temp pointer to remove process
        removed_process = current_process;
        printf("Removed process: %d\n", current_process->pid);
        // move pointer to next in tailq
        if(TAILQ_NEXT(current_process, pointers) == NULL) {
        current_process = TAILQ_FIRST(&list);
        }
        else {
          current_process = TAILQ_NEXT(current_process, pointers);
        }
        // remove the finished process from the tailq
        TAILQ_REMOVE(&list, removed_process, pointers);

      }

      // current process is longer than quant
      else {

        // take quant length off of process
        current_process->burst_time -= quantum_length;
        printf("Ran process: %d | Remaining Time: %d\n", current_process->pid, current_process->burst_time);
        // increase time ran by quant
        time_running += quantum_length;

        // move to the next in queue
        if(TAILQ_NEXT(current_process, pointers) == NULL) {
          if( TAILQ_FIRST(&list) != NULL ) {
            current_process = TAILQ_FIRST(&list);
          }
          else 
            break;
        }
        else {
          current_process = TAILQ_NEXT(current_process, pointers);
        }
      }
    }
    else { // Process hasn't arrived

      // move to next in queue
      if(TAILQ_NEXT(current_process, pointers) == NULL) {
        current_process = TAILQ_FIRST(&list);
      }
      else {
        current_process = TAILQ_NEXT(current_process, pointers);
      }
    }
  }

  /* End of "Your code here" */

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}

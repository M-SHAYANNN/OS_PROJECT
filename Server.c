
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>
#include <errno.h>

#define SEM_NAME "/my_sem"
void readSharedMemory(const char* ptr, int size) {
    // Copy shared memory data to a separate buffer
    char* buffer = malloc(size + 1); // Allocate memory for buffer (+1 for null terminator)
    if (buffer == NULL) {
        perror("Memory allocation failed");
        return;
    }
    memcpy(buffer, ptr, size); // Copy shared memory data to buffer
    buffer[size] = '\0'; // Null-terminate the buffer

    printf("Data read from shared memory:\n");
    char* line = buffer;
    char* token;
    while ((token = strsep(&line, "\n")) != NULL) {
        printf("%s\n", token);
    }

    free(buffer); // Free allocated memory
}


int main() {
    const char* name = "OS";
    int shm_fd;
    char* ptr;
    sem_t* sem;
    int read_flag = 0; // Flag to track if memory has been read

    sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("Semaphore initialization failed");
        return 1;
    }
    while (!read_flag) {
        shm_fd = shm_open(name, O_RDWR, 0666);
        if (shm_fd == -1) {
            perror("Shared memory failed");
            return 1;
        }
        struct stat s;
        if (fstat(shm_fd, &s) == -1) {
            perror("fstat");
            return 1;
        }
        int size = s.st_size;
        ptr = mmap(0, size, PROT_READ, MAP_SHARED, shm_fd, 0);
        if (ptr == MAP_FAILED) {
            perror("Map failed");
            return 1;
        }
        sem_wait(sem);
        readSharedMemory(ptr, size);

        sem_post(sem);

        if (munmap(ptr, size) == -1) {
            perror("munmap");
            return 1;
        }

        if (close(shm_fd) == -1) {
            perror("close");
            return 1;
        }

        read_flag = 1; // Set flag to indicate memory has been read
    }

    if (sem_close(sem) == -1) {
        perror("sem_close");
        return 1;
    }

    if (sem_unlink(SEM_NAME) == -1) {
        perror("sem_unlink");
        return 1;
    }

    return 0;
}

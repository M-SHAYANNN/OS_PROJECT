#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>

#define SEM_NAME "/my_sem"
int manipulateAndWriteToFile(char* ptr, int SIZE, FILE* file, sem_t* sem, int* offset) {
    char input[SIZE];

    sem_wait(sem);

    printf("Enter data to write to shared memory (or type 'exit' to stop): ");
    fgets(input, sizeof(input), stdin);

    if (strcmp(input, "exit\n") == 0) {
        sem_post(sem);
        return 1;
    }

    // Remove the newline character from the input string
    size_t input_length = strlen(input);
    if (input[input_length - 1] == '\n') {
        input[input_length - 1] = '\0';
    }

    // Calculate the position to write the new data
    int position = *offset;
    *offset += snprintf(ptr + position, SIZE - position, "%s\n", input);

    // Write to file
    fprintf(file, "%s\n", input);
    fflush(file);  // Flush the output buffer to ensure data is written immediately

    sem_post(sem);

    printf("Data written to shared memory and updated in file successfully.\n");
    return 0;
}



int main() {
    const char* file_name = "example.txt";
    const char* name = "OS";
    const int SIZE = 4096;
    int shm_fd;
    char* ptr;
    sem_t* sem;

    // Open semaphore
    sem = sem_open(SEM_NAME, O_CREAT, 0666, 1);
    if (sem == SEM_FAILED) {
        perror("Semaphore initialization failed");
        return 1;
    }

    // Open shared memory
    shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("Shared memory failed");
        return 1;
    }

    // Resize shared memory
    ftruncate(shm_fd, SIZE);

    // Map shared memory
    ptr = mmap(0, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (ptr == MAP_FAILED) {
        perror("Map failed");
        return 1;
    }

    // Create or truncate file
    FILE* file = fopen(file_name, "w");
    if (file == NULL) {
        perror("Error opening file");
        exit(1);
    }
    fclose(file);

    // Open file for appending
    file = fopen(file_name, "a");
    if (file == NULL) {
        perror("Error opening file");
        exit(1);
    }

    // Loop for writing to shared memory and file
    int num_ofChar = 0;
    while (1) {
        if (manipulateAndWriteToFile(ptr, SIZE, file, sem, &num_ofChar)) {
            break;
        }
    }

    // Close the file
    fclose(file);

    // Read and print content of shared memory
    printf("Shared Memory Content:\n");
    printf("%s\n", ptr);

    // Unmap shared memory
    if (munmap(ptr, SIZE) == -1) {
        perror("munmap");
        return 1;
    }

    // Close shared memory
    close(shm_fd);

    // Close semaphore
    sem_close(sem);

    return 0;
}

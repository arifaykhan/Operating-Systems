#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_REPLICAS 3
#define TOTAL_READERS 10

// 1. Data Structure (Following Lab pattern of grouping shared state)
typedef struct {
    int readers_count[NUM_REPLICAS];
    int waiting_writers;
    int active_writer;
    pthread_mutex_t lock;
    pthread_cond_t can_read;
    pthread_cond_t can_write;
} SystemState;

SystemState state;

// 2. Logging Function (Ensures Rule 3: Mutex for shared writes/IO)
void log_status(const char* action, int id, int replica_id, const char* content) {
    // Note: We are already inside a mutex when this is called in the Entry/Exit sections
    FILE *log_file = fopen("log.txt", "a");
    if (!log_file) return;

    fprintf(log_file, "[%s] Thread:%d | Replicas:[%d, %d, %d] | Writer:%d", 
            action, id, state.readers_count[0], state.readers_count[1], 
            state.readers_count[2], state.active_writer);
    
    if (content) fprintf(log_file, " | Content: %s", content);
    fprintf(log_file, "\n");
    
    // Also print to console for visibility
    printf("[%s] Thread:%d | Replicas:[%d, %d, %d] | Writer:%d\n", 
           action, id, state.readers_count[0], state.readers_count[1], 
           state.readers_count[2], state.active_writer);

    fclose(log_file);
}

// 3. Reader Logic
void* reader_task(void* arg) {
    int id = *(int*)arg;
    free(arg); // Clean up malloc as per Lab best practices

    usleep((rand() % 1000) * 1000); // Random spawn interval

    // --- ENTRY SECTION ---
    pthread_mutex_lock(&state.lock);

    // Rule 4: Always use while(), never if(), with pthread_cond_wait()
    // WRITER PRIORITY: Wait if writer is active OR if a writer is waiting
    while (state.active_writer || state.waiting_writers > 0) {
        pthread_cond_wait(&state.can_read, &state.lock);
    }

    // LOAD BALANCING: Find the replica with the minimum readers
    int best_rep = 0;
    for (int i = 1; i < NUM_REPLICAS; i++) {
        if (state.readers_count[i] < state.readers_count[best_rep]) {
            best_rep = i;
        }
    }
    state.readers_count[best_rep]++;
    log_status("READ_START", id, best_rep, NULL);
    pthread_mutex_unlock(&state.lock);

    // --- CRITICAL SECTION (Reading from File) ---
    char filename[20];
    sprintf(filename, "replica_%d.txt", best_rep);
    FILE *f = fopen(filename, "r");
    if (f) {
        char buffer[100];
        fgets(buffer, sizeof(buffer), f);
        fclose(f);
    }
    usleep((rand() % 500) * 1000); // Simulate reading

    // --- EXIT SECTION ---
    pthread_mutex_lock(&state.lock);
    state.readers_count[best_rep]--;
    log_status("READ_END  ", id, best_rep, NULL);

    // Rule 5: Check condition and signal
    int total_readers = 0;
    for(int i=0; i<NUM_REPLICAS; i++) total_readers += state.readers_count[i];
    
    if (total_readers == 0) {
        pthread_cond_signal(&state.can_write);
    }
    pthread_mutex_unlock(&state.lock);

    return NULL;
}

// 4. Writer Logic
void* writer_task(void* arg) {
    for (int i = 1; i <= 3; i++) {
        usleep(2000000); // Wait between write attempts

        // --- ENTRY SECTION ---
        pthread_mutex_lock(&state.lock);
        state.waiting_writers++; // Signal intent (blocks new readers)

        int total_active_readers = 1; 
        while (total_active_readers > 0 || state.active_writer) {
            total_active_readers = 0;
            for(int j=0; j<NUM_REPLICAS; j++) total_active_readers += state.readers_count[j];
            
            if (total_active_readers > 0 || state.active_writer) {
                pthread_cond_wait(&state.can_write, &state.lock);
            }
        }

        state.waiting_writers--;
        state.active_writer = 1;
        pthread_mutex_unlock(&state.lock);

        // --- CRITICAL SECTION (Updating all replicas) ---
        char content[50];
        sprintf(content, "Update_Batch_%d", i);
        for (int r = 0; r < NUM_REPLICAS; r++) {
            char filename[20];
            sprintf(filename, "replica_%d.txt", r);
            FILE *f = fopen(filename, "w");
            fprintf(f, "%s", content);
            fclose(f);
        }
        usleep(1000000); // Simulate writing time

        // --- EXIT SECTION ---
        pthread_mutex_lock(&state.lock);
        state.active_writer = 0;
        log_status("WRITE_DONE", 99, -1, content);
        
        // Rule 5: Wake all readers after writer finishes
        pthread_cond_broadcast(&state.can_read); 
        pthread_mutex_unlock(&state.lock);
    }
    return NULL;
}

// 5. Main Initialization
int main() {
    srand(time(NULL));

    // Initialize state
    pthread_mutex_init(&state.lock, NULL);
    pthread_cond_init(&state.can_read, NULL);
    pthread_cond_init(&state.can_write, NULL);
    state.waiting_writers = 0;
    state.active_writer = 0;
    for(int i=0; i<NUM_REPLICAS; i++) state.readers_count[i] = 0;

    // Create initial replica files
    for (int i = 0; i < NUM_REPLICAS; i++) {
        char filename[20];
        sprintf(filename, "replica_%d.txt", i);
        FILE *f = fopen(filename, "w");
        fprintf(f, "Initial_Data");
        fclose(f);
    }
    fclose(fopen("log.txt", "w")); // Clear log

    pthread_t r_threads[TOTAL_READERS], w_thread;

    // Task spawning (Using malloc for IDs as shown on Page 10 of PDF)
    pthread_create(&w_thread, NULL, writer_task, NULL);
    for (int i = 0; i < TOTAL_READERS; i++) {
        int* id = malloc(sizeof(int));
        *id = i;
        pthread_create(&r_threads[i], NULL, reader_task, id);
    }

    // Rule 2: Always call pthread_join()
    pthread_join(w_thread, NULL);
    for (int i = 0; i < TOTAL_READERS; i++) {
        pthread_join(r_threads[i], NULL);
    }

    printf("Simulation Complete. Results in log.txt\n");
    return 0;
}

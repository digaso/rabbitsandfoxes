
#include "threads.h"
#include <stdlib.h>
#include "semaphore.h"
#include <limits.h>

void initializeThreadingSystem(int threadCount, InputData *worldData, struct ThreadedData *threadSystem) {
    // Allocate thread management arrays
    threadSystem->threads = malloc(sizeof(pthread_t) * threadCount);
    threadSystem->conflictPerThreads = malloc(sizeof(Conflicts *) * threadCount);
    threadSystem->threadSemaphores = malloc(sizeof(sem_t) * threadCount);
    threadSystem->precedingSemaphores = malloc(sizeof(sem_t) * threadCount);

    // Initialize thread synchronization barrier
    pthread_barrier_init(&threadSystem->barrier, NULL, threadCount);

    // Initialize each thread's conflict management and synchronization
    for (int threadIndex = 0; threadIndex < threadCount; threadIndex++) {
        // Allocate conflict storage for this thread
        threadSystem->conflictPerThreads[threadIndex] = malloc(sizeof(Conflicts));
        Conflicts *threadConflicts = threadSystem->conflictPerThreads[threadIndex];
        
        // Initialize conflict counters
        threadConflicts->aboveCount = 0;
        threadConflicts->bellowCount = 0;
        
        // Allocate conflict arrays (size based on world width)
        threadConflicts->above = malloc(sizeof(Conflict) * worldData->columns);
        threadConflicts->bellow = malloc(sizeof(Conflict) * worldData->columns);

        // Initialize semaphores for thread coordination
        sem_init(&threadSystem->threadSemaphores[threadIndex], 0, 0);
        sem_init(&threadSystem->precedingSemaphores[threadIndex], 0, 0);
    }
}

/*
 * We don't need to synchronize as each thread only accesses it's part of the memory, that's independent of the
 * rest
 */
void resetThreadConflicts(int threadIndex, struct ThreadedData *threadSystem) {
    Conflicts *threadConflicts = threadSystem->conflictPerThreads[threadIndex];

    // Reset conflict counters (arrays are reused, no need to clear contents)
    threadConflicts->aboveCount = 0;
    threadConflicts->bellowCount = 0;
}

void createAndStoreConflict(Conflicts *threadConflicts, int isAboveThread, int targetRow, int targetCol, WorldSlot *sourceSlot) {
    // Thread boundary conflict: entity wants to move to another thread's region
    // Store conflict for later resolution during synchronization phase
    
    int *conflictCount;
    Conflict *conflictArray;

    if (isAboveThread) {
        // Conflict with thread responsible for rows above current thread
        conflictCount = &threadConflicts->aboveCount;
        conflictArray = threadConflicts->above;
    } else {
        // Conflict with thread responsible for rows below current thread  
        conflictCount = &threadConflicts->bellowCount;
        conflictArray = threadConflicts->bellow;
    }

    // Create conflict record
    Conflict *newConflict = &conflictArray[*conflictCount];
    newConflict->newRow = targetRow;
    newConflict->newCol = targetCol;
    newConflict->slotContent = sourceSlot->slotContent;

    // Store entity-specific data for conflict resolution
    switch (sourceSlot->slotContent) {
        case FOX:
            newConflict->data = sourceSlot->entityInfo.foxInfo;
            break;
        case RABBIT:
            newConflict->data = sourceSlot->entityInfo.rabbitInfo;
            break;
        case EMPTY:
        case ROCK:
        default:
            newConflict->data = NULL;
            break;
    }

    (*conflictCount)++;
}

int findRowByEntityCount(int targetEntityCount, const int *cumulativeEntityCounts, int totalRows) {
    // Binary search to find the row that contains the target cumulative entity count
    // Used for optimal workload distribution across threads
    
    int lowRow = 0;
    int highRow = totalRows - 1;
    int midRow = (lowRow + highRow) / 2;

    // Binary search for the row where cumulative count <= targetEntityCount
    // and the next row's cumulative count > targetEntityCount
    while (lowRow <= highRow) {
        midRow = (lowRow + highRow) / 2;
        
        int currentCount = cumulativeEntityCounts[midRow];
        int nextCount = (midRow + 1 < totalRows) ? cumulativeEntityCounts[midRow + 1] : INT_MAX;
        
        if (currentCount <= targetEntityCount && nextCount > targetEntityCount) {
            // Found the correct row
            return midRow;
        } else if (currentCount > targetEntityCount) {
            // Target is in lower half
            highRow = midRow - 1;
        } else {
            // Target is in upper half
            lowRow = midRow + 1;
        }
    }

    return midRow; // Fallback to last computed middle
}

int validateThreadConfiguration(InputData *simulationData) {
    if (simulationData->threads > simulationData->rows) {
        fprintf(stderr, "ERROR: Thread count (%d) cannot exceed row count (%d)\n", 
                simulationData->threads, simulationData->rows);
        exit(EXIT_FAILURE);
    }

    if (simulationData->threads < 1) {
        fprintf(stderr, "ERROR: Thread count must be at least 1\n");
        exit(EXIT_FAILURE);
    }

    return 1;
}

void distributeWorkloadAcrossThreads(int threadCount, ThreadRowData *threadAssignments, InputData *worldData) {
    int totalEntities = worldData->entitiesAccumulatedPerRow[worldData->rows - 1];
    int entitiesPerThread = totalEntities / threadCount;
    int lastRowIndex = worldData->rows - 1;
    int nextThreadStartRow = 0;

    // Distribute workload by assigning row ranges to each thread
    for (int threadIndex = 0; threadIndex < threadCount; threadIndex++) {
        int remainingThreads = threadCount - threadIndex - 1;
        int startRow = nextThreadStartRow;
        int endRow;

        if (threadIndex == threadCount - 1) {
            // Last thread takes all remaining rows
            endRow = lastRowIndex;
        } else {
            // Find optimal end row based on entity distribution
            int targetCumulativeEntities = (threadIndex + 1) * entitiesPerThread;
            int optimalEndRow = findRowByEntityCount(targetCumulativeEntities, 
                                                      worldData->entitiesAccumulatedPerRow, 
                                                      worldData->rows);
            
            // Ensure enough rows remain for subsequent threads
            int rowsNeededForRemainingThreads = remainingThreads;
            int maxAllowedEndRow = lastRowIndex - rowsNeededForRemainingThreads;
            
            if (optimalEndRow > maxAllowedEndRow) {
                optimalEndRow = maxAllowedEndRow;
            }
            
            // Ensure this thread gets at least one row
            if (optimalEndRow < startRow) {
                optimalEndRow = startRow;
            }
            
            endRow = optimalEndRow;
        }

        // Assign row range to thread
        threadAssignments[threadIndex].startRow = startRow;
        threadAssignments[threadIndex].endRow = endRow;
        
        // Next thread starts after this thread's range
        nextThreadStartRow = endRow + 1;
    }
}

void destroyConflict(Conflict *conflict) {
    if (conflict != NULL) {
        free(conflict);
    }
}

void synchronizeAndResolveThreadConflicts(struct ThreadConflictData *conflictData) {
    if (conflictData->inputData->threads > 1) {

        struct ThreadedData *threadedData = conflictData->threadedData;

        if (conflictData->threadNum == 0) {

            //We only need one post as the top thread only synchronizes with the thread bellow it
            sem_post(&threadedData->threadSemaphores[conflictData->threadNum]);
            //The first thread will only sync with one thread

            //Wait for the semaphores of thread below
            sem_wait(&threadedData->threadSemaphores[conflictData->threadNum + 1]);

            Conflicts *bottomConflicts = threadedData->conflictPerThreads[conflictData->threadNum + 1];

//            printf("Thread %d called handle conflicts with thread %d\n", conflictData->threadNum,  conflictData->threadNum + 1);

            resolveThreadConflicts(conflictData, bottomConflicts->aboveCount, bottomConflicts->above);

        } else if (conflictData->threadNum > 0 && conflictData->threadNum < (conflictData->inputData->threads - 1)) {

            sem_t *our_sem = &threadedData->threadSemaphores[conflictData->threadNum];
            //Since middle threads will have to sync with 2 different threads, we
            //Increment the semaphore to 2
            sem_post(our_sem);
            sem_post(our_sem);

            int topThread = conflictData->threadNum - 1, bottThread = conflictData->threadNum + 1;

            sem_t *topSem = &threadedData->threadSemaphores[topThread],
                    *bottomSem = &threadedData->threadSemaphores[bottThread];

            int topDone = 0, botDone = 0;
            int sems_left = 2;

            //We don't have to wait for both semaphores to solve the conflicts
            //Try to unlock the semaphores and do them as soon as they are unlocked
            while (sems_left > 0) {
                if (!topDone) {
                    if (sem_trywait(topSem) == 0) {

                        //Since we are bellow the thread that is above us (Who knew?)
                        //We get the conflicts of that thread with the thread bellow it (That's us!)

                        Conflicts *topConf = threadedData->conflictPerThreads[topThread];

//                        printf("Thread %d called handle conflicts with thread %d\n", conflictData->threadNum,  topThread);
                        resolveThreadConflicts(conflictData, topConf->bellowCount, topConf->bellow);

                        sems_left--;
                        topDone = 1;
                    }
                }

                if (!botDone) {
                    //If there are 2 semaphore left, then the bottom one is still not done
                    //If there's only one, then the bottom one is done if the top isn't
                    if (sem_trywait(bottomSem) == 0) {
                        //Since we are above the thread that is bellow us (Again, who knew? :))
                        //We get the conflicts of that thread with the thread above it (That's us again!)
                        Conflicts *botConf = threadedData->conflictPerThreads[bottThread];

//                        printf("Thread %d called handle conflicts with thread %d\n", conflictData->threadNum,  bottThread);
                        resolveThreadConflicts(conflictData, botConf->aboveCount, botConf->above);

                        botDone = 1;
                        sems_left--;
                    }
                }
            }

        } else {
            //The last thread will also only sync with one thread
            sem_post(&threadedData->threadSemaphores[conflictData->threadNum]);

            int topThread = conflictData->threadNum - 1;

            sem_t *topSem = &threadedData->threadSemaphores[topThread];

            sem_wait(topSem);

            Conflicts *conflicts = threadedData->conflictPerThreads[topThread];
//            printf("Thread %d called handle conflicts with thread %d\n", conflictData->threadNum,  conflictData->threadNum - 1);

            resolveThreadConflicts(conflictData, conflicts->bellowCount, conflicts->bellow);
        }
    }
}


static void signalCompletionAndWaitForBarrier(int threadNumber, InputData *data, struct ThreadedData *threadedData) {
    if (threadNumber < data->threads - 1) {
        sem_t *our_sem = &threadedData->precedingSemaphores[threadNumber];

        sem_post(our_sem);
    }

    //Wait until all the threads are done
    //Because the last thread calculates the thread balance, we can instantly start a new generation
    pthread_barrier_wait(&threadedData->barrier);

}

static void waitForPreviousThreadCompletion(int threadNumber, InputData *data, struct ThreadedData *threadedData) {

    if (threadNumber > 0) {
        sem_t *topSem = &threadedData->precedingSemaphores[threadNumber - 1];

        sem_wait(topSem);
    }

}

void updateCumulativeEntityCounts(int threadIndex, InputData *worldData, ThreadRowData *threadAssignments,
                                   struct ThreadedData *threadSystem) {
    // Wait for previous thread to complete its cumulative calculation
    waitForPreviousThreadCompletion(threadIndex, worldData, threadSystem);

    ThreadRowData *currentThreadRows = &threadAssignments[threadIndex];
    int startRow = currentThreadRows->startRow;
    int endRow = currentThreadRows->endRow;

    // Update cumulative entity counts for this thread's assigned rows
    for (int row = startRow; row <= endRow; row++) {
        int previousRowCount = (row > 0) ? worldData->entitiesAccumulatedPerRow[row - 1] : 0;
        worldData->entitiesAccumulatedPerRow[row] = previousRowCount + worldData->entitiesPerRow[row];
    }

    // Last thread recalculates workload distribution for next generation
    if (threadIndex == worldData->threads - 1) {
        distributeWorkloadAcrossThreads(worldData->threads, threadAssignments, worldData);
    }

    // Signal completion and wait for other threads
    signalCompletionAndWaitForBarrier(threadIndex, worldData, threadSystem);
}


void synchronizeWithAdjacentThreads(int threadNumber, InputData *data, struct ThreadedData *threadedData) {

    if (data->threads < 2) return;

    sem_t *our_sem = &threadedData->threadSemaphores[threadNumber];

    if (threadNumber > 0 && threadNumber < (data->threads - 1)) {
        //If we're a middle thread, we will have to post for 2 threads
        sem_post(our_sem);
    }

//    printf("Posted to sem %d\n", threadNumber);

    sem_post(our_sem);

    int val = 0;

    sem_getvalue(our_sem, &val);

    //printf("Thread %d entered post and wait %p value: %d\n", threadNumber, our_sem, val);

    if (threadNumber == 0) {
        sem_t *botSem = &threadedData->threadSemaphores[threadNumber + 1];

        // printf("Thread %d Waiting for bot_sem %d\n", threadNumber, threadNumber + 1);

        sem_getvalue(botSem, &val);

        //printf("Sem %d %p value: %d\n", threadNumber + 1, botSem, val);
        sem_wait(botSem);

        //printf("Thread %d unlocked.\n", threadNumber);
    } else if (threadNumber > 0 && threadNumber < (data->threads - 1)) {

        sem_t *botSem = &threadedData->threadSemaphores[threadNumber + 1],
                *topSem = &threadedData->threadSemaphores[threadNumber - 1];


        //printf("Thread %d Waiting for top sem,...\n", threadNumber);
        sem_getvalue(topSem, &val);
        //printf("Thread %d Sem %d %p value: %d\n", threadNumber, threadNumber - 1, botSem, val);
        sem_wait(topSem);

        //printf("Thread %d unlocked top.\n", threadNumber);
        //printf("Thread %d Waiting for bot_sem\n", threadNumber);
        sem_getvalue(botSem, &val);
        //printf("Thread %d Sem %d %p value: %d\n", threadNumber, threadNumber + 1, botSem, val);
        sem_wait(botSem);
        //printf("Thread %d unlocked bot. (UNLOCKED)\n", threadNumber);
    } else {
        sem_t *topSem = &threadedData->threadSemaphores[threadNumber - 1];

        //printf("Thread %d Waiting for top_sem %d\n", threadNumber, threadNumber - 1);
        sem_getvalue(topSem, &val);

        //printf("Thread %d Sem %d %p value: %d\n", threadNumber, threadNumber - 1, &topSem, val);
        sem_wait(topSem);
        // printf("Unlock thread %d\n", threadNumber);
    }

}

void destroyConflictsContainer(Conflicts *conflicts) {
    if (conflicts != NULL) {
        free(conflicts->above);
        free(conflicts->bellow);
        free(conflicts);
    }
}

void destroyThreadingSystem(int threadCount, struct ThreadedData *threadSystem) {
    if (threadSystem == NULL) {
        return;
    }

    // Clean up per-thread resources
    for (int threadIndex = 0; threadIndex < threadCount; threadIndex++) {
        destroyConflictsContainer(threadSystem->conflictPerThreads[threadIndex]);
        sem_destroy(&threadSystem->threadSemaphores[threadIndex]);
        sem_destroy(&threadSystem->precedingSemaphores[threadIndex]);
    }
    
    // Clean up arrays
    free(threadSystem->conflictPerThreads);
    free(threadSystem->threadSemaphores);
    free(threadSystem->precedingSemaphores);
    free(threadSystem->threads);

    // Destroy synchronization barrier
    pthread_barrier_destroy(&threadSystem->barrier);

    free(threadSystem);
}
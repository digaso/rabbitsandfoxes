#ifndef TRABALHO_2_THREADS_H
#define TRABALHO_2_THREADS_H

#include "pthread.h"
#include "semaphore.h"
#include "rabbitsandfoxes.h"

typedef struct Conflict_ {

    int newRow, newCol;

    SlotContent slotContent;

    void *data;

} Conflict;

typedef struct Conflicts_ {

    //Conflicts with the row the bounds given
    int aboveCount;

    Conflict *above;

    //Conflicts with the row below the bounds given
    int bellowCount;

    Conflict *bellow;

} Conflicts;


struct ThreadedData {
    Conflicts **conflictPerThreads;

    pthread_t *threads;

    sem_t *threadSemaphores, *precedingSemaphores;

    pthread_barrier_t barrier;
};

struct ThreadConflictData {

    int threadNum;

    int startRow, endRow;

    InputData *inputData;

    WorldSlot *world;

    struct ThreadedData *threadedData;
};

typedef struct ThreadRowData_ {

    int startRow, endRow;

} ThreadRowData;

void initializeThreadingSystem(int threadCount, InputData *worldData, struct ThreadedData *threadSystem);

void postAndWaitForSurrounding(int threadNumber, InputData *data, struct ThreadedData *threadedData);

void createAndStoreConflict(Conflicts *threadConflicts, int isAboveThread, int targetRow, int targetCol, WorldSlot *sourceSlot);

int validateThreadConfiguration(InputData *simulationData);

void distributeWorkloadAcrossThreads(int threadCount, ThreadRowData *threadAssignments, InputData *worldData);

void synchronizeThreadAndSolveConflicts(struct ThreadConflictData *conflictData);

void updateCumulativeEntityCounts(int threadIndex, InputData *worldData, ThreadRowData *threadAssignments,
                                   struct ThreadedData *threadSystem);

void resetThreadConflicts(int threadIndex, struct ThreadedData *threadSystem);

void destroyConflict(Conflict *conflict);

void destroyThreadingSystem(int threadCount, struct ThreadedData *threadSystem);

#endif //TRABALHO_2_THREADS_H

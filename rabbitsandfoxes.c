#include "rabbitsandfoxes.h"
#include "entities.h"
#include "output.h"
#include <stdlib.h>
#include "matrix_utils.h"
#include <string.h>
#include "movements.h"
#include "threads.h"
#include <sys/time.h>

#define MAX_NAME_LENGTH 6
#define PRINT_ALL_GEN 0

struct InitialInputData {
    int threadNumber;

    InputData* simulationData;

    WorldSlot* world;

    struct ThreadedData* threadedData;

    ThreadRowData* threadRowData;

    int printOutput;
};


void displayGenerationState(FILE*, InputData*, WorldSlot*);

void executeSequentialGeneration(int genNumber, InputData* simulationData, WorldSlot* world);


static void
copyWorldRegionToBuffer(int threadNumber, InputData* simulationData, struct ThreadedData* threadedData, WorldSlot* sourceWorld,
    WorldSlot* destinationBuffer,
    int copyStartRow, int copyEndRow) {

    //    pthread_barrier_wait(&threadedData->barrier);

        //synchronizeWithAdjacentThreads(threadNumber, simulationData, threadedData);

    int rowCount = (copyEndRow - copyStartRow) + 1;

    memcpy(destinationBuffer, &sourceWorld[ PROJECT(simulationData->columns, copyStartRow, 0) ],
        (rowCount * simulationData->columns * sizeof(WorldSlot)));

    if (threadedData != NULL) {
        //wait for surrounding threads to also complete their copy to allow changes to the tray
        pthread_barrier_wait(&threadedData->barrier);
    }
}



void runSequentialSimulation(FILE* inputFile, FILE* outputFile) {

    InputData* simulationData = parseSimulationParameters(inputFile);

    simulationData->threads = 1;

    struct ThreadedData* threadedData = malloc(sizeof(struct ThreadedData));

    initializeThreadingSystem(simulationData->threads, simulationData, threadedData);

    WorldSlot* world = initializeWorldMatrix(simulationData);

    loadWorldEntities(inputFile, simulationData, world);

    if (PRINT_ALL_GEN) {
        outputFile = fopen("allgen.txt", "w");
    }

    for (int gen = 0; gen < simulationData->n_gen; gen++) {

        if (PRINT_ALL_GEN) {
            fprintf(outputFile, "Generation %d\n", gen);
            printf("Generation %d\n", gen);
            displayGenerationState(outputFile, simulationData, world);
            fprintf(outputFile, "\n");
        }

        executeSequentialGeneration(gen, simulationData, world);
    }

    printf("RESULTS:\n");

    outputSimulationResults(outputFile, simulationData, world);
    fflush(outputFile);
    deallocateWorldMatrix(simulationData, world);
}

static void executeWorkerThread(struct InitialInputData* args) {

    FILE* outputFile;

    if (args->threadNumber == 0 && args->printOutput) {
        outputFile = fopen("allgen.txt", "w");
    }

    ThreadRowData* threadRowData = args->threadRowData;

    for (int gen = 0; gen < args->simulationData->n_gen; gen++) {

        if (args->printOutput) {
            pthread_barrier_wait(&args->threadedData->barrier);

            if (args->threadNumber == 0) {
                fprintf(outputFile, "Generation %d\n", gen);
                printf("Generation %d\n", gen);
                displayGenerationState(outputFile, args->simulationData, args->world);
                fprintf(outputFile, "\n");
            }

            pthread_barrier_wait(&args->threadedData->barrier);
        }

        executeParallelGeneration(args->threadNumber, gen, args->simulationData,
            args->threadedData, args->world, threadRowData);
    }

    if (args->printOutput && args->threadNumber == 0) {
        fflush(outputFile);
    }

}

void runParallelSimulation(int threadCount, FILE* inputFile, FILE* outputFile) {

    InputData* simulationData = parseSimulationParameters(inputFile);

    simulationData->threads = threadCount;

    struct ThreadedData* threadedData = malloc(sizeof(struct ThreadedData));

    initializeThreadingSystem(simulationData->threads, simulationData, threadedData);

    WorldSlot* world = initializeWorldMatrix(simulationData);

    loadWorldEntities(inputFile, simulationData, world);

    if (!validateThreadConfiguration(simulationData)) {
        exit(1);
    }

    ThreadRowData* threadRowData = malloc(sizeof(ThreadRowData) * threadCount);

    struct InitialInputData** simulationDataList = malloc(sizeof(struct InitialInputData*) * threadCount);

    struct timeval start, end;

    gettimeofday(&start, NULL);

    distributeWorkloadAcrossThreads(threadCount, threadRowData, simulationData);

    for (int thread = 0; thread < threadCount; thread++) {

        struct InitialInputData* threadInput = malloc(sizeof(struct InitialInputData));

        threadInput->simulationData = simulationData;
        threadInput->threadNumber = thread;
        threadInput->world = world;
        threadInput->threadedData = threadedData;
        threadInput->printOutput = PRINT_ALL_GEN;
        threadInput->threadRowData = threadRowData;

        simulationDataList[ thread ] = threadInput;

        printf("Initializing thread %d \n", thread);

        pthread_create(&threadedData->threads[ thread ], NULL, (void* (*)(void*)) executeWorkerThread, threadInput);
        //        executeThread(simulationData);
    }

    for (int thread = 0; thread < simulationData->threads; thread++) {
        pthread_join(threadedData->threads[ thread ], NULL);
    }

    gettimeofday(&end, NULL);

    long seconds = (end.tv_sec - start.tv_sec);
    long micros = ((seconds * 1000000) + end.tv_usec) - (start.tv_usec);

    for (int thread = 0; thread < simulationData->threads; thread++) {
        free(simulationDataList[ thread ]);
    }

    free(simulationDataList);

    printf("RESULTS:\n");

    outputSimulationResults(outputFile, simulationData, world);
    fflush(outputFile);
    printf("Took %ld microseconds\n", micros);
    deallocateWorldMatrix(simulationData, world);
    destroyThreadingSystem(threadCount, threadedData);

}

static void processRabbitTurn(int genNumber, int threadStartRow, int threadEndRow, int currentRow, int currentCol, WorldSlot* currentSlot,
    InputData* simulationData,
    WorldSlot* world,
    struct RabbitMovements* movementOptions, Conflicts* threadConflicts) {

    RabbitInfo* rabbitInfo = currentSlot->entityInfo.rabbitInfo;

    //If there is no moves then the move is successful
    int movementResult = 1, procriated = 0;

#ifdef VERBOSE
    printf("Checking rabbit (%d, %d)\n", currentRow, currentCol);
#endif

    if (movementOptions->emptyMovements > 0) {

        int nextPosition = (genNumber + currentRow + currentCol) % movementOptions->emptyMovements;

        MoveDirection direction = movementOptions->emptyDirections[ nextPosition ];
        Move* move = getMoveDirection(direction);

        int newRow = currentRow + move->x, newCol = currentCol + move->y;

#ifdef VERBOSE
        printf("Moving rabbit (%d, %d) with direction %d (Index: %d, Possible: %d) to location %d %d age %d \n", currentRow, currentCol, direction,
            nextPosition, movementOptions->emptyMovements,
            newRow, newCol, rabbitInfo->currentGen);
#endif

        WorldSlot* realSlot = &world[ PROJECT(simulationData->columns, currentRow, currentCol) ];

        if (rabbitInfo->currentGen >= simulationData->gen_proc_rabbits) {
            //If the rabbit is old enough to procriate we need to leave a rabbit at that location

            //Initialize a new rabbit for that position
            realSlot->entityInfo.rabbitInfo = createRabbitEntity();
            realSlot->entityInfo.rabbitInfo->genUpdated = genNumber;
            rabbitInfo->genUpdated = genNumber;
            rabbitInfo->prevGen = 0;
            rabbitInfo->currentGen = 0;

            simulationData->entitiesPerRow[ currentRow ]++;

            procriated = 1;
        }
        else {
            realSlot->slotContent = EMPTY;
            realSlot->entityInfo.rabbitInfo = NULL;
        }

        if (newRow < threadStartRow || newRow > threadEndRow) {
            //Conflict, we have to access another thread's memory space, create a conflict
            //And store it in our conflict list
            createAndStoreConflict(threadConflicts, newRow < threadStartRow, newRow, newCol, currentSlot);

        }
        else {
            WorldSlot* newSlot = &world[ PROJECT(simulationData->columns, newRow, newCol) ];

            movementResult = processRabbitMovement(rabbitInfo, newSlot);

            if (movementResult == 1) {
                simulationData->entitiesPerRow[ newRow ]++;
            }
        }
    }
    else {
        //No possible movements for the rabbit
        simulationData->entitiesPerRow[ currentRow ]++;

    }

    //Increment the current after performing the move, but before the conflict resolution, so that
    //we only generate children in the next generation, but we still have the correct
    //Number to handle the conflictArray
    if (!procriated) {
        //Only increment if we did not procriate
        rabbitInfo->prevGen = rabbitInfo->currentGen;
        rabbitInfo->genUpdated = genNumber;
        rabbitInfo->currentGen++;
    }

    if (!movementResult) {
        destroyRabbitEntity(rabbitInfo);
    }
}

static void
executeRabbitGeneration(int threadNumber, int genNumber, InputData* simulationData, struct ThreadedData* threadedData,
    WorldSlot* world, WorldSlot* worldSnapshot, int threadStartRow, int threadEndRow) {

    int storagePaddingTop = threadStartRow > 0 ? 1 : 0;

#ifdef VERBOSE
    printf("End Row: %d, start row: %d, storage padding top %d\n", threadEndRow, threadStartRow, storagePaddingTop);
#endif
    int trueRowCount = (threadEndRow - threadStartRow);

    Conflicts* threadConflicts;

    if (threadedData != NULL)
        threadConflicts = threadedData->conflictPerThreads[ threadNumber ];

    //First move the rabbits

    struct RabbitMovements* movementOptions = createRabbitMovementContext();

    for (int copyRow = 0; copyRow <= trueRowCount; copyRow++) {
        int row = copyRow + threadStartRow;
        simulationData->entitiesPerRow[ row ] = 0;
    }

    for (int copyRow = 0; copyRow <= trueRowCount; copyRow++) {
        int row = copyRow + threadStartRow;

        for (int col = 0; col < simulationData->columns; col++) {

            WorldSlot* currentSlot = &worldSnapshot[ PROJECT(simulationData->columns, copyRow + storagePaddingTop, col) ];

            if (currentSlot->slotContent == RABBIT) {

                analyzeRabbitMovementOptions(copyRow + storagePaddingTop, col, simulationData, worldSnapshot,
                    movementOptions);

                processRabbitTurn(genNumber, threadStartRow, threadEndRow, row, col, currentSlot,
                    simulationData, world, movementOptions, threadConflicts);

                //Even though we get passed the struct by value, we have to free it,as there's some arrays
                //Contained in it
            }
        }
    }

    destroyRabbitMovementContext(movementOptions);

    //Initialize with the conflictArray at null because we don't want to access the memory
    //Until we know it's safe to do so
    struct ThreadConflictData conflictData = { threadNumber, threadStartRow, threadEndRow, simulationData,
                                              world, threadedData };

    synchronizeAndResolveThreadConflicts(&conflictData);
}


static void processFoxTurn(int genNumber, int threadStartRow, int threadEndRow, int currentRow, int currentCol, WorldSlot* currentSlot,
    InputData* simulationData, WorldSlot* world,
    struct FoxMovements* foxMovements, Conflicts* threadConflicts) {

    FoxInfo* foxInfo = currentSlot->entityInfo.foxInfo;

    //If there is no move, the result is positive, as no other animal should try to eat us
    int foxMovementResult = 1;

    //Since we store the row that's above, we have to compensate with the storagePadding

    //Increment the gen food so the fox dies before moving and after not finding a rabbit to eat
    foxInfo->currentGenFood++;

#ifdef VERBOSE
    printf("Checking fox %p (%d %d) food %d\n", foxInfo, currentRow, currentCol, foxInfo->currentGenFood);
#endif

    if (foxMovements->rabbitMovements <= 0) {
        if (foxInfo->currentGenFood >= simulationData->gen_food_foxes) {
            //If the fox gen food reaches the limit, kill it before it moves.
            WorldSlot* realSlot = &world[ PROJECT(simulationData->columns, currentRow, currentCol) ];

            realSlot->slotContent = EMPTY;

            realSlot->entityInfo.foxInfo = NULL;

#ifdef VERBOSE
            printf("Fox %p on %d %d Starved to death\n", foxInfo, currentRow, currentCol);
#endif

            destroyFoxEntity(foxInfo);

            return;
        }
    }

    int nextPosition;

    MoveDirection direction;
    int procriated = 0;

    //Can only breed a fox when we are capable of moving
    if ((foxMovements->emptyMovements > 0 || foxMovements->rabbitMovements > 0)) {
        WorldSlot* realSlot = &world[ PROJECT(simulationData->columns, currentRow, currentCol) ];

        if (foxInfo->currentGenProc >= simulationData->gen_proc_foxes) {
            realSlot->slotContent = FOX;

            realSlot->entityInfo.foxInfo = createFoxEntity();
            realSlot->entityInfo.foxInfo->genUpdated = genNumber;

            simulationData->entitiesPerRow[ currentRow ]++;

            foxInfo->genUpdated = genNumber;
            foxInfo->prevGenProc = foxInfo->currentGenProc;
            foxInfo->currentGenProc = 0;
            procriated = 1;
        }
        else {
            //Clear the currentSlot
            realSlot->slotContent = EMPTY;

            realSlot->entityInfo.foxInfo = NULL;
        }
    }

    if (foxMovements->rabbitMovements > 0) {
        nextPosition = (genNumber + currentRow + currentCol) % foxMovements->rabbitMovements;

        direction = foxMovements->rabbitDirections[ nextPosition ];
    }
    else if (foxMovements->emptyMovements > 0) {
        nextPosition = (genNumber + currentRow + currentCol) % foxMovements->emptyMovements;

        direction = foxMovements->emptyDirections[ nextPosition ];
    }

    if (foxMovements->rabbitMovements > 0 || foxMovements->emptyMovements > 0) {
        Move* move = getMoveDirection(direction);

        int newRow = currentRow + move->x, newCol = currentCol + move->y;

        if (newRow < threadStartRow || newRow > threadEndRow) {
            //Conflict, we have to access another thread's memory space, create a conflict
            //And store it in our conflict list
            createAndStoreConflict(threadConflicts, newRow < threadStartRow, newRow, newCol, currentSlot);
        }
        else {
            WorldSlot* newSlot = &world[ PROJECT(simulationData->columns, newRow, newCol) ];

            foxMovementResult = processFoxMovement(foxInfo, newSlot);
            //We only increment the rows under our control, to avoid concurrency issues
            if (foxMovementResult == 1) {
                simulationData->entitiesPerRow[ newRow ]++;
            }
        }
    }
    else {
        simulationData->entitiesPerRow[ currentRow ]++;
#ifdef VERBOSE
        printf("FOX at %d %d has no possible movements\n", currentRow, currentCol);
#endif
    }

    if (!procriated) {
        foxInfo->genUpdated = genNumber;
        foxInfo->prevGenProc = foxInfo->currentGenProc;
    }

    if (foxMovementResult == 1 || foxMovementResult == 2) {

        if (!procriated) {
            //Only increment the procriated when the fox did not replicate
            //(Or else it would start with 1 extra gen)
            foxInfo->currentGenProc++;
        }

        //If the fox eats a rabbit, reset it's current gen food
        if (foxMovementResult == 2) {
            foxInfo->currentGenFood = 0;
        }

    }
    else if (foxMovementResult == 0) {
        //If the move failed kill the fox
        destroyFoxEntity(foxInfo);
    }
}

static void
executeFoxGeneration(int threadNumber, int genNumber, InputData* simulationData, struct ThreadedData* threadedData,
    WorldSlot* world, WorldSlot* worldSnapshot, int threadStartRow, int threadEndRow) {

    int storagePaddingTop = threadStartRow > 0 ? 1 : 0;

    int trueRowCount = threadEndRow - threadStartRow;

    Conflicts* threadConflicts;
    if (threadedData != NULL)
        threadConflicts = threadedData->conflictPerThreads[ threadNumber ];

    struct FoxMovements* foxMovements = createFoxMovementContext();

    for (int copyRow = 0; copyRow <= trueRowCount; copyRow++) {
        for (int col = 0; col < simulationData->columns; col++) {

            int row = copyRow + threadStartRow;

            WorldSlot* currentSlot = &worldSnapshot[ PROJECT(simulationData->columns, copyRow + storagePaddingTop, col) ];

            if (currentSlot->slotContent == FOX) {

                analyzeFoxMovementOptions(copyRow + storagePaddingTop, col, simulationData,
                    worldSnapshot, foxMovements);

                processFoxTurn(genNumber, threadStartRow, threadEndRow, row, col, currentSlot,
                    simulationData, world, foxMovements, threadConflicts);

            }
        }
    }

    destroyFoxMovementContext(foxMovements);

    struct ThreadConflictData conflictData = { threadNumber, threadStartRow, threadEndRow, simulationData,
                                              world, threadedData };

    synchronizeAndResolveThreadConflicts(&conflictData);
}

void executeSequentialGeneration(int genNumber, InputData* simulationData, WorldSlot* world) {

    int threadStartRow = 0, threadEndRow = simulationData->rows - 1;

    int copyStartRow = threadStartRow, copyEndRow = threadEndRow;

    int rowCount = (copyEndRow - copyStartRow) + 1;

    /**
 * A copy of our area of the tray. This copy will not be modified
 */
    WorldSlot* worldSnapshot = malloc(sizeof(WorldSlot) * rowCount * simulationData->columns);

#ifdef VERBOSE
    printf("Doing copy of world Row: %d to %d (Initial: %d %d, %d)\n", copyStartRow, copyEndRow, threadStartRow, threadEndRow,
        simulationData->rows);
#endif

    copyWorldRegionToBuffer(0, simulationData, NULL, world, worldSnapshot, copyStartRow, copyEndRow);

#ifdef VERBOSE
    printf("Done copy on thread %d\n", threadNumber);
#endif

    executeRabbitGeneration(0, genNumber, simulationData, NULL, world, worldSnapshot, threadStartRow, threadEndRow);

    copyWorldRegionToBuffer(0, simulationData, NULL, world, worldSnapshot, copyStartRow, copyEndRow);

    executeFoxGeneration(0, genNumber, simulationData, NULL, world, worldSnapshot, threadStartRow, threadEndRow);

    free(worldSnapshot);

}

void executeParallelGeneration(int threadNumber, int genNumber,
    InputData* simulationData, struct ThreadedData* threadedData, WorldSlot* world,
    ThreadRowData* threadRowData) {
    ThreadRowData* ourData = &threadRowData[ threadNumber ];

    //printf("Thread %d has start row %d and end row %d\n", threadNumber, ourData->threadStartRow, ourData->threadEndRow);

    int threadStartRow = ourData->startRow,
        threadEndRow = ourData->endRow;

    int copyStartRow = threadStartRow > 0 ? threadStartRow - 1 : threadStartRow,
        copyEndRow = threadEndRow < (simulationData->rows - 1) ? threadEndRow + 1 : threadEndRow;

    int rowCount = ((copyEndRow - copyStartRow) + 1);

    /**
     * A copy of our area of the tray. This copy will not be modified
     */
    WorldSlot worldSnapshot[ rowCount * simulationData->columns ];

#ifdef VERBOSE
    printf("Doing copy of world Row: %d to %d (Initial: %d %d, %d)\n", copyStartRow, copyEndRow, threadStartRow, threadEndRow,
        simulationData->rows);
#endif

    copyWorldRegionToBuffer(threadNumber, simulationData, threadedData, world, worldSnapshot, copyStartRow, copyEndRow);

#ifdef VERBOSE
    printf("Done copy on thread %d\n", threadNumber);
#endif

    resetThreadConflicts(threadNumber, threadedData);

    executeRabbitGeneration(threadNumber, genNumber, simulationData, threadedData, world, worldSnapshot, threadStartRow, threadEndRow);

    pthread_barrier_wait(&threadedData->barrier);

    copyWorldRegionToBuffer(threadNumber, simulationData, threadedData, world, worldSnapshot, copyStartRow, copyEndRow);

    resetThreadConflicts(threadNumber, threadedData);

    executeFoxGeneration(threadNumber, genNumber, simulationData, threadedData, world, worldSnapshot, threadStartRow, threadEndRow);

    updateCumulativeEntityCounts(threadNumber, simulationData, threadRowData, threadedData);
}


/*
 * Handles the movement conflictArray of a thread. (each thread calls this for as many conflict lists it has (Usually 2, 1 if at the ends))
 */
void resolveThreadConflicts(struct ThreadConflictData* conflictContext, int conflictCount, Conflict* conflictArray) {
#ifdef VERBOSE
    printf("Thread %d called handle conflictArray with size %d\n", conflictContext->threadNum, conflictCount);
#endif

    WorldSlot* world = conflictContext->world;

    /*
     * Go through all the conflictArray
     */
    for (int i = 0; i < conflictCount; i++) {

        Conflict* conflict = &conflictArray[ i ];

        int row = conflict->newRow, column = conflict->newCol;

        int movementResult = -1;

        if (row < conflictContext->startRow || row > conflictContext->endRow) {
            fprintf(stderr,
                "ERROR: ATTEMPTING TO RESOLVE CONFLICT WITH ROW OUTSIDE SCOPE\n Row: %d, Start Row: %d End Row: %d\n",
                row,
                conflictContext->startRow, conflictContext->endRow);
            continue;
        }

        WorldSlot* currentEntityInSlot =
            &world[ PROJECT(conflictContext->inputData->columns, row, column) ];

        //Both entities are the same, so we have to follow the rules for eating rabbits.
        if (conflict->slotContent == RABBIT) {

            movementResult = processRabbitMovement((RabbitInfo*)conflict->data, currentEntityInSlot);

            if (movementResult == 0) {
                destroyRabbitEntity(conflict->data);
            }

        }
        else if (conflict->slotContent == FOX) {

            movementResult = processFoxMovement(conflict->data, currentEntityInSlot);

            if (movementResult == 2) {
                //This happens after the gen food has been incremented, so if we set it to 0 here
                //It should produce the desired output
                ((FoxInfo*)conflict->data)->currentGenFood = 0;
            }
            else if (movementResult == 0) {
                destroyFoxEntity(conflict->data);
            }

        }

        if (movementResult == 1) {
            conflictContext->inputData->entitiesPerRow[ row ]++;
        }
    }
}



void displayGenerationState(FILE* outputFile, InputData* simulationData, WorldSlot* world) {

    for (int col = -1; col <= simulationData->columns; col++) {
        fprintf(outputFile, "-");
    }

    fprintf(outputFile, "   ");

    for (int col = -1; col <= simulationData->columns; col++) {
        fprintf(outputFile, "-");
    }

    fprintf(outputFile, " ");

    for (int col = -1; col <= simulationData->columns; col++) {
        fprintf(outputFile, "-");
    }

    fprintf(outputFile, "\n");

    for (int row = 0; row < simulationData->rows; row++) {

        for (int i = 0; i < 3; i++) {

            if (i != 0) {
                if (i == 1)
                    fprintf(outputFile, "   ");
                else if (i == 2)
                    fprintf(outputFile, " ");
            }

            fprintf(outputFile, "|");

            for (int col = 0; col < simulationData->columns; col++) {

                WorldSlot* currentSlot = &world[ PROJECT(simulationData->columns, row, col) ];

                switch (currentSlot->slotContent) {

                case ROCK:
                    fprintf(outputFile, "*");
                    break;
                case FOX:
                    if (i == 0)
                        fprintf(outputFile, "F");
                    else if (i == 1)
                        fprintf(outputFile, "%d", currentSlot->entityInfo.foxInfo->currentGenProc);
                    else if (i == 2)
                        fprintf(outputFile, "%d", currentSlot->entityInfo.foxInfo->currentGenFood);
                    break;
                case RABBIT:
                    if (i == 1)
                        fprintf(outputFile, "%d", currentSlot->entityInfo.rabbitInfo->currentGen);
                    else
                        fprintf(outputFile, "R");
                    break;
                default:
                    fprintf(outputFile, " ");
                    break;
                }
            }

            fprintf(outputFile, "|");
        }

        // fprintf(outputFile, "%d %d", simulationData->entitiesPerRow[row], simulationData->entitiesAccumulatedPerRow[row]);

        fprintf(outputFile, "\n");
    }

    for (int col = -1; col <= simulationData->columns; col++) {
        fprintf(outputFile, "-");
    }

    fprintf(outputFile, "   ");

    for (int col = -1; col <= simulationData->columns; col++) {
        fprintf(outputFile, "-");
    }

    fprintf(outputFile, " ");

    for (int col = -1; col <= simulationData->columns; col++) {
        fprintf(outputFile, "-");
    }

    fprintf(outputFile, "\n");
}

void outputSimulationResults(FILE* outputFile, InputData* simulationData, WorldSlot* worldMatrix) {

    int totalEntities = 0;
    for (int row = 0; row < simulationData->rows; row++) {
        for (int col = 0; col < simulationData->columns; col++) {
            WorldSlot* currentSlot = &worldMatrix[ PROJECT(simulationData->columns, row, col) ];
            if (currentSlot->slotContent != EMPTY) {
                totalEntities++;
            }
        }
    }

    fprintf(outputFile, "%d %d %d %d %d %d %d\n", simulationData->gen_proc_rabbits, simulationData->gen_proc_foxes,
        simulationData->gen_food_foxes,
        0, simulationData->rows, simulationData->columns, totalEntities);

    for (int row = 0; row < simulationData->rows; row++) {
        for (int col = 0; col < simulationData->columns; col++) {

            WorldSlot* currentSlot = &worldMatrix[ PROJECT(simulationData->columns, row, col) ];

            if (currentSlot->slotContent != EMPTY) {

                switch (currentSlot->slotContent) {
                case RABBIT:
                    fprintf(outputFile, "RABBIT");
                    break;
                case FOX:
                    fprintf(outputFile, "FOX");
                    break;
                case ROCK:
                    fprintf(outputFile, "ROCK");
                    break;
                default:
                    break;
                }

                fprintf(outputFile, " %d %d\n", row, col);

            }
        }
    }

}

void deallocateWorldMatrix(InputData* simulationData, WorldSlot* worldMatrix) {
    for (int row = 0; row < simulationData->rows; row++) {
        for (int col = 0; col < simulationData->columns; col++) {
            WorldSlot* currentSlot = &worldMatrix[ PROJECT(simulationData->columns, row, col) ];
            if (currentSlot->slotContent == RABBIT) {
                destroyRabbitEntity(currentSlot->entityInfo.rabbitInfo);
            }
            else if (currentSlot->slotContent == FOX) {
                destroyFoxEntity(currentSlot->entityInfo.foxInfo);
            }

            releaseMovementDirections(currentSlot->defaultPossibleMoveDirections);

        }
    }

    free(simulationData->entitiesPerRow);
    free(simulationData->entitiesAccumulatedPerRow);

    free(simulationData);
    freeMatrix((void**)&worldMatrix);
}
#include "output.h"
#include "entities.h"
#include "matrix_utils.h"
#include "movements.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_NAME_LENGTH 6

void calculateEntityDistribution(InputData* inputData, WorldSlot* world) {
    int totalEntitiesProcessed = 0;
    int totalRocks = 0;

    for (int row = 0; row < inputData->rows; row++) {
        int entitiesInCurrentRow = 0;

        for (int col = 0; col < inputData->columns; col++) {
            WorldSlot* currentSlot = &world[PROJECT(inputData->columns, row, col)];

            struct DefaultMovements movementOptions = getDefaultPossibleMovements(row, col, inputData, world);
            currentSlot->defaultP = movementOptions.movementCount;
            currentSlot->defaultPossibleMoveDirections = movementOptions.directions;

            SlotContent slotType = currentSlot->slotContent;
            if (slotType == RABBIT || slotType == FOX) {
                totalEntitiesProcessed++;
                entitiesInCurrentRow++;
            }
            else if (slotType == ROCK) {
                totalRocks++;
            }
        }

        inputData->entitiesPerRow[row] = entitiesInCurrentRow;
        inputData->entitiesAccumulatedPerRow[row] = totalEntitiesProcessed;
    }

    inputData->rocks = totalRocks;
}

InputData* parseSimulationParameters(FILE* file) {
    InputData* simulationConfig = malloc(sizeof(InputData));
    
    if (simulationConfig == NULL) {
        return NULL;
    }

    // Parse reproduction parameters
    fscanf(file, "%d", &simulationConfig->gen_proc_rabbits);
    fscanf(file, "%d", &simulationConfig->gen_proc_foxes);
    fscanf(file, "%d", &simulationConfig->gen_food_foxes);
    
    // Parse simulation dimensions
    fscanf(file, "%d", &simulationConfig->n_gen);
    fscanf(file, "%d", &simulationConfig->rows);
    fscanf(file, "%d", &simulationConfig->columns);
    fscanf(file, "%d", &simulationConfig->initialPopulation);

    // Allocate memory for entity tracking arrays
    size_t rowArraySize = sizeof(int) * simulationConfig->rows;
    simulationConfig->entitiesAccumulatedPerRow = malloc(rowArraySize);
    simulationConfig->entitiesPerRow = malloc(rowArraySize);
    
    if (simulationConfig->entitiesAccumulatedPerRow == NULL || 
        simulationConfig->entitiesPerRow == NULL) {
        free(simulationConfig->entitiesAccumulatedPerRow);
        free(simulationConfig->entitiesPerRow);
        free(simulationConfig);
        return NULL;
    }

    return simulationConfig;
}

WorldSlot* initializeWorldMatrix(InputData* data) {
    WorldSlot* worldMatrix = (WorldSlot*)initMatrix(data->rows, data->columns, sizeof(WorldSlot));
    return worldMatrix;
}

static SlotContent parseEntityType(const char* entityName) {
    if (strcmp("ROCK", entityName) == 0) {
        return ROCK;
    }
    else if (strcmp("FOX", entityName) == 0) {
        return FOX;
    }
    else if (strcmp("RABBIT", entityName) == 0) {
        return RABBIT;
    }
    return EMPTY;
}

static void initializeEntityInfo(WorldSlot* slot) {
    switch (slot->slotContent) {
        case FOX:
            slot->entityInfo.foxInfo = initFoxInfo();
            break;
        case RABBIT:
            slot->entityInfo.rabbitInfo = initRabbitInfo();
            break;
        case ROCK:
        case EMPTY:
        default:
            break;
    }
}

void loadWorldEntities(FILE* file, InputData* simulationData, WorldSlot* world) {
    printf("Initial population: %d\n", simulationData->initialPopulation);

    for (int entityIndex = 0; entityIndex < simulationData->initialPopulation; entityIndex++) {
        char entityName[MAX_NAME_LENGTH + 1];
        int row, column;
        
        // Clear the entity name buffer
        memset(entityName, 0, sizeof(entityName));
        
        // Read entity data from file
        fscanf(file, "%s", entityName);
        fscanf(file, "%d", &row);
        fscanf(file, "%d", &column);

        // Get the target world slot
        WorldSlot* targetSlot = &world[PROJECT(simulationData->columns, row, column)];
        
        // Set entity type and initialize entity-specific information
        targetSlot->slotContent = parseEntityType(entityName);
        initializeEntityInfo(targetSlot);
    }

    // Calculate entity distribution across the world
    calculateEntityDistribution(simulationData, world);
}
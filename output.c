#include "output.h"
#include "entities.h"
#include "matrix_utils.h"
#include "movements.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_NAME_LENGTH 6

void initialRowEntityCount(InputData* inputData, WorldSlot* world) {
    int globalCounter = 0;
    int rockAmount = 0;

    for (int row = 0; row < inputData->rows; row++) {
        int thisRow = 0;

        for (int col = 0; col < inputData->columns; col++) {
            WorldSlot* worldSlot = &world[PROJECT(inputData->columns, row, col)];

            struct DefaultMovements defaultMovements = getDefaultPossibleMovements(row, col, inputData, world);
            worldSlot->defaultP = defaultMovements.movementCount;
            worldSlot->defaultPossibleMoveDirections = defaultMovements.directions;

            if (worldSlot->slotContent == RABBIT || worldSlot->slotContent == FOX) {
                globalCounter++;
                thisRow++;
            }
            else if (worldSlot->slotContent == ROCK) {
                rockAmount++;
            }
        }

        inputData->entitiesPerRow[row] = thisRow;
        inputData->entitiesAccumulatedPerRow[row] = globalCounter;
    }

    inputData->rocks = rockAmount;
}

InputData* readInputData(FILE* file) {
    InputData* inputData = malloc(sizeof(InputData));

    fscanf(file, "%d", &inputData->gen_proc_rabbits);
    fscanf(file, "%d", &inputData->gen_proc_foxes);
    fscanf(file, "%d", &inputData->gen_food_foxes);
    fscanf(file, "%d", &inputData->n_gen);
    fscanf(file, "%d", &inputData->rows);
    fscanf(file, "%d", &inputData->columns);
    fscanf(file, "%d", &inputData->initialPopulation);

    inputData->entitiesAccumulatedPerRow = malloc(sizeof(int) * (inputData->rows));
    inputData->entitiesPerRow = malloc(sizeof(int) * inputData->rows);

    return inputData;
}

WorldSlot* initWorld(InputData* data) {
    WorldSlot* worldMatrix = (WorldSlot*)initMatrix(data->rows, data->columns, sizeof(WorldSlot));
    return worldMatrix;
}

void readWorldInitialData(FILE* file, InputData* data, WorldSlot* world) {
    char entityName[MAX_NAME_LENGTH + 1] = { '\0' };
    int entityRow, entityColumn;

    printf("Initial population: %d\n", data->initialPopulation);

    for (int i = 0; i < data->initialPopulation; i++) {
        fscanf(file, "%s", entityName);
        fscanf(file, "%d", &entityRow);
        fscanf(file, "%d", &entityColumn);

        WorldSlot* worldSlot = &world[PROJECT(data->columns, entityRow, entityColumn)];

        if (strcmp("ROCK", entityName) == 0) {
            worldSlot->slotContent = ROCK;
        }
        else if (strcmp("FOX", entityName) == 0) {
            worldSlot->slotContent = FOX;
        }
        else if (strcmp("RABBIT", entityName) == 0) {
            worldSlot->slotContent = RABBIT;
        }
        else {
            worldSlot->slotContent = EMPTY;
        }

        switch (worldSlot->slotContent) {
        case ROCK:
            break;
        case FOX:
            worldSlot->entityInfo.foxInfo = initFoxInfo();
            break;
        case RABBIT:
            worldSlot->entityInfo.rabbitInfo = initRabbitInfo();
            break;
        default:
            break;
        }

        for (int j = 0; j < MAX_NAME_LENGTH + 1; j++) {
            entityName[j] = '\0';
        }
    }

    initialRowEntityCount(data, world);
}
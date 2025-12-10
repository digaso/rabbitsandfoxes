#ifndef TRABALHO_2_MOVEMENTS_H
#define TRABALHO_2_MOVEMENTS_H

#include "rabbitsandfoxes.h"

typedef enum MoveDirection_ {
    NORTH = 0,
    EAST = 1,
    SOUTH = 2,
    WEST = 3
} MoveDirection;

typedef struct Move_ {
    int x, y;
} Move;

struct DefaultMovements {
    int movementCount;

    MoveDirection *directions;
};

struct FoxMovements {
    //Movements that lead to a rabbit
    int rabbitMovements;

    MoveDirection *rabbitDirections;

    int emptyMovements;

    MoveDirection *emptyDirections;

};

struct RabbitMovements {

    int emptyMovements;

    MoveDirection *emptyDirections;

};

Move *getMoveDirection(MoveDirection direction);

struct DefaultMovements calculateValidMovements(int row, int col, InputData *worldData, WorldSlot *world);

struct FoxMovements *createFoxMovementContext();

struct RabbitMovements *createRabbitMovementContext();

void analyzeFoxMovementOptions(int row, int col, InputData *worldData, WorldSlot *world, struct FoxMovements *result);

void analyzeRabbitMovementOptions(int row, int col, InputData *worldData, WorldSlot *world, struct RabbitMovements *result);

void releaseMovementDirections(MoveDirection *directions);

void releaseDefaultMovements(struct DefaultMovements *movements);

void destroyFoxMovementContext(struct FoxMovements *context);

void destroyRabbitMovementContext(struct RabbitMovements *context);

#endif //TRABALHO_2_MOVEMENTS_H

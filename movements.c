
#include "movements.h"
#include "matrix_utils.h"
#include <stdlib.h>

#define DIRECTIONS 4

static Move *NORTH_MOVE = NULL;
static Move *EAST_MOVE = NULL;
static Move *SOUTH_MOVE = NULL;
static Move *WEST_MOVE = NULL;

static MoveDirection *defaultDirections = NULL;

static void initializeMovementVectors() {
    // Initialize directional movement vectors
    NORTH_MOVE = malloc(sizeof(Move));
    NORTH_MOVE->x = -1;  // Up (negative row)
    NORTH_MOVE->y = 0;

    EAST_MOVE = malloc(sizeof(Move));
    EAST_MOVE->x = 0;
    EAST_MOVE->y = 1;    // Right (positive column)

    SOUTH_MOVE = malloc(sizeof(Move));
    SOUTH_MOVE->x = 1;   // Down (positive row)
    SOUTH_MOVE->y = 0;

    WEST_MOVE = malloc(sizeof(Move));
    WEST_MOVE->x = 0;
    WEST_MOVE->y = -1;   // Left (negative column)
}

static void initializeAllDirectionsArray() {
    // Create array with all possible directions for memory optimization
    defaultDirections = malloc(sizeof(MoveDirection) * DIRECTIONS);
    
    defaultDirections[0] = NORTH;
    defaultDirections[1] = EAST;
    defaultDirections[2] = SOUTH;
    defaultDirections[3] = WEST;
}

Move *getMoveDirection(MoveDirection direction) {

    if (NORTH_MOVE == NULL || EAST_MOVE == NULL || SOUTH_MOVE == NULL || WEST_MOVE == NULL) {

        initializeMovementVectors();

    }

    switch (direction) {
        case NORTH: {
            return NORTH_MOVE;
        }
        case EAST: {
            return EAST_MOVE;
        }
        case SOUTH: {
            return SOUTH_MOVE;
        }
        case WEST: {
            return WEST_MOVE;
        }

        default:
            return NORTH_MOVE;
    }

}

static int isValidPosition(int row, int col, InputData *worldData) {
    return (row >= 0 && col >= 0 && row < worldData->rows && col < worldData->columns);
}

static int isMovementBlocked(int fromRow, int fromCol, MoveDirection direction, 
                             InputData *worldData, WorldSlot *world) {
    Move *moveVector = getMoveDirection(direction);
    int targetRow = fromRow + moveVector->x;
    int targetCol = fromCol + moveVector->y;
    
    // Check world boundaries
    if (!isValidPosition(targetRow, targetCol, worldData)) {
        return 1;  // Movement blocked by world boundary
    }
    
    // Check for permanent obstacles (rocks)
    WorldSlot *targetSlot = &world[PROJECT(worldData->columns, targetRow, targetCol)];
    if (targetSlot->slotContent == ROCK) {
        return 1;  // Movement blocked by rock
    }
    
    return 0;  // Movement is valid
}

struct DefaultMovements calculateValidMovements(int row, int col, InputData *worldData, WorldSlot *world) {
    int validDirections[DIRECTIONS] = {1, 1, 1, 1};  // Initially assume all directions valid
    int validMovementCount = 0;
    
    // Check each direction for validity
    for (int direction = 0; direction < DIRECTIONS; direction++) {
        if (isMovementBlocked(row, col, direction, worldData, world)) {
            validDirections[direction] = 0;
        } else {
            validMovementCount++;
        }
    }
    
    // Allocate directions array
    MoveDirection *availableDirections;
    
    if (validMovementCount == DIRECTIONS) {
        // All directions valid - use shared array for memory efficiency
        if (defaultDirections == NULL) {
            initializeAllDirectionsArray();
        }
        availableDirections = defaultDirections;
    } else {
        // Some directions blocked - create custom array
        availableDirections = malloc(sizeof(MoveDirection) * validMovementCount);
        int arrayIndex = 0;
        
        for (int direction = 0; direction < DIRECTIONS; direction++) {
            if (validDirections[direction]) {
                availableDirections[arrayIndex++] = direction;
            }
        }
    }
    
    struct DefaultMovements result = {validMovementCount, availableDirections};
    return result;
}

struct FoxMovements *createFoxMovementContext() {
    struct FoxMovements *context = malloc(sizeof(struct FoxMovements));
    
    if (context == NULL) {
        return NULL;
    }
    
    // Initialize movement counters
    context->emptyMovements = 0;
    context->rabbitMovements = 0;
    
    // Allocate direction arrays (maximum possible directions)
    context->rabbitDirections = malloc(sizeof(MoveDirection) * DIRECTIONS);
    context->emptyDirections = malloc(sizeof(MoveDirection) * DIRECTIONS);
    
    if (context->rabbitDirections == NULL || context->emptyDirections == NULL) {
        free(context->rabbitDirections);
        free(context->emptyDirections);
        free(context);
        return NULL;
    }
    
    return context;
}

struct RabbitMovements *createRabbitMovementContext() {
    struct RabbitMovements *context = malloc(sizeof(struct RabbitMovements));
    
    if (context == NULL) {
        return NULL;
    }
    
    // Initialize movement counter
    context->emptyMovements = 0;
    
    // Allocate direction array (maximum possible directions)
    context->emptyDirections = malloc(sizeof(MoveDirection) * DIRECTIONS);
    
    if (context->emptyDirections == NULL) {
        free(context);
        return NULL;
    }
    
    return context;
}

void analyzeFoxMovementOptions(int row, int col, InputData *worldData, WorldSlot *world, struct FoxMovements *result) {
    WorldSlot *currentSlot = &world[PROJECT(worldData->columns, row, col)];
    
    int preyMovements = 0, emptyMovements = 0;
    int preyDirectionsFlags[currentSlot->defaultP];
    int emptyDirectionsFlags[currentSlot->defaultP];
    
    // Analyze each possible direction
    for (int dirIndex = 0; dirIndex < currentSlot->defaultP; dirIndex++) {
        MoveDirection direction = currentSlot->defaultPossibleMoveDirections[dirIndex];
        Move *moveVector = getMoveDirection(direction);
        
        int targetRow = row + moveVector->x;
        int targetCol = col + moveVector->y;
        WorldSlot *targetSlot = &world[PROJECT(worldData->columns, targetRow, targetCol)];
        
        SlotContent targetContent = targetSlot->slotContent;
        
        if (targetContent == RABBIT) {
            // Fox can hunt prey
            preyMovements++;
            preyDirectionsFlags[dirIndex] = 1;
            emptyDirectionsFlags[dirIndex] = 0;
        } else if (targetContent == EMPTY) {
            // Fox can move to empty space
            emptyMovements++;
            emptyDirectionsFlags[dirIndex] = 1;
            preyDirectionsFlags[dirIndex] = 0;
        } else {
            // Fox cannot move here (fox or rock)
            emptyDirectionsFlags[dirIndex] = 0;
            preyDirectionsFlags[dirIndex] = 0;
        }
    }
    
    // Store results
    result->rabbitMovements = preyMovements;
    result->emptyMovements = emptyMovements;
    
    // Populate direction arrays based on priority (prey first, then empty spaces)
    if (preyMovements > 0) {
        int arrayIndex = 0;
        for (int dirIndex = 0; dirIndex < currentSlot->defaultP; dirIndex++) {
            if (preyDirectionsFlags[dirIndex]) {
                result->rabbitDirections[arrayIndex++] = currentSlot->defaultPossibleMoveDirections[dirIndex];
            }
        }
    } else if (emptyMovements > 0) {
        int arrayIndex = 0;
        for (int dirIndex = 0; dirIndex < currentSlot->defaultP; dirIndex++) {
            if (emptyDirectionsFlags[dirIndex]) {
                result->emptyDirections[arrayIndex++] = currentSlot->defaultPossibleMoveDirections[dirIndex];
            }
        }
    }
}

void analyzeRabbitMovementOptions(int row, int col, InputData *worldData, WorldSlot *world,
                                   struct RabbitMovements *result) {
    WorldSlot *currentSlot = &world[PROJECT(worldData->columns, row, col)];
    
    int safeMovements = 0;
    int safeDirectionsFlags[currentSlot->defaultP];
    
    // Analyze each possible direction for safe movement
    for (int dirIndex = 0; dirIndex < currentSlot->defaultP; dirIndex++) {
        MoveDirection direction = currentSlot->defaultPossibleMoveDirections[dirIndex];
        Move *moveVector = getMoveDirection(direction);
        
        int targetRow = row + moveVector->x;
        int targetCol = col + moveVector->y;
        WorldSlot *targetSlot = &world[PROJECT(worldData->columns, targetRow, targetCol)];
        
        if (targetSlot->slotContent == EMPTY) {
            // Rabbit can safely move to empty space
            safeMovements++;
            safeDirectionsFlags[dirIndex] = 1;
        } else {
            // Rabbit cannot move here (occupied by fox, rabbit, or rock)
            safeDirectionsFlags[dirIndex] = 0;
        }
    }
    
    // Store results
    result->emptyMovements = safeMovements;
    
    // Populate direction array with safe movement options
    if (safeMovements > 0) {
        int arrayIndex = 0;
        for (int dirIndex = 0; dirIndex < currentSlot->defaultP; dirIndex++) {
            if (safeDirectionsFlags[dirIndex]) {
                result->emptyDirections[arrayIndex++] = currentSlot->defaultPossibleMoveDirections[dirIndex];
            }
        }
    }
}

void releaseMovementDirections(MoveDirection *directions) {
    if (directions != NULL && directions != defaultDirections) {
        free(directions);
    }
}

void releaseDefaultMovements(struct DefaultMovements *movements) {
    if (movements != NULL) {
        releaseMovementDirections(movements->directions);
    }
}

void destroyFoxMovementContext(struct FoxMovements *context) {
    if (context != NULL) {
        free(context->rabbitDirections);
        free(context->emptyDirections);
        free(context);
    }
}

void destroyRabbitMovementContext(struct RabbitMovements *context) {
    if (context != NULL) {
        free(context->emptyDirections);
        free(context);
    }
}
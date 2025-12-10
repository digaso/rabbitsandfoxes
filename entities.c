#include "entities.h"
#include <stdlib.h>
#include <stdio.h>

FoxInfo* createFoxEntity(void) {
    FoxInfo* newFox = malloc(sizeof(FoxInfo));
    
    if (newFox == NULL) {
        return NULL;
    }
    
    // Initialize fox state - newborn fox
    newFox->currentGenFood = 0;     // Generations since last meal
    newFox->currentGenProc = 0;     // Generations since birth
    newFox->genUpdated = 0;         // Last generation updated
    newFox->prevGenProc = 0;        // Previous generation proc count
    
    return newFox;
}

RabbitInfo* createRabbitEntity(void) {
    RabbitInfo* newRabbit = malloc(sizeof(RabbitInfo));
    
    if (newRabbit == NULL) {
        return NULL;
    }
    
    // Initialize rabbit state - newborn rabbit
    newRabbit->currentGen = 0;      // Generations since birth
    newRabbit->genUpdated = 0;      // Last generation updated
    newRabbit->prevGen = 0;         // Previous generation count
    
    return newRabbit;
}

void destroyFoxEntity(FoxInfo* foxEntity) {
    if (foxEntity != NULL) {
        free(foxEntity);
    }
}

void destroyRabbitEntity(RabbitInfo* rabbitEntity) {
    if (rabbitEntity != NULL) {
        free(rabbitEntity);
    }
}

// Movement outcome constants
#define MOVEMENT_FAILED 0
#define MOVEMENT_SUCCESS 1  
#define MOVEMENT_KILLED_PREY 2
#define MOVEMENT_ERROR -1

static int calculateFoxAge(FoxInfo* fox, FoxInfo* otherFox) {
    if (fox->genUpdated > otherFox->genUpdated) {
        return fox->currentGenProc;
    }
    else if (fox->genUpdated < otherFox->genUpdated) {
        return fox->currentGenProc + 1;
    }
    else {
        return fox->currentGenProc;
    }
}

static int resolveFoxConflict(FoxInfo* movingFox, FoxInfo* occupyingFox) {
    int movingFoxAge = calculateFoxAge(movingFox, occupyingFox);
    int occupyingFoxAge = calculateFoxAge(occupyingFox, movingFox);
    
#ifdef VERBOSE
    printf("Fox conflict: moving fox %p vs occupying fox %p\n", movingFox, occupyingFox);
#endif
    
    if (movingFoxAge > occupyingFoxAge) {
#ifdef VERBOSE
        printf("Moving fox %p wins with age %d vs %d\n", movingFox, movingFoxAge, occupyingFoxAge);
#endif
        return MOVEMENT_SUCCESS;
    }
    else if (movingFoxAge == occupyingFoxAge) {
        // Same age - resolve by food level (higher = less hungry = stronger)
        if (movingFox->currentGenFood < occupyingFox->currentGenFood) {
#ifdef VERBOSE
            printf("Moving fox %p wins with food level %d vs %d\n", movingFox, 
                   movingFox->currentGenFood, occupyingFox->currentGenFood);
#endif
            return MOVEMENT_SUCCESS;
        }
        else {
#ifdef VERBOSE
            printf("Occupying fox %p wins with food level %d vs %d\n", occupyingFox,
                   occupyingFox->currentGenFood, movingFox->currentGenFood);
#endif
            return MOVEMENT_FAILED;
        }
    }
    else {
#ifdef VERBOSE
        printf("Occupying fox %p wins with age %d vs %d\n", occupyingFox, occupyingFoxAge, movingFoxAge);
#endif
        return MOVEMENT_FAILED;
    }
}

int processFoxMovement(FoxInfo* foxEntity, WorldSlot* targetSlot) {
    SlotContent targetType = targetSlot->slotContent;
    
    switch (targetType) {
        case FOX: {
            FoxInfo* occupyingFox = targetSlot->entityInfo.foxInfo;
            int conflictResult = resolveFoxConflict(foxEntity, occupyingFox);
            
            if (conflictResult == MOVEMENT_SUCCESS) {
                destroyFoxEntity(occupyingFox);
                targetSlot->entityInfo.foxInfo = foxEntity;
                return MOVEMENT_SUCCESS;
            }
            return MOVEMENT_FAILED;
        }
        
        case RABBIT:
#ifdef VERBOSE
            printf("Fox %p killed rabbit %p\n", foxEntity, targetSlot->entityInfo.rabbitInfo);
#endif
            destroyRabbitEntity(targetSlot->entityInfo.rabbitInfo);
            targetSlot->slotContent = FOX;
            targetSlot->entityInfo.foxInfo = foxEntity;
            return MOVEMENT_KILLED_PREY;
            
        case EMPTY:
            targetSlot->slotContent = FOX;
            targetSlot->entityInfo.foxInfo = foxEntity;
            return MOVEMENT_SUCCESS;
            
        case ROCK:
        default:
            fprintf(stderr, "ERROR: Attempted to move fox to invalid location (content: %d)\n", targetType);
            return MOVEMENT_ERROR;
    }
}

static int calculateRabbitAge(RabbitInfo* rabbit, RabbitInfo* otherRabbit) {
    if (rabbit->genUpdated > otherRabbit->genUpdated) {
        return rabbit->currentGen;
    }
    else if (rabbit->genUpdated < otherRabbit->genUpdated) {
        return rabbit->currentGen + 1;
    }
    else {
        return rabbit->currentGen;
    }
}

static int resolveRabbitConflict(RabbitInfo* movingRabbit, RabbitInfo* occupyingRabbit) {
    int movingRabbitAge = calculateRabbitAge(movingRabbit, occupyingRabbit);
    int occupyingRabbitAge = calculateRabbitAge(occupyingRabbit, movingRabbit);
    
#ifdef VERBOSE
    printf("Rabbit conflict: moving %p (age %d) vs occupying %p (age %d) - details: (%d %d %d) vs (%d %d %d)\n",
           movingRabbit, movingRabbitAge, occupyingRabbit, occupyingRabbitAge,
           movingRabbit->currentGen, movingRabbit->genUpdated, movingRabbit->prevGen,
           occupyingRabbit->currentGen, occupyingRabbit->genUpdated, occupyingRabbit->prevGen);
#endif
    
    // Older rabbit wins (survival of the fittest - experience matters)
    return (movingRabbitAge > occupyingRabbitAge) ? MOVEMENT_SUCCESS : MOVEMENT_FAILED;
}

int processRabbitMovement(RabbitInfo* rabbitEntity, WorldSlot* targetSlot) {
    SlotContent targetType = targetSlot->slotContent;
    
    switch (targetType) {
        case RABBIT: {
            RabbitInfo* occupyingRabbit = targetSlot->entityInfo.rabbitInfo;
            int conflictResult = resolveRabbitConflict(rabbitEntity, occupyingRabbit);
            
            if (conflictResult == MOVEMENT_SUCCESS) {
                destroyRabbitEntity(occupyingRabbit);
                targetSlot->entityInfo.rabbitInfo = rabbitEntity;
                return MOVEMENT_SUCCESS;
            }
            return MOVEMENT_FAILED;
        }
        
        case EMPTY:
            targetSlot->slotContent = RABBIT;
            targetSlot->entityInfo.rabbitInfo = rabbitEntity;
            return MOVEMENT_SUCCESS;
            
        case FOX:
        case ROCK:
        default:
            fprintf(stderr, "ERROR: Attempted to move rabbit to invalid location (content: %d)\n", targetType);
            return MOVEMENT_ERROR;
    }
}
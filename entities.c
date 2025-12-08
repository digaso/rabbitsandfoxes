#include "entities.h"
#include <stdlib.h>
#include <stdio.h>

FoxInfo* initFoxInfo(void) {
    FoxInfo* foxInfo = malloc(sizeof(FoxInfo));
    foxInfo->currentGenFood = 0;
    foxInfo->currentGenProc = 0;
    foxInfo->genUpdated = 0;
    foxInfo->prevGenProc = 0;
    return foxInfo;
}

RabbitInfo* initRabbitInfo(void) {
    RabbitInfo* rabbitInfo = malloc(sizeof(RabbitInfo));
    rabbitInfo->currentGen = 0;
    rabbitInfo->genUpdated = 0;
    rabbitInfo->prevGen = 0;
    return rabbitInfo;
}

void freeFoxInfo(FoxInfo* foxInfo) {
    free(foxInfo);
}

void freeRabbitInfo(RabbitInfo* rabbitInfo) {
    free(rabbitInfo);
}

int handleMoveFox(FoxInfo* foxInfo, WorldSlot* newSlot) {
    if (newSlot->slotContent == FOX) {
        int foxInfoAge, newSlotAge;

        if (foxInfo->genUpdated > newSlot->entityInfo.rabbitInfo->genUpdated) {
            foxInfoAge = foxInfo->currentGenProc;
            newSlotAge = newSlot->entityInfo.foxInfo->currentGenProc + 1;
        }
        else if (foxInfo->genUpdated < newSlot->entityInfo.rabbitInfo->genUpdated) {
            foxInfoAge = foxInfo->currentGenProc + 1;
            newSlotAge = newSlot->entityInfo.foxInfo->currentGenProc;
        }
        else {
            foxInfoAge = foxInfo->currentGenProc;
            newSlotAge = newSlot->entityInfo.rabbitInfo->currentGen;
        }

#ifdef VERBOSE
        printf("Fox conflict with fox %p, current fox in slot is %p\n", foxInfo, newSlot->entityInfo.foxInfo);
#endif

        if (foxInfoAge > newSlotAge) {
#ifdef VERBOSE
            printf("Fox jumping in %p has larger gen proc (%d vs %d)\n", foxInfo, foxInfo->currentGenProc,
                newSlot->entityInfo.foxInfo->currentGenProc);
#endif
            freeFoxInfo(newSlot->entityInfo.foxInfo);
            newSlot->entityInfo.foxInfo = foxInfo;
            return 1;
        }
        else if (foxInfoAge == newSlotAge) {
            if (foxInfo->currentGenFood >= newSlot->entityInfo.foxInfo->currentGenFood) {
#ifdef VERBOSE
                printf("Fox %p has been killed by gen food (FoxInfo: %d vs %d)\n", foxInfo,
                    foxInfo->currentGenFood, newSlot->entityInfo.foxInfo->currentGenFood);
#endif
                return 0;
            }
            else {
#ifdef VERBOSE
                printf("Fox %p has been killed by gen food (FoxInfo: %d vs %d)\n", newSlot->entityInfo.foxInfo,
                    foxInfo->currentGenFood, newSlot->entityInfo.foxInfo->currentGenFood);
#endif
                freeFoxInfo(newSlot->entityInfo.foxInfo);
                newSlot->entityInfo.foxInfo = foxInfo;
                return 1;
            }
        }
        else {
#ifdef VERBOSE
            printf("Fox already there %p has larger gen proc (%d vs %d)\n", newSlot->entityInfo.foxInfo,
                foxInfoAge, newSlotAge);
#endif
            return 0;
        }
    }
    else if (newSlot->slotContent == RABBIT) {
        newSlot->slotContent = FOX;
#ifdef VERBOSE
        printf("Fox %p Killed Rabbit %p\n", foxInfo, newSlot->entityInfo.rabbitInfo);
#endif
        freeRabbitInfo(newSlot->entityInfo.rabbitInfo);
        newSlot->entityInfo.foxInfo = foxInfo;
        return 2;
    }
    else if (newSlot->slotContent == EMPTY) {
        newSlot->slotContent = FOX;
        newSlot->entityInfo.foxInfo = foxInfo;
        return 1;
    }
    else {
        fprintf(stderr, "TRIED MOVING A FOX TO A ROCK\n");
    }
    return -1;
}

int handleMoveRabbit(RabbitInfo* rabbitInfo, WorldSlot* newSlot) {
    if (newSlot->slotContent == RABBIT) {
        int rabbitInfoAge, newSlotAge;

        if (rabbitInfo->genUpdated > newSlot->entityInfo.rabbitInfo->genUpdated) {
            rabbitInfoAge = rabbitInfo->currentGen;
            newSlotAge = newSlot->entityInfo.rabbitInfo->currentGen + 1;
        }
        else if (rabbitInfo->genUpdated < newSlot->entityInfo.rabbitInfo->genUpdated) {
            rabbitInfoAge = rabbitInfo->currentGen + 1;
            newSlotAge = newSlot->entityInfo.rabbitInfo->currentGen;
        }
        else {
            rabbitInfoAge = rabbitInfo->currentGen;
            newSlotAge = newSlot->entityInfo.rabbitInfo->currentGen;
        }

#ifdef VERBOSE
        printf("Two rabbits collided, rabbitInfo: %p vs newSlot: %p Age: %d vs %d (%d %d %d, %d %d %d)\n", rabbitInfo,
            newSlot->entityInfo.rabbitInfo,
            rabbitInfoAge, newSlotAge, rabbitInfo->currentGen, rabbitInfo->genUpdated, rabbitInfo->prevGen,
            newSlot->entityInfo.rabbitInfo->currentGen,
            newSlot->entityInfo.rabbitInfo->genUpdated,
            newSlot->entityInfo.rabbitInfo->prevGen);
#endif

        if (rabbitInfoAge > newSlotAge) {
            freeRabbitInfo(newSlot->entityInfo.rabbitInfo);
            newSlot->entityInfo.rabbitInfo = rabbitInfo;
            return 1;
        }
        else {
            return 0;
        }
    }
    else if (newSlot->slotContent == EMPTY) {
        newSlot->slotContent = RABBIT;
        newSlot->entityInfo.rabbitInfo = rabbitInfo;
        return 1;
    }
    else {
        fprintf(stdout, "TRIED MOVING RABBIT TO %d\n", newSlot->slotContent);
    }
    return -1;
}
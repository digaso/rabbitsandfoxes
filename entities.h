#ifndef ENTITIES_H
#define ENTITIES_H

#include "rabbitsandfoxes.h"

// Entity initialization and cleanup
FoxInfo* initFoxInfo(void);
RabbitInfo* initRabbitInfo(void);
void freeFoxInfo(FoxInfo* foxInfo);
void freeRabbitInfo(RabbitInfo* rabbitInfo);

// Entity movement handling
int handleMoveFox(FoxInfo* foxInfo, WorldSlot* newSlot);
int handleMoveRabbit(RabbitInfo* rabbitInfo, WorldSlot* newSlot);

#endif // ENTITIES_H
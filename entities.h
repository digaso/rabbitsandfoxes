#ifndef ENTITIES_H
#define ENTITIES_H

#include "rabbitsandfoxes.h"

// Entity initialization and cleanup
FoxInfo* createFoxEntity(void);
RabbitInfo* createRabbitEntity(void);
void destroyFoxEntity(FoxInfo* foxEntity);
void destroyRabbitEntity(RabbitInfo* rabbitEntity);

// Entity movement handling
int processFoxMovement(FoxInfo* foxEntity, WorldSlot* targetSlot);
int processRabbitMovement(RabbitInfo* rabbitEntity, WorldSlot* targetSlot);

#endif // ENTITIES_H
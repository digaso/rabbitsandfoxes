#ifndef OUTPUT_H
#define OUTPUT_H

#include "rabbitsandfoxes.h"
#include <stdio.h>

// Input functions
InputData* parseSimulationParameters(FILE* file);
WorldSlot* initializeWorldMatrix(InputData* data);
void loadWorldEntities(FILE* file, InputData* simulationData, WorldSlot* world);
void calculateEntityDistribution(InputData* inputData, WorldSlot* world);

// Output functions  
void outputSimulationResults(FILE* outputFile, InputData* simulationData, WorldSlot* worldMatrix);
void displayGenerationState(FILE* outputFile, InputData* simulationData, WorldSlot* world);

// Cleanup functions
void deallocateWorldMatrix(InputData* simulationData, WorldSlot* worldMatrix);

#endif // OUTPUT_H
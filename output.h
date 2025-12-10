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
void printResults(FILE* outputFile, InputData* inputData, WorldSlot* worldSlot);
void printPrettyAllGen(FILE* outputFile, InputData* inputData, WorldSlot* world);

// Cleanup functions
void freeWorldMatrix(InputData* data, WorldSlot* worldMatrix);

#endif // OUTPUT_H
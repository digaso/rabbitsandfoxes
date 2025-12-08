#ifndef OUTPUT_H
#define OUTPUT_H

#include "rabbitsandfoxes.h"
#include <stdio.h>

// Input functions
InputData* readInputData(FILE* file);
WorldSlot* initWorld(InputData* data);
void readWorldInitialData(FILE* file, InputData* data, WorldSlot* world);
void initialRowEntityCount(InputData* inputData, WorldSlot* world);

// Output functions  
void printResults(FILE* outputFile, InputData* inputData, WorldSlot* worldSlot);
void printPrettyAllGen(FILE* outputFile, InputData* inputData, WorldSlot* world);

// Cleanup functions
void freeWorldMatrix(InputData* data, WorldSlot* worldMatrix);

#endif // OUTPUT_H
# Rabbits and Foxes Ecosystem Simulation

**Project done by:** Diogo Alves, 202006033 and Mário Minhava, 202206190

## Project Overview

This project implements a parallel solution for simulating a **Rabbits and Foxes Ecosystem** using **multi-threading with pthreads**. The implementation leverages parallel computing to distribute the simulation workload across multiple threads, enabling efficient processing of large ecosystem grids while maintaining ecosystem dynamics and inter-entity interactions.

## Algorithm Implementation

### Ecosystem Simulation Problem

The algorithm simulates a predator-prey ecosystem where rabbits and foxes interact according to specific behavioral rules. The simulation progresses through discrete generations, with each generation consisting of:

1. **Rabbit Movement Phase**: All rabbits attempt to move to adjacent empty cells
2. **Rabbit Conflict Resolution**: Handle rabbit movements that cross thread boundaries
3. **Fox Movement Phase**: Foxes move toward prey or empty cells
4. **Fox Conflict Resolution**: Handle fox movements and finalize generation state

### Entity Behavior Rules

**Rabbit Behavior:**
- Move to adjacent empty cells (north, east, south, west)
- Cannot move diagonally or into rocks/occupied cells
- Reproduce after reaching procreation age
- Die of old age or when eaten by foxes

**Fox Behavior:**
- Prefer moving toward cells containing rabbits
- Can move to empty cells if no prey available
- Require regular feeding to survive (food age tracking)
- Reproduce after reaching procreation age
- Die from starvation or old age

**Conflict Resolution:**
When multiple entities attempt to move to the same cell, resolution follows priority rules:
- **Rabbits**: Entity closest to procreation survives
- **Foxes**: Entity closest to procreation survives; if tied, entity furthest from starvation survives

### Parallel Algorithm Approach

The parallel implementation divides the ecosystem grid by rows, with each thread responsible for a specific region. This approach provides:

1. **Dynamic Load Balancing**: Thread regions are assigned based on entity density rather than fixed grid divisions
2. **Minimal Synchronization**: Only adjacent threads need to communicate for boundary movements
3. **Conflict Management**: Cross-boundary movements are handled through structured conflict resolution

## Project Structure

The project follows a modular architecture with clear separation of concerns:

| File                     | Purpose                                                    |
| ------------------------ | ---------------------------------------------------------- |
| `main.c`                 | Program entry point, command-line argument processing     |
| `rabbitsandfoxes.h/.c`   | Core simulation logic, generation processing              |
| `entities.h/.c`          | Entity creation, destruction, and lifecycle management    |
| `movements.h/.c`         | Movement analysis and conflict detection                  |
| `threads.h/.c`           | Thread management, synchronization, and parallel control |
| `matrix_utils.h/.c`      | Grid utilities and memory management                      |
| `output.h/.c`            | Ecosystem state display and result formatting            |
| `makefile`               | Build system with compilation and comprehensive testing   |

## Architecture and Data Structures

### Core Structures

#### `WorldSlot`
Represents individual ecosystem positions:

```c
typedef struct WorldSlot_ {
    SlotContent slotContent;
    int defaultP;
    MoveDirection *defaultPossibleMoveDirections;
    union {
        FoxInfo *foxInfo;
        RabbitInfo *rabbitInfo;
    } entityInfo;
} WorldSlot;
```

**Optimization Features:**
- **Pre-computed valid movements**: Stored per position to avoid runtime checks
- **Memory-efficient storage**: Shared movement direction arrays for identical positions
- **Union type safety**: Type-safe entity information storage

#### `ThreadedData`
Complete thread coordination infrastructure:

```c
struct ThreadedData {
    Conflicts **conflictPerThreads;
    pthread_t *threads;
    sem_t *threadSemaphores, *precedingSemaphores;
    pthread_barrier_t barrier;
};
```

**Synchronization Strategy:**
- **Barrier synchronization**: Ensures all threads complete phases together
- **Semaphore coordination**: Manages conflict resolution between adjacent threads
- **Conflict buffers**: Structured storage for cross-boundary movements

## Core Functions and Algorithm Logic

### 1. Parallel Simulation Architecture

**Main Coordination Functions:**

| Function      | Responsibility |
|---------------|----------------|
| `runParallelSimulation()` | Thread creation, workload distribution, timing measurement |
| `executeParallelGeneration()` |          Single generation execution across all threads |
| `executeWorkerThread()` |  Individual thread execution loop |

### 2. Dynamic Work Distribution Strategy

**Entity-Density Load Balancing:**
The system assigns work based on entity density rather than fixed row divisions. Binary search finds optimal row boundaries targeting equal entities per thread. 

### 3. Generation Processing Pipeline

**Two-Phase Execution:**
1. **Rabbit Phase**: Threads create local snapshots, process rabbit movements, defer cross-boundary conflicts, synchronize
2. **Fox Phase**: Update snapshots, process fox movements (prioritizing prey), resolve conflicts, update entity counts

Deterministic movement uses `(generation + row + col) % possibleMoves` ensuring reproducible results across thread counts.

### 4. Entity Behavioral Models

**Rabbits**: Move to empty adjacent cells. Reproduce when reaching maturity. Age-based conflict resolution (older wins).

**Foxes**: Prioritize moves toward prey, fallback to empty cells. Increment starvation counter each turn, die if exceeding threshold. Age-based conflicts with food level tiebreaker.

### 5. Inter-Thread Coordination

**Synchronization Strategy:**
- **Global Barriers**: All threads synchronize between rabbit and fox phases
- **Local Semaphores**: Adjacent threads coordinate boundary conflict resolution
- **Conflict Buffers**: Thread-safe storage for cross-boundary movements

**Communication Pattern**: Threads only interact with immediate neighbors, reducing complexity from O(n²) to O(n).

## Technical Implementation Details

**Memory Management**: Dynamic entity allocation with union-based storage. Pre-computed movement directions cached per position. Contiguous world matrix allocation for cache efficiency.

**Thread Safety**: Territorial ownership (threads own specific rows) + ordered synchronization prevents race conditions. Barrier synchronization eliminates deadlocks. Conflict buffers isolate cross-boundary operations.

## Performance Results

### Execution Times and Speedup Analysis

[**Performance data and measurements will be added here**]

*Performance tables showing execution times for different ecosystem sizes and thread counts*

### Speedup Charts and Efficiency Plots

[**Performance plots and graphs will be inserted here**]

*Visual analysis of parallel performance including:*
- *Speedup vs Thread Count graphs*
- *Efficiency analysis charts*
- *Scalability comparison plots*

### Performance Analysis Discussion

**Optimal Performance Characteristics:**
- **Sweet Spot**: [To be determined from experimental results]
- **Diminishing Returns**: [Analysis based on measured data]
- **Scalability Threshold**: [Identified from performance measurements]

**Bottleneck Analysis:**
1. **Small Ecosystems**: Thread overhead vs computation balance
2. **Medium Ecosystems**: Communication vs computation trade-offs
3. **Large Ecosystems**: Scalability and efficiency characteristics

### Communication Overhead Analysis

The parallel algorithm minimizes communication through:
- **Localized synchronization**: Only adjacent threads communicate
- **Structured conflict resolution**: Predictable communication patterns
- **Dynamic load balancing**: Reduces idle time and workload imbalances


## Conclusion

This implementation successfully demonstrates efficient parallel simulation of complex ecosystem dynamics using pthread-based parallelization. The modular design enables flexible testing across different ecosystem sizes and thread configurations while maintaining correctness and performance.

**Future Enhancements:**
- **Hybrid Parallelization**: Combine pthreads with OpenMP for nested parallelism
- **Non-blocking Communication**: Reduce synchronization overhead through asynchronous message passing
- **NUMA Optimization**: Thread affinity and memory locality improvements for multi-socket systems
- **GPU Acceleration**: Offload movement calculations to CUDA/OpenCL for massive parallelism
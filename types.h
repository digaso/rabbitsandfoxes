#include <pthread.h>

#ifndef TYPES_H
#define TYPES_H

typedef pthread_mutex_t mutex_t;
typedef pthread_cond_t cond_t;


typedef enum { EMPTY = 0, ROCK = 1, RABBIT = 2, FOX = 3 } kind_t;


typedef struct {
  kind_t kind;
  int proc_age; /* generations since birth or last procreate */
  int food_age; /* for foxes: generations since last ate */
} cell_t;


/* Candidate intent for a target cell */
typedef struct intent_s {
  kind_t kind;
  int proc_age;
  int food_age; /* meaningful for foxes: smaller means less hungry */
  int from_r, from_c;
  int will_procreate; /* 0/1 */
  struct intent_s* next;
} intent_t;

typedef struct {
  int id;
  int r0, r1; /* inclusive range rows [r0, r1) */
} worker_arg_t;


#endif /* TYPES_H */
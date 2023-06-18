#include "fake_process.h"
#include "linked_list.h"
#pragma once

//Ho spostato qui le struct in quanto SchedPSJFArgs è utilizzata anche in fake_os.c
typedef struct {
  int quantum;
  float alpha;
  float* burst_predictions; //Previsioni di durata dei burst di CPU per ogni processo
  int* burst_effective;
} SchedPSJFArgs;

typedef struct {
  int quantum;
} SchedRRArgs;


//Simile a FakeProcess
typedef struct {
  ListItem list;
  int pid;
  ListHead events;
} FakePCB;

//All'interno vi è una funziona di scheduling che può cambiare => puntatore a funzione
struct FakeOS;
typedef void (*ScheduleFn)(struct FakeOS* os, void* args); //è questa

typedef struct FakeOS{
  FakePCB** running; // Array di PCB in esecuzione per ogni core
  int num_cores; // Numero di core
  ListHead ready;
  ListHead waiting;
  int timer;
  ScheduleFn schedule_fn; //Si scrive qui la funzione dello scheduling
  void* schedule_args;
  int has_schedule_sjf; //Parametro per verificare se lo scheduler è sjf.
  ListHead processes; //All'interno vi sono tutti i processi. Li carico tutti e poi controllo l'arrival time per crearli ecc
} FakeOS;

//Ho aggiunto un parametro per il numero di core
void FakeOS_init(FakeOS* os, int num_cores);

void FakeOS_simStep(FakeOS* os); //Incrementa di uno il timer e in base allo scheduling farà delle cose
void FakeOS_destroy(FakeOS* os);
int allRunningNull(FakeOS* os); // Funzione per verificare se tutti gli elementi di os->running sono NULL
int oneRunningNull(FakeOS* os); // Funzione per verificare se almeno un elemento di os->running è NULL
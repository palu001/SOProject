#include "fake_process.h"
#include "linked_list.h"
#pragma once

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
  FakePCB* running; //Un solo PCB
  ListHead ready;
  ListHead waiting;
  int timer;
  ScheduleFn schedule_fn; //Si scrive qui la funzione dello scheduling
  void* schedule_args;

  ListHead processes; //All'interno vi sono tutti i processi. Li carico tutti e poi controllo l'arrival time per crearli ecc
} FakeOS;

void FakeOS_init(FakeOS* os);
void FakeOS_simStep(FakeOS* os); //Incrementa di uno il timer e in base allo scheduling farà delle cose
void FakeOS_destroy(FakeOS* os);
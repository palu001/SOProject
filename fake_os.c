#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "fake_os.h"

void FakeOS_init(FakeOS* os,int num_cores) {
  //Alloco i core
  os->running = (FakePCB**)malloc(num_cores * sizeof(FakePCB*));
  for (int i = 0; i < num_cores; i++) {
    //Inizialmente nessun processo è in esecuzione
    os->running[i] = NULL;
  }
  os->num_cores = num_cores;
  List_init(&os->ready);
  List_init(&os->waiting);
  List_init(&os->processes);
  os->timer=0;
  os->schedule_fn=0;
}

//Prendo il FakeProcess e lo rendo un FakePCB. Lo metto in os
void FakeOS_createProcess(FakeOS* os, FakeProcess* p) {
  // sanity check
  assert(p->arrival_time==os->timer && "time mismatch in creation");
  // we check that in the list of PCBs there is no
  // pcb having the same pid
  // check su processi nei core
  for (int i=0;i<os->num_cores;i++){
    assert((!os->running[i] || os->running[i]->pid!=p->pid) && "pid taken");
  }
  

  ListItem* aux=os->ready.first;
  while(aux){
    FakePCB* pcb=(FakePCB*)aux;
    assert(pcb->pid!=p->pid && "pid taken");
    aux=aux->next;
  }

  aux=os->waiting.first;
  while(aux){
    FakePCB* pcb=(FakePCB*)aux;
    assert(pcb->pid!=p->pid && "pid taken");
    aux=aux->next;
  }
  
  // all fine, no such pcb exists
  FakePCB* new_pcb=(FakePCB*) malloc(sizeof(FakePCB));
  new_pcb->list.next=new_pcb->list.prev=0;
  new_pcb->pid=p->pid;
  new_pcb->events=p->events;
  //Iniziamente a nessun processo è assegnato un core
  new_pcb->core=-1;

  assert(new_pcb->events.first && "process without events");

  // depending on the type of the first event
  // we put the process either in ready or in waiting
  ProcessEvent* e=(ProcessEvent*)new_pcb->events.first;
  switch(e->type){
  case CPU:
    List_pushBack(&os->ready, (ListItem*) new_pcb);
    break;
  case IO:
    List_pushBack(&os->waiting, (ListItem*) new_pcb);
    break;
  default:
    assert(0 && "illegal resource");
    ;
  }
}




void FakeOS_simStep(FakeOS* os){
  
  printf("************** TIME: %08d **************\n", os->timer);

  //scan process waiting to be started
  //and create all processes starting now
  ListItem* aux=os->processes.first;
  while (aux){
    FakeProcess* proc=(FakeProcess*)aux;
    FakeProcess* new_process=0;
    if (proc->arrival_time==os->timer){
      new_process=proc;
    }
    aux=aux->next;
    //Se c'è un nuovo processo, questo viene creato come PCB e inserito nell'os
    if (new_process) {
      printf("\tcreate pid:%d\n", new_process->pid);
      //Viene tolto dalla lista di tutti i processi
      new_process=(FakeProcess*)List_detach(&os->processes, (ListItem*)new_process);
      FakeOS_createProcess(os, new_process);
      free(new_process);
    }
  }

  // scan waiting list, and put in ready all items whose event terminates
  aux=os->waiting.first;
  while(aux) {
    FakePCB* pcb=(FakePCB*)aux;
    aux=aux->next;
    ProcessEvent* e=(ProcessEvent*) pcb->events.first;
    //Mostro in quale core è in waiting
    printf("\twaiting pid: %d in core %d\n", pcb->pid,pcb->core);
    assert(e->type==IO);
    e->duration--; 
    printf("\t\tremaining time:%d\n",e->duration);
    if (e->duration==0){
      printf("\t\tend burst\n");
      List_popFront(&pcb->events);
      free(e);
      List_detach(&os->waiting, (ListItem*)pcb);
      if (! pcb->events.first) {
        // kill process
        printf("\t\tend process\n");
        free(pcb);
      } else {
        //handle next event
        e=(ProcessEvent*) pcb->events.first;
        switch (e->type){
        case CPU:
          printf("\t\tmove to ready\n");
          List_pushBack(&os->ready, (ListItem*) pcb);
          break;
        case IO:
          printf("\t\tmove to waiting\n");
          List_pushBack(&os->waiting, (ListItem*) pcb);
          break;
        }
      }
    }
  }

  // decrement the duration of running
  // if event over, destroy event
  // and reschedule process
  // if last event, destroy running
  // Esecuzione svolta in ogni core
  for (int i = 0; i < os->num_cores; i++) {
    FakePCB* running = os->running[i];
    printf("\trunning pid: %d in core %d\n", running?running->pid:-1,running?running->core:i);
    if (running){
      ProcessEvent* e=(ProcessEvent*) running->events.first;
      assert(e->type==CPU);
      e->duration--;
      printf("\t\tremaining time:%d\n",e->duration);
      if (e->duration==0){
        printf("\t\tend burst\n");
        List_popFront(&running->events);
        free(e);
        //In questo core non vi è più nessun processo in running
        os->running[i]=NULL;
        if (!running->events.first){
          printf("\t\tend process\n");
          //Eliminato il processo
          free(running);
        }
        else{
          e=(ProcessEvent*) running->events.first;
          switch (e->type){
            case CPU:
              printf("\t\tmove to ready\n");
              List_pushBack(&os->ready, (ListItem*) running);
              break;
            case IO:
              printf("\t\tmove to waiting\n");
              List_pushBack(&os->waiting, (ListItem*) running);
              break;
          }
        }
      }
    }
  }
  // call schedule, if defined
  //verifica che ci siano prima core disponibili e processi in ready
  if (os->schedule_fn && os->ready.first && !oneRunningNull(os) ){
    (*os->schedule_fn)(os, os->schedule_args); 
  }
  ++os->timer;
}

// Funzione per verificare se tutti gli elementi di os->running sono NULL
int allRunningNull(FakeOS* os) {
  for (int i = 0; i < os->num_cores; i++) {
    if (os->running[i] != NULL) {
      return 0;  // Non tutti gli elementi sono NULL
    }
  }
  return 1;  // Tutti gli elementi sono NULL
}
int oneRunningNull(FakeOS* os){
  for (int i = 0; i < os->num_cores; i++) {
    if (os->running[i] == NULL) {
      return 0;  // Non tutti gli elementi sono NULL
    }
  }
  return 1;  // Tutti gli elementi sono NULL
}

void FakeOS_destroy(FakeOS* os) {
}

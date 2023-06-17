#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "fake_os.h"

FakeOS os;


void schedPSJF(FakeOS* os,void* args_){
  
  SchedPSJFArgs* args=(SchedPSJFArgs*)args_;
  
  // look for the first process in ready
  // if none, return
  if (! os->ready.first)
    return;
  //controllo ogni core libero
  for (int i=0; i<os->num_cores;i++){
    if (os->running[i] == NULL && os->ready.first != NULL) {
      
      FakePCB* to_remove_best=(FakePCB*)os->ready.first;
      
      
      float pred_t_1_best=args->burst_effective[to_remove_best->pid-1]*args->alpha+args->burst_predictions[to_remove_best->pid-1]*(1-args->alpha);
      
      ListItem* aux=os->ready.first;
      
      while (aux){
        FakePCB* to_remove_i=(FakePCB*)aux;
        aux=aux->next;
        float pred=args->burst_effective[to_remove_i->pid-1]*args->alpha+args->burst_predictions[to_remove_i->pid-1]*(1-args->alpha);
        if (pred<pred_t_1_best){
          to_remove_best=to_remove_i;
          pred_t_1_best=pred;
        }
      }
      printf("Pred Scelto %d\n",to_remove_best->pid);
      args->burst_effective[to_remove_best->pid-1]=0;
      args->burst_predictions[to_remove_best->pid-1]=pred_t_1_best;
      //Modificare questa istruzione-il resto è uguale
      FakePCB* pcb=(FakePCB*) List_detach(&os->ready,(ListItem*)to_remove_best);
      pcb->core=i;
      os->running[i]=pcb;
      assert(pcb->events.first);
      ProcessEvent* e = (ProcessEvent*)pcb->events.first;
      assert(e->type==CPU);
      // A partire da un evento, ne sono generati 2 dove il primo ha durata del quanto e il secondo è durata iniziale meno quanto.
    // Così ho solo eventi che durano al massimo quanto il quanto
    // look at the first event
    // if duration>quantum
    // push front in the list of event a CPU event of duration quantum
    // alter the duration of the old event subtracting quantum
      if (e->duration>args->quantum) {
        ProcessEvent* qe=(ProcessEvent*)malloc(sizeof(ProcessEvent));
        qe->list.prev=qe->list.next=0;
        qe->type=CPU;
        qe->duration=args->quantum;
        e->duration-=args->quantum;
        List_pushFront(&pcb->events, (ListItem*)qe);
      }
    } 
  }

}

//scheduler FIFO
void schedFF(FakeOS* os,void* args_){
  // look for the first process in ready
  // if none, return
  if (! os->ready.first)
    return;
    //controllo ogni core libero
  for (int i=0; i<os->num_cores;i++){
    if (os->running[i] == NULL && os->ready.first != NULL) {
      FakePCB* pcb=(FakePCB*) List_popFront(&os->ready);
      pcb->core=i;
      os->running[i]=pcb;
      assert(pcb->events.first);
      ProcessEvent* e = (ProcessEvent*)pcb->events.first;
      assert(e->type==CPU);
    }
  }
}

//scheduler a round robin
void schedRR(FakeOS* os, void* args_){
  SchedRRArgs* args=(SchedRRArgs*)args_;

  // look for the first process in ready
  // if none, return
  if (! os->ready.first)
    return;
  
  //controllo ogni core libero
  for (int i=0; i<os->num_cores;i++){
    if (os->running[i] == NULL && os->ready.first != NULL) {
      FakePCB* pcb=(FakePCB*) List_popFront(&os->ready);
      pcb->core=i;
      os->running[i]=pcb;
      assert(pcb->events.first);
      ProcessEvent* e = (ProcessEvent*)pcb->events.first;
      assert(e->type==CPU);
      // A partire da un evento, ne sono generati 2 dove il primo ha durata del quanto e il secondo è durata iniziale meno quanto.
    // Così ho solo eventi che durano al massimo quanto il quanto
    // look at the first event
    // if duration>quantum
    // push front in the list of event a CPU event of duration quantum
    // alter the duration of the old event subtracting quantum
      if (e->duration>args->quantum) {
        ProcessEvent* qe=(ProcessEvent*)malloc(sizeof(ProcessEvent));
        qe->list.prev=qe->list.next=0;
        qe->type=CPU;
        qe->duration=args->quantum;
        e->duration-=args->quantum;
        List_pushFront(&pcb->events, (ListItem*)qe);
      }
    } 
  }
};



int main(int argc, char** argv) {
  assert(argc >= 4 && "Usage: ./program <num_cores> <quantum> <scheduler> <process_file1> <process_file2> ...");
  int num_cores = atoi(argv[1]);
  assert(num_cores > 0 && "Errore: Numero di Core deve essere maggiore di 0");
  int quantum=atoi(argv[2]);
  assert(quantum > 0 && "Errore: Quanto deve essere maggiore di 0");
  char* scheduler = argv[3];
  FakeOS_init(&os, num_cores);
  SchedRRArgs srr_args;
  SchedPSJFArgs sjf_args;
  if (strcmp(scheduler, "RR") == 0) {
    srr_args.quantum = quantum;
    os.schedule_fn = schedRR;
    os.has_schedule_sjf=0;
    os.schedule_args = &srr_args;
  }
  else if (strcmp(scheduler, "FF") == 0) {
    os.schedule_fn=schedFF;
    os.has_schedule_sjf=0;
  }
  else if (strcmp(scheduler, "SJF") == 0) {
    sjf_args.quantum = quantum;
    sjf_args.alpha=0.65;
    sjf_args.burst_predictions = calloc(argc - 4, sizeof(float));
    sjf_args.burst_effective = calloc(argc - 4, sizeof(int));
    
    os.schedule_fn=schedPSJF;
    os.has_schedule_sjf=1;
    os.schedule_args = &sjf_args;
  }
  else{
    printf("Errore: Scheduler non supportato\n");
    return -1;
  }
  
  for (int i = 4; i < argc; ++i) {
    FakeProcess new_process;
    int num_events = FakeProcess_load(&new_process, argv[i]);
    printf("loading [%s], pid: %d, events:%d\n",
           argv[i], new_process.pid, num_events);
    if (num_events) {
      FakeProcess* new_process_ptr = (FakeProcess*)malloc(sizeof(FakeProcess));
      *new_process_ptr = new_process;
      List_pushBack(&os.processes, (ListItem*)new_process_ptr);
    }
  }
  printf("num processes in queue %d\n", os.processes.size);
  //Condizione per terminare->Devo controllare ogni core in esecuzione (prima ne bastava uno)
  while (!allRunningNull(&os) || os.ready.first || os.waiting.first || os.processes.first) {
    FakeOS_simStep(&os);
  }
  free(os.running);
  if (strcmp(scheduler, "SJF") == 0) {
    free(sjf_args.burst_predictions);
    free(sjf_args.burst_effective);
  }
  os.running = NULL;
}


//
// Created by Jon Moríñigo on 7/10/21.
//

#ifndef OS_DEPEND_H
#define OS_DEPEND_H
#endif //OS_DEPEND_H


struct PCB{
    int pid;
    int ttl;
    int prioridad;
    struct PCB *next;
};
typedef struct PCB PCB;
//LA RUNQUEUE TENEMOS QUE PONER CUANTOS QUEREMOS MAX EN TODA LA RUNQUEUE

struct pcq{
    int max;
    int count;
    PCB *inicio;
    PCB *final;
};
typedef struct pcq pcq;

struct hilo{
    PCB *MyProc;
};
typedef struct hilo hilo;

struct core{
    hilo *hilos;
};
typedef struct core core;

struct CPU{
    core *core;
};
typedef struct CPU CPU;


#ifndef OS_DEPEND_H
#define OS_DEPEND_H
#endif //OS_DEPEND_H
struct mm{
    int code;
    int data;
    int *pgb;
}
typedef struct mm mm;

struct PCB{
    int pid;
    int ttl;
    int prioridad;
    int quantum;
    int quantumRestante;
    struct mm mm;
    struct PCB *next;
};
typedef struct PCB PCB;
struct PM{
    int FF; //byte array de espacios libres
    int *PM;
}
struct pcq{
    int max;
    int count;
    PCB *inicio;
    PCB *final;
};
typedef struct pcq pcq;

struct hilo{
    PCB *MyProc;
    int RI;  //registro de instruccion
    int *REG16; //array de 16 registros
    int *PTBR;
    int *TLB; //6 ENTRADAS DE TRADUCCION
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

struct TLBentry{
    int PID;
    char *virtual;
    int fisica;
};
typedef struct TLBentry TLBentry;
struct RTC{
    pcq *pQ[100];
    int *bitmap;
    int count;
};
typedef struct RTC RTC;

#ifndef OS_DEPEND_H
#define OS_DEPEND_H
#endif //OS_DEPEND_H

struct PCB{
    int pid;
    int *REG16;
    int PC;
    int prioridad;
    int quantum;
    int quantumRestante;
    int numEntradasTLB;
    struct mm *mm;
    struct PCB *next;
};
typedef struct PCB PCB;
struct PM{
    int *FF; //byte array de espacios libres
    int *M;
    int FB;  //numero de bloques libres
};
typedef struct PM PM;
struct pcq{
    int max;
    int count;
    PCB *inicio;
    PCB *final;
};
typedef struct pcq pcq;
struct TLBe{
    int PID;
    int virtual;
    int fisica;
};
typedef struct TLBe TLBe;
struct hilo{
    PCB *MyProc;
    int RI;  //registro de instruccion
    int PC;
    int *REG16; //array de 16 registros
    TLBe *PTBR;
    int indexTLB;
    TLBe *TLBA; //6 ENTRADAS DE TRADUCCION
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


struct RTC{
    pcq *pQ[100];
    int *bitmap;
    int count;
};
typedef struct RTC RTC;
struct mm{
    int code;
    int data;
    TLBe *pgb;
};
typedef struct mm mm;

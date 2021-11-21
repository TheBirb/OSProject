#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include "../Headers/depend.h"

#define MAX 100
//hay que tener un PCB en 0, para que cuando un hilo este vacío
int cf=0; //EN QUE CLOCK ESTAMOS
int pid =1;  //pid del proceso
//estructura de la maquina
CPU *CpuList;
int cpus, cores, hilos;
//estructuras de control de acceso atomico
pthread_mutex_t lock;
pthread_cond_t sigT;
pthread_cond_t broadcast;
pthread_mutex_t lock;
pthread_mutex_t clkm;
int flag=0;

//estructura de control de timers,
//El timer[0] es el numero de clocks que espera el process generator
//El timer[1] es el numero de clocks que espera el scheduler
int times[] = {3,4};
//Estructura procesos
struct PCB pcb0;
//Estructuras RTC
struct RTC realTimeClass;
int PID=1;
//cuantos procesos hay en ejecución
int nhejec=0;
//para control de timers
int done=0;


/**
 * Funcion para añadir un proceso a una cola determinada del RTC
 * @param pcb proceso a añadir
 * @param prioridad indice de la cola deseada
 * @return
 */
void *pushQ(PCB *pcb, int prioridad){
    if( realTimeClass.bitmap[prioridad]==0){
        realTimeClass.pQ[prioridad]-> inicio = pcb;
        realTimeClass.pQ[prioridad]->final = pcb;
        realTimeClass.bitmap[prioridad]=1;
    }else{
        realTimeClass.pQ[prioridad]->final->next = pcb;
    }
    realTimeClass.pQ[prioridad]->final = pcb;
    realTimeClass.pQ[prioridad]->count++;
    realTimeClass.count++;
    return 0;
}


/**
 * Funcion para obtener el proceso de un cola determinada del RTC
 * @param prioridad indice de la cola
 * @return proceso con la prioridad seleccionada
 */
PCB *pullQ(int prioridad){
    PCB *pcb = realTimeClass.pQ[prioridad]->inicio;
    if(realTimeClass.pQ[prioridad]->count==1){
        realTimeClass.pQ[prioridad]->inicio=NULL;
        realTimeClass.pQ[prioridad]->final=NULL;
        realTimeClass.bitmap[prioridad]=0;
    }else{
        realTimeClass.pQ[prioridad]->inicio= realTimeClass.pQ[prioridad]->inicio->next;
    }
    realTimeClass.pQ[prioridad]->count--;
    realTimeClass.count--;
    return(pcb);
}


/**
 * Programa para simular el clock de la maquina
 * @return
 */
void *clk() {
    int i, j, k;
    while (1) {
        pthread_mutex_lock(&clkm);
        cf++;
        printf("Clock %d\n", cf);
        while(done<2){
            pthread_cond_wait(&sigT, &clkm);
        }
        LOCK:
        while(flag==1){}
        if(flag==0){
            flag=1;
        }else{
            goto LOCK;
        }
        for (i = 0; i < cpus; i++) {
            for (j = 0; j < cores; j++) {
                for (k = 0; k < hilos; k++) {
                    if (CpuList[i].core[j].hilos[k].MyProc->pid != 0) {
                        if (CpuList[i].core[j].hilos[k].MyProc->ttl == 0) {
                            CpuList[i].core[j].hilos[k].MyProc->pid = 0;
                            nhejec--;
                        } else {
                            if(CpuList[i].core[j].hilos[k].MyProc->quantumRestante==0){
                                pushQ(CpuList[i].core[j].hilos[k].MyProc, CpuList[i].core[j].hilos[k].MyProc->prioridad);
                                CpuList[i].core[j].hilos[k].MyProc->pid = 0;
                                nhejec--;
                            }else{
                                CpuList[i].core[j].hilos[k].MyProc->quantumRestante--;
                            }
                            CpuList[i].core[j].hilos[k].MyProc->ttl--;
                        }
                    }
                }
            }
        }
        flag=0;
        done=0;
        sleep(1);
        printf("N procesos: %d\n", nhejec);
        pthread_cond_broadcast(&broadcast);
        pthread_mutex_unlock(&clkm);
    }
}


/**
 * Generador de procesos
 */
void processGen(){
    PCB *npcb = (PCB*)malloc(sizeof(struct PCB));
    npcb->pid = PID;
    PID++;
    npcb->ttl=rand() % 300;
    int prio =rand() % 99;
    npcb->prioridad= prio;
    npcb->quantum=10;
    if(realTimeClass.pQ[npcb->prioridad]->count<MAX){
        pushQ( npcb, npcb->prioridad);
    }else{
        PID--;
    }
}


/**
 * Timer del generador de procesos
 * @return
 */
void *timer1(){
    int tiempo= times[0];
    int d1=0; //contador de ciclos
    pthread_mutex_lock(&clkm);
    while(1){
        done++;
        d1++;
        if(d1==tiempo){
            LOCK:
            while(flag==1){}
            if(flag==0){
                flag=1;
            }else{
                goto LOCK;
            }
            processGen();
            flag=0;
            d1=0;
        }
        pthread_cond_signal(&sigT);
        pthread_cond_wait(&broadcast, &clkm);
    }
}


/**
 * Scheduler con política RTC
 * @return
 */
void *scheduler(){
    int i,j,k,l;
    int index=0;
    if(realTimeClass.count>0){
        for(i=0; i<cpus; i++){
            for(j=0; j<cores; j++){
                for(k=0; k<cores; k++){
                    if(CpuList[i].core[j].hilos[k].MyProc->pid==0){
                        PCB *process=NULL;
                        for (l = index; l < 100; ++l) {
                            if (realTimeClass.bitmap[l] == 1) {
                                process = pullQ(l);
                                nhejec++;
                                index=l;
                                break;
                            }
                        }
                        if (process != NULL) {
                            process->quantumRestante=process->quantum;
                            CpuList[i].core[j].hilos[k].MyProc=process;
                        }
                    }
                }
            }
        }
    }
    return 0;
}


/**
 * Timer del scheduler
 * @return
 */
void *timer2(){
    int tiempo= times[1];
    int d2=0; //contador de ciclos
    pthread_mutex_lock(&clkm);
    while(1){
        done++;
        d2++;
        if(d2==tiempo){
            LOCK:
            while(flag==1){}
            if(flag==0){
                flag=1;
            }else{
                goto LOCK;
            }
            scheduler();
            flag=0;
            d2=0;
        }
        pthread_cond_signal(&sigT);
        pthread_cond_wait(&broadcast, &clkm);
    }
}


int main() {
    int i, j, k;
    pcb0.pid = 0;
    printf("Cuantos CPU\n");
    scanf("%d", &cpus);
    printf("Cuantos Cores por CPU: \n");
    scanf("%d", &cores);
    printf("Cuantas hilos por Core: \n");
    scanf("%d", &hilos);
    //INICIALIZACIÓN DE LA ESTRUCTURA DE LA MAQUINA
    CpuList = (CPU *) malloc(cpus * sizeof(CPU));
    for (i = 0; i < cpus; i++) {
        struct CPU cpu;
        CpuList[i] = cpu;
        CpuList[i].core = (core *) malloc(cores * sizeof(core));
        for (j = 0; j < cores; j++) {
            struct core core;
            CpuList[i].core[j] = core;
            CpuList[i].core[j].hilos = (hilo *) malloc(hilos * sizeof(hilo));
            for (k = 0; k < hilos; k++) {
                struct hilo hilo;
                hilo.MyProc = &pcb0;
                CpuList[i].core[j].hilos[k] = hilo;
            }
        }
    }
    //COLA DE RTC
    realTimeClass.bitmap=(int*)malloc(100*sizeof(int));
    for (int l = 0; l < MAX; ++l) {
        struct pcq *procQ = malloc(sizeof(struct pcq));
        procQ->count = 0;
        procQ->max = 100;
        realTimeClass.bitmap[l] = 0;
        realTimeClass.pQ[l] = procQ;
    }
    realTimeClass.count = 0;
    //INICIALIZACIÓN DE ESTRUCTURAS DE CONTROL
    pthread_cond_init(&sigT, NULL);
    pthread_cond_init(&broadcast, NULL);
    pthread_mutex_init(&clkm,NULL);
    pthread_mutex_init(&lock,NULL);
    //INICIALIZACIÓN DE THREADS
    pthread_t tid[5];
    pthread_create(&tid[0], NULL, clk, NULL);
    pthread_create(&tid[1], NULL, timer1,NULL);
    pthread_create(&tid[2], NULL, timer2,NULL);
    pthread_join(tid[0],NULL);
    pthread_join(tid[1],NULL);
    pthread_join(tid[2],NULL);
}

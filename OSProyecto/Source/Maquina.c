//
// Created by Jon Moríñigo on 1/10/21.
//
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include "../Headers/depend.h"
#include <semaphore.h>


//hay que tener un PCB en 0, para que cuando un hilo este vacío
int cf=0;
int pid =1;
CPU *CpuList;
pcq *pcqxCPU;
pthread_mutex_t lock;
pthread_cond_t condP;
pthread_cond_t condS;
pthread_cond_t sigT;
pthread_mutex_t lock;
sem_t semQ;
int times[] = {2,3,4};
int cpus, cores, hilos;
struct PCB pcb0;
struct pcq newProcesses;
int PID=1;
int nhejec=0;
//------METODOS DE LA COLA---------
void pushQ(pcq *cola, PCB *pcb){
    if(cola->count==0){
        cola -> inicio = pcb;
        cola->final = pcb;
    }else{
        cola->final->next = pcb;
    }
    cola->final = pcb;
    cola->count++;
}
PCB *pullQ(pcq *cola){
    PCB *pcb = cola->inicio;
    if(cola->count==1){
        cola->inicio=NULL;
        cola->final=NULL;
    }else{
        cola->inicio= cola->inicio->next;
    }
    cola->count--;
    return(pcb);
}
PCB *atomicQueue(int type, pcq *cola, PCB *pcb){
    sem_wait(&semQ);
    pthread_mutex_lock(&lock);
    PCB *npcb;
    if(type==1){
        pushQ(cola, pcb);
    }else{
        npcb = pullQ(cola);
    }
    pthread_mutex_unlock(&lock);
    sem_post(&semQ);
    return(npcb);
}
//---------------------------------------
void *clk(){
    int i,j,k;
    while(1){
        printf("%d\n",newProcesses.count);
        cf++;
        sleep(1);
        printf("Clock %d\n",cf);
        pthread_cond_signal(&sigT);
        for(i=0; i<cpus; i++){
            for(j=0; j<cores; j++){
                for(k=0; k<cores; k++){
                    if(CpuList[i].core[j].hilos[k].MyProc->pid!=0){
                        if(CpuList[i].core[j].hilos[k].MyProc->ttl==0){
                            nhejec--;
                            CpuList[i].core[j].hilos[k].MyProc->pid=0;
                        }else{
                            CpuList[i].core[j].hilos[k].MyProc->ttl--;
                        }
                    }
                }
            }
        }
        printf("%d\n",nhejec);
    }
}
void *timer(int index){
    //index=0 => processGen
    //index=1 => scheduler
    int tiempo= times[index];
    int d1=0;
    int d2=0;
    if(index==0){
        while(1){
            pthread_cond_wait(&sigT, &lock);
            d1++;
            if(d1==tiempo){
                pthread_cond_signal(&condP);
                d1=0;
            }
        }
    }else{
        while(1){
            pthread_cond_wait(&sigT, &lock);
            d2++;
            if(d2==tiempo){
                pthread_cond_signal(&condS);
                d2=0;
            }
        }
    }


}
void *processGen(){
    while(1){
        pthread_cond_wait(&condP, &lock);
        if(newProcesses.max > newProcesses.count){
            PCB *npcb = malloc(sizeof(struct PCB));
            npcb->pid = PID;
            PID++;
            npcb->ttl=rand() % 2;
            npcb->prioridad= rand() % 99;
            atomicQueue(1, &newProcesses, npcb);
        }
    }
}
void *scheduler(){
    int i,j,k;
    while(1){
        pthread_cond_wait(&condS, &lock);
        if(newProcesses.count>0){
            for(i=0; i<cpus; i++){
                for(j=0; j<cores; j++){
                    for(k=0; k<cores; k++){
                        if(CpuList[i].core[j].hilos[k].MyProc->pid==0){
                            if(newProcesses.count!=0){
                                PCB *process = atomicQueue(2,&newProcesses,NULL);
                                if (process != NULL) {
                                    CpuList[i].core[j].hilos[k].MyProc=process;
                                    nhejec++;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

int main(){
    int i,j,k;
    pcb0.pid=0;
    printf("Cuantos CPU\n");
    scanf("%d", &cpus);
    printf("Cuantos Cores por CPU: \n");
    scanf("%d", &cores);
    printf("Cuantas hilos por Core: \n");
    scanf("%d", &hilos);
    CpuList = (CPU*) malloc(cpus * sizeof(CPU));
    for(i=0; i<cpus; i++){
        struct CPU cpu;
        CpuList[i]=cpu;
        CpuList[i].core= (core*) malloc(cores*sizeof(core));
        for(j=0; j<cores;j++) {
            struct core core;
            CpuList[i].core[j] = core;
            CpuList[i].core[j].hilos = (hilo*) malloc(hilos * sizeof(hilo));
            for (k = 0; k < hilos; k++) {
                struct hilo hilo;
                hilo.MyProc=&pcb0;
                CpuList[i].core[j].hilos[k] = hilo;

            }
        }
    }
    //COLA DE NEW
    newProcesses.max=100;

    //THREADS
    pthread_t tid[5];
    pthread_create(&tid[0], NULL, clk, NULL);
    pthread_create(&tid[1], NULL,processGen, NULL);
    pthread_create(&tid[2], NULL,scheduler, NULL);
    pthread_create(&tid[3], NULL, timer(0), NULL);
    pthread_create(&tid[4], NULL, timer(1), NULL);
    //ESPERAMOS A QUE TERMINEN
    pthread_join(tid[0],NULL);
    pthread_join(tid[1],NULL);
    pthread_join(tid[2],NULL);
    pthread_join(tid[3],NULL);
    pthread_join(tid[4],NULL);
    return 0;
}



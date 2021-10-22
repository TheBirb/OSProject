//
// Created by Jon Moríñigo on 1/10/21.
//
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include "../Headers/depend.h"



//hay que tener un PCB en 0, para que cuando un hilo este vacío
int cf=0;
int pid =1;
CPU *CpuList;
pcq *pcqxCPU;
pthread_mutex_t lock;
pthread_cond_t condP;
pthread_cond_t condS;
pthread_cond_t condD;
pthread_cond_t sigT;
int times[] = {2,3,4};
int cpus, cores, hilos;
struct PCB pcb0;
struct pcq processQ;
void *clk(){
    while(1){
        cf++;
        sleep(1);
        printf("Clock %d\n",cf);
        pthread_cond_signal(&sigT);
    }
}
void *timer(int index){
    //index=0 => processGen
    //index=1 => sheduler
    //index=2 => dispatcher
    int tiempo= times[index];
    int d1=0;
    int d2=0;
    int d3=0;
    if(index==0){
        while(1){
            pthread_cond_wait(&sigT, &lock);
            d1++;
            if(d1==tiempo){
                pthread_cond_signal(&condP);
                d1=0;
            }
        }
    }else if(index==1){
        while(1){
            pthread_cond_wait(&sigT, &lock);
            d2++;
            if(d2==tiempo){
                pthread_cond_signal(&condS);
                d2=0;
            }
        }
    }else{
        while(1){
            pthread_cond_wait(&sigT, &lock);
            d3++;
            if(d3==tiempo){
                pthread_cond_signal(&condD);
                d3=0;
            }
        }
    }


}
void *processGen(){
    int ttl;
    while(1){
        pthread_cond_wait(&condP, &lock);
        printf("TrabajandoGenProc, %d\n",cf);

    }
}
void *scheduler(){
    while(1){
        pthread_cond_wait(&condS, &lock);
        printf("TrabajandoScheduler, %d\n", cf);
        sleep(2);
    }
}
void *dispatcher(){
    int i,j,k;
    while(1){
        pthread_cond_wait(&condD, &lock);
        //miramos cola de ready
        printf("TrabajandoDispatcher, %d\n", cf);
        for(i=0; i<cpus; i++){
            for(j=0; j<cores; j++){
                for(k=0; k<cores; k++){
                   if(CpuList[i].core[j].hilos[k].MyProc->pid==0){
                       //vacio procesamos lista de ready
                   }else{
                       if(CpuList[i].core[j].hilos[k].MyProc->ttl==0){
                           CpuList[i].core[j].hilos[k].MyProc->pid=0;
                       }else{
                           CpuList[i].core[j].hilos[k].MyProc->ttl--;
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

    pthread_t tid[6];
    pthread_create(&tid[0], NULL, clk, NULL);
    pthread_create(&tid[1], NULL,processGen, NULL);
    pthread_create(&tid[2], NULL,scheduler, NULL);
    pthread_create(&tid[3], NULL,dispatcher, NULL);
    pthread_create(&tid[4], NULL, timer(0), NULL);
    pthread_create(&tid[5], NULL, timer(1), NULL);
    pthread_join(tid[0],NULL);
    pthread_join(tid[1],NULL);
    pthread_join(tid[2],NULL);
    pthread_join(tid[3],NULL);
    pthread_join(tid[4],NULL);
    pthread_join(tid[5],NULL);
    return 0;
}



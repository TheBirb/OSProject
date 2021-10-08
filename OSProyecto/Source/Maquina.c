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
CPU *CpuList;

pcq *pcqxCPU;
void clk(int cf){
    while(1){
        cf++;
        sleep(2);
    }
}
void processgenerator(){

}
void scheduler(){

}
int main(){
    int cpus, cores, hilos;
    int i,j,k;
    scanf("Cuantas CPU: %d", cpus);
    scanf("Cuantos Cores por CPU: %d", cores);
    scanf("Cuantas hilos por Core: %d", hilos);
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
                CpuList[i].core[j].hilos[k] = hilo;
            }
        }
    }

    pthread_t tid[3];
    pthread_create(&tid[0], NULL, clk, NULL);
    pthread_create(&tid[1], NULL, processgenerator, NULL);
    pthread_create(&tid[2], NULL, scheduler, NULL);

    //clock, time, processgen, scheduler
    pthread_join(&tid[0],NULL);
    pthread_join(&tid[1],NULL);
}



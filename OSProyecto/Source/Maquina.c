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
pthread_mutex_t lock;
pthread_cond_t condP;
pthread_cond_t condS;
pthread_cond_t sigT;
int times[] = {2,3};
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
    //index=1 => process
    int tiempo= times[index];
    int done=0;
    if(index==0){
        while(1){
            pthread_cond_wait(&sigT, &lock);
            done++;
            if(done==tiempo){
                pthread_cond_signal(&condP);
                done=0;
            }
        }
    }else{
        while(1){
            pthread_cond_wait(&sigT, &lock);
            done++;
            if(done==tiempo){
                pthread_cond_signal(&condS);
                done=0;
            }
        }
    }


}
void *processGen(){
    while(1){
        pthread_cond_wait(&condP, &lock);
        printf("TrabajandoGenProc, %d\n",cf);
        sleep(3);
    }
}
void *scheduler(){
    while(1){
        pthread_cond_wait(&condS, &lock);
        printf("TrabajandoScheduler, %d\n", cf);
        sleep(2);
    }
}
int main(){
    int cpus, cores, hilos;
    int i,j,k;
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
                CpuList[i].core[j].hilos[k] = hilo;
            }
        }
    }

    pthread_t tid[5];
    pthread_create(&tid[0], NULL, clk, NULL);
    pthread_create(&tid[1], NULL,processGen, NULL);
    pthread_create(&tid[2], NULL, scheduler, NULL);
    pthread_create(&tid[3], NULL, timer(0), NULL);
    pthread_create(&tid[4], NULL, timer(1), NULL);
    pthread_join(tid[0],NULL);
    pthread_join(tid[1],NULL);
    pthread_join(tid[2],NULL);
    pthread_join(tid[3],NULL);
    pthread_join(tid[4],NULL);
    return 0;
}



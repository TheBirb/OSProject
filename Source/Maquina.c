#include <string.h>
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include "../Headers/depend.h"
#include "../Headers/defines.h"

#define MAX 100
#define MAXMEM 1<<24
#define NUMBLOCK 1<<18
//definicion de funciones
void ___error_message(int cod, char *s);
void red();
void green();
void yellow ();
void reset();
void print_machine();
int contarLineas(char *filename);
char *getNumberFormat();
int genNewProcesses();
void scheduler();
void pushQ(PCB *pcb, int prioridad);
PCB *pullQ(int prioridad);
void loader();
void *timer1();
void *timer2();
void *clk();
//generacion prog
struct configuration_t conf;
unsigned int user_highest;
unsigned int user_space;

//hay que tener un PCB en 0, para que cuando un hilo este vacío
int cf=0; //EN QUE CLOCK ESTAMOS
int pid =1;  //pid del proceso
//estructura de la maquina
CPU *CpuList;
int cpus, cores, hilos;
//estructuras de control de acceso atomico
pthread_cond_t sigT;
pthread_cond_t broadcast;
pthread_mutex_t clkm;
pthread_mutex_t locking;
int flag=0;
//estructura de control de loader
int LPA;  //programa que esta leyendo
//estructura de control de timers,
//El timer[0] es el numero de clocks que espera el process generator
//El timer[1] es el numero de clocks que espera el scheduler
int times[] = {2,5};
//Estructuras RTC
struct RTC realTimeClass;
int PID=1;
//cuantos procesos hay en ejecución
int nhejec=0;
//para control de timers
int done=0;
//MEMORIA FISICA
struct PM PhyM;

int main(int argc, char *argv[]) {
    int i, j, k,max;
    if(argc!=4){
        exit(1);
    }
    char *p;
    long conv = strtol(argv[1], &p, 10);
    cpus=(int)conv;
    conv = strtol(argv[2], &p, 10);
    cores=(int)conv;
    conv = strtol(argv[3], &p, 10);
    hilos=(int)conv;
    //MEMORIA FISICA
    PhyM.FF = malloc(sizeof(int) * NUMBLOCK);
    PhyM.FB = NUMBLOCK;
    PhyM.M=(int*) malloc(sizeof(int)*MAXMEM);
    //INICIALIZACIÓN DE LA ESTRUCTURA DE LA MAQUINA
    LPA=0; //Inicializa el programa que se esta leyendo a 0
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
                PCB *pcb=(PCB*)malloc(sizeof(struct PCB));
                pcb->pid=0;
                struct hilo hilo;
                hilo.MyProc = pcb;
                hilo.indexTLB=0;
                hilo.REG16= (int*)malloc(sizeof(int)*16);
                hilo.TLBA= (TLBe *) malloc(sizeof(struct TLBe) * 6); //6 entradas de TLB
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
    pthread_mutex_init(&locking, NULL);
    //INICIALIZACIÓN DE THREADS
    pthread_t tid[3];
    pthread_create(&tid[0], NULL, clk, NULL);
    pthread_create(&tid[1], NULL, timer1,NULL);
    pthread_create(&tid[2], NULL, timer2,NULL);
    pthread_join(tid[0],NULL);
    pthread_join(tid[1],NULL);
    pthread_join(tid[2],NULL);
}

/**
 * Programa para simular el clock de la maquina
 * @return
 */
void *clk() {
    int i, j, k,l;
    int binary,bloque,hpid,d_fisica,op,reg1,reg2,reg3,v_address,data,data_start;
    while (1) {
        pthread_mutex_lock(&clkm);
        cf++;
        printf("Clock %d\n", cf);
        while(done<2){
            pthread_cond_wait(&sigT, &clkm);
        }
        pthread_mutex_lock( &locking );
        for (i = 0; i < cpus; i++) {
            for (j = 0; j < cores; j++) {
                for (k = 0; k < hilos; k++) {
                    if (CpuList[i].core[j].hilos[k].MyProc->pid != 0) {
                        if(CpuList[i].core[j].hilos[k].MyProc->quantumRestante==0){
                            CpuList[i].core[j].hilos[k].MyProc->quantumRestante=10;
                            CpuList[i].core[j].hilos[k].MyProc->PC= CpuList[i].core[j].hilos[k].PC;
                            CpuList[i].core[j].hilos[k].MyProc->REG16=CpuList[i].core[j].hilos[k].REG16;
                            pushQ(CpuList[i].core[j].hilos[k].MyProc, CpuList[i].core[j].hilos[k].MyProc->prioridad);
                            PCB *pcb=(PCB*)malloc(sizeof(struct PCB));
                            pcb->pid=0;
                            pcb->PC=0;
                            pcb->prioridad=0;
                            CpuList[i].core[j].hilos[k].MyProc=pcb;
                            CpuList[i].core[j].hilos[k].PC=0;
                            nhejec--;
                        }else{
                            d_fisica=-1;
                            //Mirar TLB
                            bloque = (CpuList[i].core[j].hilos[k].PC / 64) * 64;
                            hpid = CpuList[i].core[j].hilos[k].MyProc->pid;
                            for (l = 0; l <6 ; ++l) {
                                if(CpuList[i].core[j].hilos[k].TLBA[l].virtual == bloque && CpuList[i].core[j].hilos[k].TLBA[l].PID == hpid){
                                    d_fisica=CpuList[i].core[j].hilos[k].TLBA[l].fisica + CpuList[i].core[j].hilos[k].PC;
                                    break;
                                }
                            }
                            //Si no esta en la TLB mirar la PTBR
                            if(d_fisica==-1){
                                for(l=0; l<CpuList[i].core[j].hilos[k].MyProc->numEntradasTLB; ++l){
                                    //Si coincide la virtual con el bloque
                                    if(CpuList[i].core[j].hilos[k].MyProc->mm->pgb[l].virtual == bloque){
                                        int index=CpuList[i].core[j].hilos[k].indexTLB;
                                        CpuList[i].core[j].hilos[k].TLBA[index] = CpuList[i].core[j].hilos[k].MyProc->mm->pgb[l];
                                        CpuList[i].core[j].hilos[k].indexTLB++;
                                        if(CpuList[i].core[j].hilos[k].indexTLB==6){
                                            CpuList[i].core[j].hilos[k].indexTLB=0;
                                        }
                                        d_fisica=CpuList[i].core[j].hilos[k].TLBA[l].fisica + CpuList[i].core[j].hilos[k].PC;
                                        break;
                                    }
                                }
                            }
                            //Teniendo la direccion fisica obtenemos la instruccion
                            binary = PhyM.M[d_fisica];
                            //Y procesamos la instruccion
                            op = (binary >> 28) & 0x0F;
                            reg1      = (binary >> 24) & 0x0F;
                            reg2      = (binary >> 20) & 0x0F;
                            reg3      = (binary >> 16) & 0x0F;
                            v_address = binary & 0x00FFFFFF;
                            CpuList[i].core[j].hilos[k].RI=op;
                            if(op==0){//ld
                                //Miramos en la tlb la v_address
                                d_fisica=-1;
                                data_start=CpuList[i].core[j].hilos[k].MyProc->mm->data;
                                bloque = (((v_address-data_start)/4)/64)*64;
                                bloque+=data_start;
                                hpid = CpuList[i].core[j].hilos[k].MyProc->pid;
                                for (l = 0; l <6 ; ++l) {
                                    //si la virtual es el inicio del marco que necesitamos y es la traduccion del mismo programa
                                    if(CpuList[i].core[j].hilos[k].TLBA[l].virtual == bloque && CpuList[i].core[j].hilos[k].TLBA[l].PID == hpid){
                                        d_fisica=CpuList[i].core[j].hilos[k].TLBA[l].fisica + ((v_address/4)-(data_start/4));
                                        break;
                                    }
                                }
                                //Si no esta en la TLB mirar la PTBR
                                if(d_fisica==-1){
                                    for(l=0; l<CpuList[i].core[j].hilos[k].MyProc->numEntradasTLB; ++l){
                                        //Si coincide la virtual con el bloque
                                        if(CpuList[i].core[j].hilos[k].MyProc->mm->pgb[l].virtual == bloque){
                                            int index=CpuList[i].core[j].hilos[k].indexTLB;
                                            CpuList[i].core[j].hilos[k].TLBA[index] = CpuList[i].core[j].hilos[k].MyProc->mm->pgb[l];
                                            CpuList[i].core[j].hilos[k].indexTLB++;
                                            if(CpuList[i].core[j].hilos[k].indexTLB==6){
                                                CpuList[i].core[j].hilos[k].indexTLB=0;
                                            }
                                            d_fisica=CpuList[i].core[j].hilos[k].TLBA[l].fisica + ((v_address/4)-(data_start/4));
                                            break;
                                        }
                                    }
                                }
                                data = PhyM.M[d_fisica];
                                CpuList[i].core[j].hilos[k].REG16[reg1] = data;
                            }else if(op==2){//add
                                CpuList[i].core[j].hilos[k].REG16[reg1] = CpuList[i].core[j].hilos[k].REG16[reg2] + CpuList[i].core[j].hilos[k].REG16[reg3];
                            }else if(op==1){//st
                                d_fisica=-1;
                                data_start=CpuList[i].core[j].hilos[k].MyProc->mm->data;
                                bloque = (((v_address-data_start)/4)/64)*64;
                                bloque+=data_start;
                                hpid = CpuList[i].core[j].hilos[k].MyProc->pid;
                                for (l = 0; l <6 ; ++l) {
                                    //si la virtual es el inicio del marco que necesitamos y es la traduccion del mismo programa
                                    if(CpuList[i].core[j].hilos[k].TLBA[l].virtual == bloque && CpuList[i].core[j].hilos[k].TLBA[l].PID == hpid){
                                        d_fisica=CpuList[i].core[j].hilos[k].TLBA[l].fisica+((v_address/4)-(data_start/4));
                                        break;
                                    }
                                }
                                //Si no esta en la TLB mirar la PTBR
                                if(d_fisica==-1){
                                    for(l=0; l<CpuList[i].core[j].hilos[k].MyProc->numEntradasTLB; ++l){
                                        //Si coincide la virtual con el bloque
                                        if(CpuList[i].core[j].hilos[k].MyProc->mm->pgb[l].virtual == bloque){
                                            CpuList[i].core[j].hilos[k].TLBA[CpuList[i].core[j].hilos[k].indexTLB] = CpuList[i].core[j].hilos[k].MyProc->mm->pgb[l];
                                            CpuList[i].core[j].hilos[k].indexTLB++;
                                            if(CpuList[i].core[j].hilos[k].indexTLB==6){
                                                CpuList[i].core[j].hilos[k].indexTLB=0;
                                            }
                                            d_fisica=CpuList[i].core[j].hilos[k].TLBA[l].fisica + ((v_address/4)-(data_start/4));
                                            break;
                                        }
                                    }
                                }

                                PhyM.M[d_fisica] = CpuList[i].core[j].hilos[k].REG16[reg1];
                            }else{//exit
                                for(l=0; l<CpuList[i].core[j].hilos[k].MyProc->numEntradasTLB; l++){
                                    PhyM.FF[CpuList[i].core[j].hilos[k].MyProc->mm->pgb[l].fisica]=0;
                                    PhyM.FB++;
                                    PCB *pcb=(PCB*)malloc(sizeof(struct PCB));
                                    pcb->pid=0;
                                    pcb->PC=0;
                                    pcb->prioridad=0;
                                    CpuList[i].core[j].hilos[k].PC=0;
                                    CpuList[i].core[j].hilos[k].MyProc=pcb;
                                }
                            }
                        }
                        if(op!=15){
                            CpuList[i].core[j].hilos[k].PC++;
                            CpuList[i].core[j].hilos[k].MyProc->PC++;
                            CpuList[i].core[j].hilos[k].MyProc->quantumRestante--;
                        }
                    }
                }
            }
        }
        print_machine();
        pthread_mutex_unlock( &locking );
        done=0;
        sleep(1);
        pthread_cond_broadcast(&broadcast);
        pthread_mutex_unlock(&clkm);
    }
}
/**
 * Cargador de procesos
 */
void loader(){
    int i,j,code_start, contBlock, binary,sumContBlock,data_start,number_of_lines,vc_address,contPag,freeBlock,prioridad,numInstr,codeBlocks,dataLines,dataBlocks,blocks;
    double div;
    char line[80];
    char *nombreFichero;
    struct TLBe tlb;
    struct PCB *npcb;
    FILE *fd;
    //Antes de nada comprobamos que hay programas generados
    if((fd = fopen("prog000.elf", "r"))==NULL){
        genNewProcesses();
    }else {
        fclose(fd);
    }
    struct TLBe *tlbs = malloc(sizeof(struct TLBe) * NUMBLOCK );
    npcb = (PCB*)malloc(sizeof(struct PCB));
    npcb->mm = malloc(sizeof(struct mm));
    npcb->mm->pgb = tlbs;
    npcb->pid = PID;
    npcb->quantum=10;
    npcb->REG16 = (int*)malloc(sizeof(int)*16);
    npcb->PC=0;
    prioridad = rand() % 99;
    if (prioridad == 0){
        prioridad=1;
    }
    npcb->prioridad = prioridad;
    PID++;
    if(LPA==50){
        genNewProcesses();
        LPA=0;
    }
    //Nombre del fichero
    nombreFichero= getNumberFormat(LPA);
    //procesar fichero
    numInstr= contarLineas(nombreFichero);
    fd = fopen(nombreFichero, "r");
    fscanf(fd,"%s %X", line, &code_start);
    fscanf(fd,"%s %X", line, &data_start);
    npcb->mm->code=code_start;
    npcb->mm->data=data_start;
    //Bloques para codigo
    number_of_lines = (data_start - code_start) >> 2;
    div = (double)number_of_lines / (double)64;
    codeBlocks = (int)ceil(div);
    if (codeBlocks<0){
        codeBlocks=1;
    }
    //Bloques para datos
    dataLines = numInstr - number_of_lines;
    div = (double)dataLines / (double)64;
    dataBlocks = (int)ceil(div);
    if (dataBlocks<0){
        dataBlocks=1;
    }
    blocks=dataBlocks+codeBlocks;
    if(PhyM.FB>blocks&& realTimeClass.pQ[npcb->prioridad]->count<MAX){
        vc_address=0;
        freeBlock=0;
        contPag=0;
        contBlock=0;
        for(i=0; i<number_of_lines; i++) {
            fscanf(fd, "%08X", &binary);
            if (contPag == 0) {
                for (j = freeBlock; j < NUMBLOCK; j++) {
                    if (PhyM.FF[j] == 0) { //first-Fit
                        freeBlock = j;
                        tlb.PID = npcb->pid;
                        tlb.virtual = vc_address;
                        tlb.fisica = freeBlock * 64 + contPag;
                        PhyM.M[tlb.fisica] = binary;
                        npcb->mm->pgb[contBlock] = tlb;
                        break;
                    }
                }

            } else {
                for (j = freeBlock; j < NUMBLOCK; j++) {
                    if (PhyM.FF[j] == 0) { //first-Fit
                        freeBlock = j;
                        PhyM.M[freeBlock * 64 + contPag] = binary;
                        break;
                    }
                }
            }
            contPag++;
            vc_address += 4;
            if (contPag == 64) { //Si llega al final del bloque resetea
                contPag = 0;
                sumContBlock++;
                contBlock++;
                PhyM.FF[j] = 1;
            }
        }
        if(contPag!=0){ //Si no ha llegado al final lo marca como completo
            contPag=0;
            contBlock++;
            sumContBlock++;
            PhyM.FF[j]=1;
            freeBlock=0;
        }
        //Ahora cargamos los datos en memoria
        while (fscanf(fd, "%8X", &binary) != EOF) {
            if(contPag==0){
                for(j=freeBlock; j<NUMBLOCK; j++) {
                    if(PhyM.FF[j]==0){
                        freeBlock=j;
                        tlb.PID=npcb->pid;
                        tlb.virtual=vc_address;
                        tlb.fisica=freeBlock * 64 + contPag;
                        PhyM.M[tlb.fisica]=binary;
                        npcb->mm->pgb[contBlock]=tlb;
                        break;
                    }
                }
            }else{
                for(j=freeBlock; j<NUMBLOCK; j++) {
                    if(PhyM.FF[j]==0){ //first-Fit
                        freeBlock=j;
                        int index=freeBlock * 64 + contPag;
                        PhyM.M[index]=binary;
                        break;
                    }
                }
            }
            vc_address+=4;
            contPag++;
            if(contPag==64){
                contPag=0;
                contBlock++;
                sumContBlock++;
                PhyM.FF[j]=1;
            }
        }
        if(contPag!=0){
            sumContBlock++;
            PhyM.FF[j]=1;
        }
        PhyM.FB-=sumContBlock;
        npcb->numEntradasTLB=sumContBlock;
        pushQ(npcb, npcb->prioridad);
        LPA++;

    }else{
        PID--;
    }

    fclose(fd);
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
            pthread_mutex_lock( &locking );
            loader();
            pthread_mutex_unlock( &locking );
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
void scheduler(){
    int i,j,k,l;
    int index=0;
    if(realTimeClass.count>0){
        for(i=0; i<cpus; i++){
            for(j=0; j<cores; j++){
                for(k=0; k<hilos; k++){
                    if(CpuList[i].core[j].hilos[k].MyProc->pid==0){
                        for (l = index; l < 100; l++) {
                            if (realTimeClass.bitmap[l] == 1) {
                                PCB *process = pullQ(l);
                                process->quantumRestante=process->quantum;
                                CpuList[i].core[j].hilos[k].MyProc=process;
                                //Recuperamos estado del programa si ha sido ejectuado antes
                                CpuList[i].core[j].hilos[k].REG16=process->REG16;
                                CpuList[i].core[j].hilos[k].PC=process->PC;
                                CpuList[i].core[j].hilos[k].PTBR=process->mm->pgb;
                                nhejec++;
                                index=l;
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
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
            pthread_mutex_lock( &locking );
            scheduler();
            pthread_mutex_unlock( &locking );
            d2=0;
        }
        pthread_cond_signal(&sigT);
        pthread_cond_wait(&broadcast, &clkm);
    }
}
void ___error_message(int cod, char *s) {
    switch (cod) {
        case 0: // texto
            printf("☼☼☼☼☼☼☼☼ %s ☼☼☼☼☼☼☼☼\n", s);
            break; //0
        default:
            break; // default
    } // switch
    exit(-1);
}
void red () {
    printf("\033[1;31m");
}
void yellow () {
    printf("\033[0;33m");
}
void green () {
    printf("\033[0;32m");
}
void reset () {
    printf("\033[0m");
}
void print_machine(){
    int i,j,k;
    char lines[1024];

    strcpy(lines,"-------");
    printf("       ");
    for (i = 0; i < cpus; i++)
    {
        printf("|     cpu%02d    ",i);
        strcat(lines,"---------------");
    }
    printf("|\n%s-\n",lines);
    for (j = 0; j < cores; j++)
    {
        for (k = 0; k < hilos; k++){
            printf("core%02d |",j);
            for (i = 0; i < cpus; i++)
            {
                PCB *act_pcb=CpuList[i].core[j].hilos[k].MyProc;
                red();
                printf(" %04d",act_pcb->pid);
                green();
                printf(" %03d",act_pcb->PC);
                yellow();
                printf(" %03d",act_pcb->prioridad);
                reset();
                printf(" |");
            }
            printf("\n");

        }
        printf("%s-\n",lines);
    }
    /*print_ready_lists();*/
}
int contarLineas(char *filename){
    char c;
    int count=1;
    FILE *fp = fopen(filename, "r");
    for(c= getc(fp); c!=EOF; c=getc(fp)){
        if(c=='\n'){
            count++;
        }
    }
    fclose(fp);
    count-=2;
    return count;
}
char* getNumberFormat(){
    int lpa=LPA;
    int length = snprintf( NULL, 0, "%d", lpa );
    char *str;
    if(lpa < 10){
        str = malloc( length + 11 );
        snprintf( str, length + 11, "prog00%d.elf", lpa );
    }else if(lpa > 9){
        str = malloc( length + 10 );
        snprintf( str, length + 10, "prog0%d.elf", lpa );
    }else{
        str = malloc( length + 9 );
        snprintf( str, length + 9, "prog%d.elf", lpa );
    }
    return str;
}
/**
 * Funcion para generar nuevos procesos cuando se ha llegado al maximo generado
 * @return 0 si exito, 1 si error
 */
int genNewProcesses(){
    FILE          *fd;
    char          line[MAX_LINE_LENGTH], file_name[MAX_LINE_LENGTH];
    unsigned int  i, pnum, datum;
    unsigned int  code_start, code_size;
    unsigned int  data_start, data_size;
    unsigned int  var_offset1, var_offset2, var_offset3;
    unsigned char reg1, reg2, reg3;

    conf.user_lowest  = USER_LOWEST_ADDRESS;
    conf.virtual_bits = VIRTUAL_BITS_DEFAULT;
    conf.offset_bits  = PAGE_SIZE_BITS;
    conf.max_lines    = MAX_LINES_DEFAULT;
    conf.prog_name    = PROG_NAME_DEFAULT;
    conf.first_number = FIRST_NUMBER_DEFAULT;
    conf.how_many     = HOW_MANY_DEFAULT;

    user_highest = (1 << conf.virtual_bits) - 1;
    user_space   = user_highest - conf.user_lowest + 1;

    for(pnum = conf.first_number; pnum < (conf.first_number + conf.how_many); pnum++){

        sprintf(file_name,"%s%03d.elf", conf.prog_name, pnum);
        if((fd = fopen(file_name, "w")) == NULL){
            ___error_message(0,"Error while opening file");
        } // if

        code_start = conf.user_lowest;
        code_size  = (((rand() % conf.max_lines) >> 2) << 2) + 4 + 1; // Al menos un grupo instrucciones
        data_start = code_start + (code_size << 2);
        data_size  = (rand() % conf.max_lines) + 3; // Al menos 3 datos

        sprintf(line,".text %06X\n", code_start);
        fputs(line, fd);
        sprintf(line,".data %06X\n", data_start);
        fputs(line, fd);

        for (i=0; i < ((code_size - 1) >> 2); i++) { // bloques de cuatro líneas
            reg1 = rand() % 16;
            reg2 = (reg1 + 1) % 16;
            reg3 = (reg1 + 2) % 16;
            var_offset1  = (rand() % data_size) << 2;
            var_offset2  = (((var_offset1 >> 2) + 1) % data_size) << 2;
            var_offset3  = (((var_offset2 >> 2) + 1) % data_size) << 2;

            if (rand() % 2) { // add: ld ld add st
                sprintf(line,"0%1X%06X\n", reg1, data_start + var_offset1); //ld
                fputs(line, fd);
                sprintf(line,"0%1X%06X\n", reg2, data_start + var_offset2); // ld
                fputs(line, fd);
                sprintf(line,"2%1X%1X%1X0000\n", reg3, reg1, reg2); // add
                fputs(line, fd);
                sprintf(line,"1%1X%06X\n", reg3, data_start + var_offset3); // st
                fputs(line, fd);
            }
            else { // swap: ld ld st st
                sprintf(line,"0%1X%06X\n", reg1, data_start + var_offset1); //ld
                fputs(line, fd);
                sprintf(line,"0%1X%06X\n", reg2, data_start + var_offset2); // ld
                fputs(line, fd);
                sprintf(line,"1%1X%06X\n", reg1, data_start + var_offset2); // st
                fputs(line, fd);
                sprintf(line,"1%1X%06X\n", reg2, data_start + var_offset1); // st
                fputs(line, fd);
            }
        } // for code

        sprintf(line,"F0000000\n"); // exit
        fputs(line, fd);

        for (i=0; i < data_size; i++) {
            datum = (rand() % VALUE) - (VALUE >> 1);
            sprintf(line,"%08X\n", datum);
            fputs(line, fd);
        } // for data

        fclose(fd);
    }

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
 * Funcion para añadir un proceso a una cola determinada del RTC
 * @param pcb proceso a añadir
 * @param prioridad indice de la cola deseada
 * @return
 */
void pushQ(PCB *pcb, int prioridad){

    if(realTimeClass.pQ[prioridad]->count==0){
        realTimeClass.pQ[prioridad]->inicio = pcb;
        realTimeClass.bitmap[prioridad]=1;
    }else{
        realTimeClass.pQ[prioridad]->final->next = pcb;
    }
    realTimeClass.pQ[prioridad]->final = pcb;
    realTimeClass.pQ[prioridad]->count++;
    realTimeClass.count++;
}
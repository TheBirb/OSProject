//
// Created by Jon Moríñigo on 22/10/21.
//
#include "../Headers/depend.h"
void pull(pcq *cola, PCB *pcb){
    cola->final->next = &pcb;
    cola->final = &pcb;
    cola->count++;
}
PCB *push(pcq *cola){
    PCB *pcb = cola->inicio;
    cola->inicio= cola->inicio->next;
    cola->count--;
    return(&pcb);
}
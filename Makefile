HEADERS= Headers/depend.h
Default: Maquina

Maquina.o: Source/Maquina.c $(HEADERS)
	gcc -c Source/Maquina.c -o Maquina.o -lpthread
Program: Maquina.o
	gcc Maquina.o -o Maquina 
Clean:
	-rm -f Maquina.o
	-rm -f Maquina

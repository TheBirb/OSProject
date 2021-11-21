HEADERS= Headers/depend.h
Default: Maquina

Maquina.o: Source/Maquina.c $(HEADERS)
	gcc -c Source/Maquina.c -o Maquina.o
Program: Maquina.o
	gcc Maquina.o -o Maquina -lpthread
Clean:
	-rm -f Maquina.o
	-rm -f Maquina

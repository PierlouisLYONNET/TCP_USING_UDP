all: serveur1-NRV serveur2-NRV serveur3-NRV

serveur1-NRV: serveur1-NRV.o 
	gcc serveur1-NRV.o -o serveur1-NRV

serveur1-NRV.o: serveur1-NRV.c 
	gcc serveur1-NRV.c -c -o serveur1-NRV.o

serveur2-NRV: serveur2-NRV.o 
	gcc serveur2-NRV.o -o serveur2-NRV

serveur2-NRV.o: serveur2-NRV.c 
	gcc serveur2-NRV.c -c -o serveur2-NRV.o

serveur3-NRV: serveur3-NRV.o 
	gcc serveur3-NRV.o -o serveur3-NRV

serveur3-NRV.o: serveur3-NRV.c 
	gcc serveur3-NRV.c -c -o serveur3-NRV.o
	
clean:
	rm -f client serveur1-NRV serveur2-NRV serveur3NRV *.o
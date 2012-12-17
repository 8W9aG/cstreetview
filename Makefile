googlestreetview: clear main.o googlestreetview.o
	g++ main.o googlestreetview.o -lopencv_core -lopencv_highgui -lcurl -ltinyxml2 -lturbojpeg -o example

clear:
	rm -f *.o
	rm -f example

main.o:
	g++ -c main.c -o main.o

googlestreetview.o:
	g++ -c googlestreetview.c -o googlestreetview.o

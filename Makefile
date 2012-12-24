cstreetview: clear main.o cstreetview.o
	g++ main.o cstreetview.o -lopencv_core -lopencv_highgui -lopencv_imgproc -lcurl -ltinyxml2 -lturbojpeg -o example

clear:
	rm -f *.o
	rm -f example

main.o:
	g++ -c main.c -o main.o

cstreetview.o:
	g++ -c cstreetview.c -o cstreetview.o

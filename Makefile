all: mount-poi.cpp poi.o fuse_imp.o
	g++ mount-poi.cpp poi.o fuse_imp.o `pkg-config fuse --cflags --libs` -o poi

poi.o : poi.hpp poi.cpp
	g++ -Wall -c poi.cpp

fuse_imp.o : fuse_imp.hpp fuse_imp.cpp
	g++ -Wall -c fuse_imp.cpp
	
clean:
	rm *~
	
clear:
	rm *.o

all: mount-poi.cpp poi.o fuse_imp.o
<<<<<<< HEAD
	g++ mount-poi.cpp poi.o fuse_imp.o -D_FILE_OFFSET_BITS=64 `pkg-config fuse --cflags --libs` -o mount-poi
=======
	g++ mount-poi.cpp poi.o fuse_imp.o -D_FILE_OFFSET_BITS=64 `pkg-config fuse --cflags --libs` -o poi
>>>>>>> origin/master

poi.o : poi.hpp poi.cpp
	g++ -Wall -c poi.cpp -D_FILE_OFFSET_BITS=64

fuse_imp.o : fuse_imp.hpp fuse_imp.cpp
	g++ -Wall -c fuse_imp.cpp -D_FILE_OFFSET_BITS=64
	
clean:
	rm *~
	
clear:
	rm *.o

all: mount-poi.cpp poi.o fuse_imp.o
<<<<<<< HEAD
<<<<<<< HEAD
	g++ mount-poi.cpp poi.o fuse_imp.o -D_FILE_OFFSET_BITS=64 `pkg-config fuse --cflags --libs` -o mount-poi
=======
	g++ mount-poi.cpp poi.o fuse_imp.o -D_FILE_OFFSET_BITS=64 `pkg-config fuse --cflags --libs` -o poi
>>>>>>> origin/master
=======
	g++ mount-poi.cpp poi.o fuse_imp.o `pkg-config fuse --cflags --libs` -o poi
>>>>>>> 4dc131679ca0d6bb73359d2e98a0f4bf26682ff5

poi.o : poi.hpp poi.cpp
	g++ -Wall -c poi.cpp

fuse_imp.o : fuse_imp.hpp fuse_imp.cpp
	g++ -Wall -c fuse_imp.cpp
	
clean:
	rm *~
	
clear:
	rm *.o

#include "fuse_imp.hpp"
#include "poi.hpp"
#include "entry.hpp"
#include <iostream>


struct fuse_operations poi_operations;

POIFS POI;

using namespace std;

void init() {
	poi_operations.getattr	= poi_getattr;
	poi_operations.readdir	= poi_readdir;
	poi_operations.mkdir		= poi_mkdir;
	poi_operations.mknod		= poi_mknod;
	poi_operations.read 		= poi_read;
	poi_operations.rmdir		= poi_rmdir;
	poi_operations.unlink		= poi_unlink;
	poi_operations.rename		= poi_rename;
	poi_operations.write		= poi_write;
	poi_operations.truncate	= poi_truncate;
	poi_operations.chmod		= poi_chmod;
	poi_operations.link		= poi_link;
	poi_operations.open 		= poi_open;
};

int main(int argc, char *argv[]) {
	if (argc < 3 || argc > 4) {
		cout << "Invalid argument specified" << endl;
		cout << "Should be : ./mount-poi <mountdirectory> <poifile> [-new]";
		return 0;
	} else {
		if (argc > 3 && string(argv[3]) == "-new") {
			POI.createPoi(argv[2], argv[1]);
		}

		// read the newly created file
		POI.readPoi(argv[2]);

		// Init fuse
		init();
		int fuse_argc = 2;
		char* fuse_argv[2] = {argv[0], argv[1]};

		// Pass the rest to fuse
		return fuse_main(fuse_argc, fuse_argv, &poi_operations, NULL);
	}
}
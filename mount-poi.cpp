#include "fuse_imp.hpp"
#include "poi.hpp"
#include <iostream>

using namespace std;

static struct fuse_operations poi_operations {
	.getattr	= poi_getattr;
	.readdir	= poi_readdir;
	.mkdir		= poi_mkdir;
	.mknod		= poi_mknod;
	.read 		= poi_read;
	.rmdir		= poi_rmdir;
	.unlink		= poi_unlink;
	.rename		= poi_rename;
	.write		= poi_write;
	.truncate	= poi_truncate;
	.chmod		= poi_chmod;
	.link		= poi_link;
	.open 		= poi_open;
};


int main(int argc, char *argv[]) {
	if (argc < 3 || argc > 4) {
		cout << "Invalid argument specified" << endl;
		cout << "Should be : ./mount-poi <mountdirectory> <poifile> [-new]"
		return 0;
	else {
		if (argc > 3 && string(argv[3]) == "-new") {
			poi.createPoi(argv[2], argv[1]);
		}

		// read the newly created file
		poi.readPoi(argv[2]);

		// Init fuse
		int fuse_argc = 2;
		char* fuse_argv[2] = {argv[0], argv[1]};

		// Pass the rest to fuse
		return fuse_main(fuse_argc, fuse_argv, &poi_operations, NULL);
	}
}
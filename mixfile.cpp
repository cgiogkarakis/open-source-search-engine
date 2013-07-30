#include "gb-include.h"

#include <sys/time.h>  // gettimeofday()
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

main ( int argc , char *argv[] ) {

	// usage check
	if ( argc != 2 ) {
		fprintf(stderr,"mixfile <filename>\n");
		exit(-1);
	}

	// set File class
	char *fname = argv[1];
	int fd = open ( fname , O_RDONLY ) ;


	// open file
	if ( fd < 0 ) {
		fprintf(stderr,"mixfile::open: %s %s",fname,strerror(errno)); 
		exit(-1);
	}

	// get file size
        struct stat stats;
        stats.st_size = 0;
        stat ( fname , &stats );
	long fileSize = stats.st_size;

	// store a \0 at the end
	long bufSize = fileSize + 1;

	// make buffer to hold all
	char *buf = (char *) malloc ( bufSize );
	if ( ! buf) { 
		fprintf(stderr,"mixfile::malloc: %s",strerror(errno));
		exit(-1);
	}
	char *bufEnd = buf + bufSize;

	// read em all in
	long nr = read ( fd , buf , fileSize ) ;
	if ( nr != fileSize ) {
		fprintf(stderr,"mixfile::read: %s %s",fname,strerror(errno));
		exit(-1);
	}

	close ( fd );

	fprintf(stderr,"read %li bytes\n",nr);

	// store ptrs to each line
	char *lines[100000];

	// count lines in the file, which is now in memory, buf
	long  n = 0;

	// set first line
	lines[n++] = buf;

	// set lines array
	char *p = buf;
	for ( long i = 0 ; i < bufSize ; i++ ) 
		if ( i > 0 && buf[i-1] == '\n' ) {
			lines[n++] = &buf[i];
			// set to a \0
			buf[i-1] = '\0';
		}

	fprintf(stderr,"read %li lines\n",n);

	// mix the lines up
	for ( long i = 0 ; i < n ; i++ ) {
		// switch line #i with a random line
		long j = rand() % n;
		char *tmp = lines[i];
		lines[i]  = lines[j];
		lines[j]  = tmp;
	}

	// output to stdout
	for ( long i = 0 ; i < n ; i++ ) 
		printf("%s\n", lines[i]);
}

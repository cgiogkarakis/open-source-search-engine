#include "gb-include.h"

#include <sys/time.h>  // gettimeofday()
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

long urlDecode ( char *dest , char *s , long slen ) ;
long htob ( char s ) ;

main ( int argc , char *argv[] ) {

	// usage check
	if ( argc != 3 ) {
		fprintf(stderr,"numwords <filename> <numQueryWords>\n");
		exit(-1);
	}

	// set File class
	char *fname = argv[1];
	int fd = open ( fname , O_RDONLY ) ;

	// # words
	long nw = atoi ( argv[2] );

	// open file
	if ( fd < 0 ) {
		fprintf(stderr,"numwords::open: %s %s",fname,strerror(errno)); 
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
		fprintf(stderr,"numwords::malloc: %s",strerror(errno));
		exit(-1);
	}
	char *bufEnd = buf + bufSize;

	// read em all in
	long nr = read ( fd , buf , fileSize ) ;
	if ( nr != fileSize ) {
		fprintf(stderr,"numwords::read: %s %s",fname,strerror(errno));
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

	char sbuf [2048];

	// output to stdout
	for ( long i = 0 ; i < n ; i++ ) {
		char *s = strstr(lines[i],"q=");
		if ( ! s ) continue;
		s+= 2;
		char *start = s;
		// decode
		long nd = urlDecode ( sbuf , s , gbstrlen(s) );
		sbuf[nd] = '\0';
		// count words
		s = sbuf;
		long n = 0;
		long punct = 1;
		for ( ; *s ; s++ ) {
			//if ( *s == '%' ) {
			//	if ( *(s+1)=='2' && *(s+1)=='0' )
			//}
			if ( isalnum(*s) ) { 
				if ( punct == 1 ) { punct = 0; n++; }
				continue;
			}
			if ( s == start ) continue;
			if ( ispunct(*(s-1)) ) continue;
			punct = 1;
		}
		// does it match?
		if ( n == nw ) printf("%s\n", lines[i]);
	}
}

// . decodes "s/slen" and stores into "dest"
// . returns the number of bytes stored into "dest"
long urlDecode ( char *dest , char *s , long slen ) {
	long j = 0;
	for ( long i = 0 ; i < slen ; i++ ) {
		if ( s[i] == '+' ) { dest[j++]=' '; continue; }
		dest[j++] = s[i];
		if ( s[i]  != '%'  ) continue;
		if ( i + 2 >= slen ) continue;
		// convert hex chars to values
		unsigned char a = htob ( s[i+1] ) * 16; 
		unsigned char b = htob ( s[i+2] )     ;
		dest[j-1] = (char) (a + b);
		i += 2;
	}
	return j;
}

// convert hex digit to value
long htob ( char s ) {
	if ( isdigit(s) ) return s - '0';
	if ( s >= 'a'  && s <= 'f' ) return (s - 'a') + 10;
	if ( s >= 'A'  && s <= 'F' ) return (s - 'A') + 10;
	return 0;
}

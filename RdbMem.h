// Matt Wells, copyright Apr 2002

// . this handles memory alloaction for an Rdb class
// . Rdb::addRecord often copies the record data to add to the tree
//   so it now uses the RdbMem::dup() here

#ifndef _RDBMEM_
#define _RDBMEM_

class RdbMem {

 public:

	RdbMem();
	~RdbMem();

	// initialize us with the RdbDump class your rdb is using
	//bool init ( class RdbDump *rdb , long memToAlloc , char keySize );
	bool init ( class Rdb *rdb , long memToAlloc , char keySize ,
		    char *allocName );

	void reset();

	// . if a dump is not going on this uses the primary mem space
	// . if a dump is going on and this key has already been dumped
	//   (we check RdbDump::getFirstKey()/getLastKey()) add it to the
	//   secondary mem space, otherwise add it to the primary mem space
	//void *dupData ( key_t key , char *data , long dataSize );
	void *dupData ( char *key , char *data , long dataSize ,
			collnum_t collnum );

	// used by dupData
	//void *allocData ( key_t key , long dataSize );
	void *allocData ( char *key , long dataSize , collnum_t collnum );

	// how much mem is available?
	long getAvailMem() {
		// don't allow ptrs to equal each other...
		if ( m_ptr1 == m_ptr2 ) return 0;
		if ( m_ptr1 <  m_ptr2 ) return m_ptr2 - m_ptr1 - 1;
		return m_ptr1 - m_ptr2 - 1;
	}

	long getTotalMem() { return m_memSize; };

	long getUsedMem() { return m_memSize - getAvailMem(); };

	// used to determine when to dump
	bool is90PercentFull () { return m_is90PercentFull; };

	// . when a dump completes we free the primary mem space and make
	//   the secondary mem space the new primary mem space
	void  freeDumpedMem();

	// Rdb which contains this class calls this to prevent swap-out once
	// per minute or so
	//long scanMem ( ) ;

	// keep hold of this class
	//class RdbDump *m_dump;
	class Rdb *m_rdb;

	// the primary mem
	char *m_ptr1;
	// the secondary mem
	char *m_ptr2;

	// the full mem we alloc'd initially
	char *m_mem;
	long  m_memSize;

	// this is true when our primary mem is 90% of m_memSize
	bool  m_is90PercentFull;

	// limit ptrs for primary mem ptr, m_ptr1, to breech to be 90% full
	char *m_90up   ;
	char *m_90down ;

	char  m_ks;
	char *m_allocName;
};

#endif

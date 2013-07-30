// Matt Wells, Copyright May 2001

// . RdbList is the heart of Rdb, Record DataBase
// . an RdbList is a list of rdb records sorted by their keys
// . an rdb record is just a key with an optional dataSize and/or data
// . all records in the RdbList must have keys in [m_startKey, m_endKey]
// . TODO: speed up by using templates are by having 2-3 different RdbLists:
//         1 for dataLess Rdb's, 1 for fixedDataSize Rdb's, 1 for var dataSize

// . m_useHalfKeys is only for IndexLists
// . it is a compression method for key-only lists (data-less)
// . it allows use of 6-byte keys if the last 12-byte key before has the same
//   most significant 6 bytes
// . this saves space and time (35% of indexdb can be cut)
// . we cannot just override skipCurrentRecord(), etc. in IndexList because
//   it would have to be a virtual function thing (ptr to a function) when
//   called in RdbMap, Msg1, merge_r(), ... OR the callers would have
//   to have a separate routine just for IndexLists
// . for speed I opted to just add the m_useHalfKeys option to the RdbList 
//   class rather than have a virtual function or having to write lots of 
//   additional support routines for IndexLists

#ifndef _RDBLIST_H_
#define _RDBLIST_H_

class RdbList {

 public:

	// IndexList sees keys as termId/score/docId tuples
	friend class IndexList; // this class is derived from RdbList
	friend class RdbScan;   // for hacking to make first key read 12 bytes
	friend class RdbDump;   // for hacking m_listPtrHi/m_listPtr
	friend class RdbMap;    // for call to RdbList::setListPtr()
	friend class Msg1;

	RdbList () ;
	~RdbList () ;
	void constructor();
	void destructor ();

	// sets m_listSize to 0, keeps any allocated buffer (m_alloc), however
	void reset ( );

	// like reset, but frees m_alloc/m_allocSize and resets all to 0
	void freeList ( );

	// return false and sets g_errno on error
	bool copyList ( class RdbList *list );

	// . set it to this list
	// . "list" is a serialized sequence of rdb records sorted by key
	// . startKey/endKey specifies the list's range
	// . there may, however, be some keys in the list outside of the range
	// . if "ownData" is true we free "list" on our reset/destruction
	void set (char *list          , 
		  long  listSize      , 
		  char *alloc         ,
		  long  allocSize     ,
		  //key_t startKey      , 
		  //key_t endKey        ,
		  char *startKey      , 
		  char *endKey        ,
		  long  fixedDataSize , 
		  bool  ownData       ,
		  bool  useHalfKeys   ,
		  char  keySize       ); // 12 is default

	// call the above set()
	void set (char *list          , 
		  long  listSize      , 
		  char *alloc         ,
		  long  allocSize     ,
		  key_t startKey      , 
		  key_t endKey        ,
		  long  fixedDataSize , 
		  bool  ownData       ,
		  bool  useHalfKeys   ) {
		set(list,listSize,alloc,allocSize,(char *)&startKey,
		    (char *)&endKey,fixedDataSize,ownData,useHalfKeys,
		    sizeof(key_t));
	}

	// like above but uses 0/maxKey for startKey/endKey
	void set (char *list          , 
		  long  listSize      , 
		  char *alloc         ,
		  long  allocSize     ,
		  long  fixedDataSize , 
		  bool  ownData       ,
		  bool  useHalfKeys   ,
		  char  keySize       = sizeof(key_t) );

	// just set the start and end keys
	//void set ( key_t startKey , key_t endKey );
	void set ( char *startKey , char *endKey );

	void setStartKey ( key_t startKey ){
	  if ( m_ks!=12 ) { char *xx=NULL;*xx=0;}
	  KEYSET(m_startKey,(char *)&startKey,12);
	  //*((key_t *)m_startKey) = startKey; };
	};
	void setEndKey   ( key_t endKey   ){
	  if ( m_ks!=12 ) { char *xx=NULL;*xx=0;}
	  KEYSET(m_endKey,(char *)&endKey,12);
	  //*(key_t *)m_endKey   = endKey  ; };
	};
	void setStartKey ( char *startKey ){KEYSET(m_startKey,startKey,m_ks);};
	void setEndKey   ( char *endKey   ){KEYSET(m_endKey  ,endKey  ,m_ks);};

	void setUseHalfKeys ( bool use ) { m_useHalfKeys = use; };

	// if you don't want data to be freed on destruction then don't own it
	void setOwnData ( bool ownData ) { m_ownData = ownData; };

	void setFixedDataSize ( long fixedDataSize ) { 
		m_fixedDataSize = fixedDataSize; };

	//key_t getStartKey        () { return m_startKey; };
	//key_t getEndKey          () { return m_endKey;   };
	char *getStartKey        () { return m_startKey; };
	char *getEndKey          () { return m_endKey;   };
	long  getFixedDataSize   () { return m_fixedDataSize; };
	bool  getOwnData         () { return m_ownData; };

	void  getStartKey        ( char *k ) { KEYSET(k,m_startKey,m_ks);};
	void  getEndKey          ( char *k ) { KEYSET(k,m_endKey  ,m_ks);};

	void  getLastKey         ( char *k ) { 
		if ( ! m_lastKeyIsValid ) { char *xx=NULL;*xx=0; }
		KEYSET(k,getLastKey(),m_ks);};

	// will scan through each record if record size is variable
	long  getNumRecs         () ;

	// these operate on the whole list
	char *getList            () { return m_list; };
	long  getListSize        () { return m_listSize; };
	char *getListEnd         () { return m_list + m_listSize; };
	//key_t getListStartKey    () { return m_startKey; };
	//key_t getListEndKey      () { return m_endKey; };
	char *getListStartKey    () { return m_startKey; };
	char *getListEndKey      () { return m_endKey; };


	// often these equal m_list/m_listSize, but they may encompass
	char *getAlloc           () { return m_alloc; };
	long  getAllocSize       () { return m_allocSize; };

	// . skip over the current record and point to the next one
	// . returns false if we skipped into a black hole (end of list)
	bool skipCurrentRecord ( ) { 
		return skipCurrentRec ( getRecSize ( m_listPtr ) ); };

	bool skipCurrentRec ( ) { 
		return skipCurrentRec ( getRecSize ( m_listPtr ) ); };

	// this is specially-made for RdbMap's processing of IndexLists
	bool skipCurrentRec ( long recSize ) {
		m_listPtr += recSize;
		if ( m_listPtr >= m_listEnd ) return false;
		if ( m_ks == 18 ) {
			// a 6 byte key? do not change listPtrHi nor Lo
			if ( m_listPtr[0] & 0x04 ) return true;
			// a 12 byte key?
			if ( m_listPtr[0] & 0x02 ) {
				m_listPtrLo = m_listPtr + 6;
				return true;
			}
			// if it's a full 18 byte key, change both ptrs
			m_listPtrHi = m_listPtr + 12;
			m_listPtrLo = m_listPtr + 6;
			return true;
		}
		if ( m_useHalfKeys && ! isHalfBitOn ( m_listPtr ) ) 
			m_listPtrHi = m_listPtr + (m_ks-6);
		return true;
	};

	bool  isExhausted        () { return (m_listPtr >= m_listEnd); };
	//key_t getCurrentKey      () { return getKey      ( m_listPtr );};
	key_t getCurrentKey      () { 
		key_t key ; getKey ( m_listPtr,(char *)&key ); return key; };
	void  getCurrentKey      (void *key) { getKey(m_listPtr,(char *)key);};
	long  getCurrentDataSize () { return getDataSize ( m_listPtr );};
	char *getCurrentData     () { return getData     ( m_listPtr );};
	long  getCurrentRecSize  () { return getRecSize  ( m_listPtr );};
	long  getCurrentSize     () { return m_listEnd - m_listPtr; };
	char *getCurrentRec      () { return m_listPtr; };
	char *getListPtr         () { return m_listPtr; };
	char *getListPtrHi       () { return m_listPtrHi; };
	void  resetListPtr       () ;

	// are there any records in the list?
	bool  isEmpty     ( ) { return (m_listSize == 0); };

	// . add this record to the end of the list,  @ m_list+m_listSize
	// . returns false and sets errno on error
	// . grows list (m_allocSize) if we need more space
	//bool addRecord ( key_t &key , long dataSize , char *data ,
	bool addRecord ( key_t &key , long dataSize , char *data ,
			 bool bitch = true ) {
		return addRecord ((char *)&key,dataSize,data,bitch); };
	bool addRecord ( char *key , long dataSize , char *data ,
			 bool bitch = true );
	//bool addKey    ( key_t &key );

	// . record has key included in this case
	// . returns false and sets errno on error
	// . grows list (m_allocSize) if we need more space
	bool addRecordRaw ( char *rec , long recSize );

	// . constrain a list to [startKey,endKey]
	// . returns false and sets g_errno on error
	// . only called by Msg3.cpp for 1 list reads to avoid memmov()'ing
	//   and malloc()'ing
	// . may change m_list and/or m_listSize
	//bool constrain ( key_t   startKey    , 
	//		 key_t   endKey      ,
	bool constrain ( char   *startKey    , 
			 char   *endKey      ,
			 long    minRecSizes ,
			 long    hintOffset  ,
			 //key_t   hintKey     ,
			 char   *hintKey     ,
			 char   *filename    ,
			 long    niceness    ) ;

	// . this MUST be called before calling merge_r() 
	// . will alloc enough space for m_listSize + sizes of "lists"
	bool prepareForMerge ( RdbList **lists            , 
			       long      numLists         , 
			       long      minRecSizes = -1 );

	// . merge the lists into this list
	// . set our startKey/endKey to "startKey"/"endKey"
	// . exclude any records from lists not in that range
	void merge_r ( RdbList **lists         , 
		       long      numLists      , 
		       //key_t     startKey      , 
		       //key_t     endKey        , 
		       char     *startKey      , 
		       char     *endKey        , 
		       long      minRecSizes   ,
		       bool      removeNegRecs ,
		       char      rdbId         ,
		       long     *filtered      ,
		       long     *tfns          , // used for titledb
		       RdbList  *tfndbList     , // used for titledb
		       bool      isRealMerge   ,
		       long      niceness      );
	/*
	// . we now use half keys for Tfndb, cuts mem usage in half.
	// . Tfndb keys are special in that we ignore the 'f' and C' bits
	//   when comparing keys in this routine
	bool indexMerge_r ( RdbList **lists         ,  
			    long      numLists      ,
			    //key_t     startKey      ,
			    //key_t     endKey        ,
			    char     *startKey      ,
			    char     *endKey        ,
			    long      minRecSizes   ,
			    bool      removeNegKeys ,
			    //key_t     prevKey       ,
			    char     *prevKey       ,
			    long     *prevCountPtr  ,
			    long      truncLimit    ,
			    long     *dupsRemoved   ,
			    //bool      isTfndb       ,
			    char      rdbId         ,
			    long     *filtered      ,
			    bool      doGroupMask   , //= true ,
			    bool      isRealMerge   , //= false );
			    bool      useBigRootList ,
			    long      niceness       );

	bool indexMerge_r ( RdbList **lists         ,  
			    long      numLists      ,
			    key_t     startKey      ,
			    key_t     endKey        ,
			    long      minRecSizes   ,
			    bool      removeNegKeys ,
			    key_t     prevKey       ,
			    long     *prevCountPtr  ,
			    long      truncLimit    ,
			    long     *dupsRemoved   ,
			    //bool      isTfndb       ,
			    char      rdbId         ,
			    long     *filtered      ,
			    bool      doGroupMask   ,//= true ,
			    bool      isRealMerge   ,//= false ) {
			    bool      useBigRootList ,
			    long      niceness       ) {
		return indexMerge_r ( lists         ,  
				      numLists      ,
				      (char *)&startKey      ,
				      (char *)&endKey        ,
				      minRecSizes   ,
				      removeNegKeys ,
				      (char *)&prevKey       ,
				      prevCountPtr  ,
				      truncLimit    ,
				      dupsRemoved   ,
				      //isTfndb       ,
				      rdbId         ,
				      filtered      ,
				      doGroupMask   ,
				      isRealMerge   ,
				      useBigRootList ,
				      niceness       ); };
	*/

	bool posdbMerge_r ( RdbList **lists         ,  
			    long      numLists      ,
			    char     *startKey      ,
			    char     *endKey        ,
			    long      minRecSizes   ,
			    bool      removeNegKeys ,
			    long     *filtered      ,
			    bool      doGroupMask   ,
			    bool      isRealMerge   ,
			    long      niceness       ) ;


	// returns false if we skipped into a black hole (end of list)
	long getRecSize ( char *rec ) {
		// posdb?
		if ( m_ks == 18 ) {
			if ( rec[0]&0x04 ) return 6;
			if ( rec[0]&0x02 ) return 12;
			return 18;
		}
		if ( m_useHalfKeys ) {
			//if ( isHalfBitOn(rec) ) return 6;
			if ( isHalfBitOn(rec) ) return m_ks-6;
			//return sizeof(key_t);
			return m_ks;
		}
		//if (m_fixedDataSize == 0) return sizeof(key_t);
		//if (m_fixedDataSize >0) return sizeof(key_t)+m_fixedDataSize;
		//return *(long *)(rec + sizeof(key_t)) + sizeof(key_t) + 4 ;
		if (m_fixedDataSize == 0) return m_ks;
		// negative keys always have no datasize entry
		if ( (rec[0] & 0x01) == 0 ) return m_ks;
		if (m_fixedDataSize >  0) return m_ks+m_fixedDataSize;
		return *(long *)(rec + m_ks) + m_ks + 4 ;
	};

	// . is the format bit set? that means it's a 12-byte key
	// . used exclusively for index lists (indexdb)
	// . see Indexdb.h for format of the 12-byte and 6-byte indexdb keys
	bool isHalfBitOn ( char *rec ) { return ( *rec & 0x02 ); };

	bool useHalfKeys () { return m_useHalfKeys; };

	char *getData     ( char *rec ) ;
	long  getDataSize ( char *rec ) ;
	void  getKey      ( char *rec , char *key ) ;
	key_t getKey      ( char *rec ) {
		key_t k; getKey(rec,(char *)&k); return k; };

	// . merge_r() sets m_lastKey for the list it merges the others into
	// . otherwise, this may be invalid
	//key_t getLastKey  ( ) ;
	char *getLastKey  ( ) ;
	//void  setLastKey  ( key_t k );
	void  setLastKey  ( char *k );
	// sometimes we don't have a valid m_lastKey because it is only
	// set in calls to constrain(), merge_r() and indexMerge_r()
	bool  isLastKeyValid () { return m_lastKeyIsValid; };

	//key_t getFirstKey ( ) { return *(key_t *)m_list; };
	char *getFirstKey ( ) { return m_list; };


	bool growList ( long newSize ) ;

	// . check to see if keys in order
	// . logs any problems
	// . set "removedNegRecs" to true if neg recs should have been removed
	// . sleeps if any problems encountered
	bool checkList_r ( bool removedNegRecs , bool sleepOnProblem = true ,
			   char rdbId = 0 ); // RDB_NONE );

	// . removes records whose keys aren't in proper range (corruption)
	// . returns false and sets errno on error/problem
	bool removeBadData_r ( ) ;

	// . print out the list (uses log())
	int printList ();

	void  setListPtrs ( char *p , char *hi ) {m_listPtr=p;m_listPtrHi=hi;};

	void setListSize ( long size ) { m_listSize = size; };

	//void testIndexMerge ( );

	// private:

	bool checkIndexList_r ( bool removedNegRecs , bool sleepOnProblem );

	// the unalterd raw list. keys may be outside of [m_startKey,m_endKey]
	char  *m_list;
	long   m_listSize;     // how many bytes we're using for a list
	//key_t  m_startKey;
	//key_t  m_endKey;
	char   m_startKey [ MAX_KEY_BYTES ];
	// the list contains all the keys in [m_startKey,m_endKey] so make
	// sure if the list is truncated by minrecsizes that you decrease
	// m_endKey so this is still true. seems like zak did not do that
	// for rdbbuckets code.
	char   m_endKey   [ MAX_KEY_BYTES ];
	char  *m_listEnd;      // = m_list + m_listSize
	char  *m_listPtr;      // points to current record in list

	long   m_allocSize;  // how many bytes we've allocated at m_alloc
	char  *m_alloc    ;  // start of chunk that was allocated

	// m_fixedDataSize is -1 if records are variable length,
	// 0 for data-less records (keys only) and N for records of dataSize N
	long  m_fixedDataSize;

	// this is set to the last key in this list if we were made by merge()
	//key_t m_lastKey;
	char  m_lastKey [ MAX_KEY_BYTES ];
	bool  m_lastKeyIsValid;

	// max list rec sizes to merge as set by prepareForMerge()
	long   m_mergeMinListSize;

	// . this points to the most significant 6 bytes of a key
	// . only valid if m_useHalfKeys is true
	char  *m_listPtrHi;

	// for the secondary compression bit for posdb
	char  *m_listPtrLo;

	// do we own the list data (m_list)? if so free it on destruction
	bool   m_ownData;       

	// are keys compressed? only used for index lists right now
	bool   m_useHalfKeys;

	// keysize, usually 12, for 12 bytes. can be 16 for date index (datedb)
	char   m_ks;
	// if first key is only 6 bytes, we store the top 6 here and
	// point m_listPtrHi to it when list ptrs are reset
	//char   m_tmp[6];
};

// . inline this which compares to keys split into 6 byte ptrs
// . returns -1, 0 , 1 if a < b , a == b , a > b
// . for comparison purposes, we must set 0x02 (half bits) on all keys
//   so negative keys will always be ordered before their positive
/*
inline char cmp ( char *alo , char *ahi , char *blo , char *bhi ) {
	if (*(unsigned long  *)(&ahi[2])<*(unsigned long  *)&bhi[2]) return -1;
	if (*(unsigned long  *)(&ahi[2])>*(unsigned long  *)&bhi[2]) return  1;
	if (*(unsigned short *)( ahi   )<*(unsigned short *)bhi    ) return -1;
	if (*(unsigned short *)( ahi   )>*(unsigned short *)bhi    ) return  1;
	if (*(unsigned long  *)(&alo[2])<*(unsigned long  *)&blo[2]) return -1;
	if (*(unsigned long  *)(&alo[2])>*(unsigned long  *)&blo[2]) return  1;
	// we now ignore the half bit
	if ( ((*(unsigned short *)( alo   ))|0x02) <
	     ((*(unsigned short *)  blo    )|0x02)  ) return -1;
	if ( ((*(unsigned short *)( alo   ))|0x02) >
	     ((*(unsigned short *)  blo    )|0x02)  ) return  1;
	return 0;
};
*/

// man, inline is not working... don't count on it
#define fcmp(alo,ahi,blo,bhi) \
          (*(unsigned long  *)&((char *)ahi)[2] <      \
	   *(unsigned long  *)&((char *)bhi)[2]  ? -1  \
       :  (*(unsigned long  *)&((char *)ahi)[2] >      \
	   *(unsigned long  *)&((char *)bhi)[2]  ?  1  \
       :  (*(unsigned short *) ((char *)ahi)    <      \
	   *(unsigned short *) ((char *)bhi)     ? -1  \
       :  (*(unsigned short *) ((char *)ahi)    >      \
	   *(unsigned short *) ((char *)bhi)     ?  1  \
       :  (*(unsigned long  *)&((char *)alo)[2] <      \
	   *(unsigned long  *)&((char *)blo)[2]  ? -1  \
       :  (*(unsigned long  *)&((char *)alo)[2] >      \
	   *(unsigned long  *)&((char *)blo)[2]  ?  1  \
       :(((*(unsigned short *) ((char *)alo)    )|0x02) <        \
	 ((*(unsigned short *) ((char *)blo)    )|0x02)    ? -1  \
       :(((*(unsigned short *) ((char *)alo)    )|0x02) >        \
	 ((*(unsigned short *) ((char *)blo)    )|0x02)    ?  1  \
	 : 0 ))))))))

// like above but we treat positive and negative keys as identical
#define fcmp2(alo,ahi,blo,bhi) \
          (*(unsigned long  *)&((char *)ahi)[2] <      \
	   *(unsigned long  *)&((char *)bhi)[2]  ? -1  \
       :  (*(unsigned long  *)&((char *)ahi)[2] >      \
	   *(unsigned long  *)&((char *)bhi)[2]  ?  1  \
       :  (*(unsigned short *) ((char *)ahi)    <      \
	   *(unsigned short *) ((char *)bhi)     ? -1  \
       :  (*(unsigned short *) ((char *)ahi)    >      \
	   *(unsigned short *) ((char *)bhi)     ?  1  \
       :  (*(unsigned long  *)&((char *)alo)[2] <      \
	   *(unsigned long  *)&((char *)blo)[2]  ? -1  \
       :  (*(unsigned long  *)&((char *)alo)[2] >      \
	   *(unsigned long  *)&((char *)blo)[2]  ?  1  \
       :(((*(unsigned short *) ((char *)alo)    )|0x03) <        \
	 ((*(unsigned short *) ((char *)blo)    )|0x03)    ? -1  \
       :(((*(unsigned short *) ((char *)alo)    )|0x03) >        \
	 ((*(unsigned short *) ((char *)blo)    )|0x03)    ?  1  \
	 : 0 ))))))))


// like above but we treat positive and negative keys as identical
#define fcmp2low(alo,blo) \
          (*(unsigned long  *)&((char *)alo)[2] <      \
	   *(unsigned long  *)&((char *)blo)[2]  ? -1  \
       :  (*(unsigned long  *)&((char *)alo)[2] >      \
	   *(unsigned long  *)&((char *)blo)[2]  ?  1  \
       :(((*(unsigned short *) ((char *)alo)    )|0x03) <        \
	 ((*(unsigned short *) ((char *)blo)    )|0x03)    ? -1  \
       :(((*(unsigned short *) ((char *)alo)    )|0x03) >        \
	 ((*(unsigned short *) ((char *)blo)    )|0x03)    ?  1  \
	 : 0 ))))

// . like above but this compares Tfndb keys so it ignores the tfn bits
// . see Tfndb.h for the bit map of a Tfndb key
inline char cmp2b ( char *alo , char *ahi , char *blo , char *bhi ) {
	//if(*(unsigned long  *)(&ahi[2])<*(unsigned long  *)&bhi[2])return -1;
	//if(*(unsigned long  *)(&ahi[2])>*(unsigned long  *)&bhi[2])return  1;
	if (*(unsigned long  *)( ahi   )<*(unsigned long  *)bhi    ) return -1;
	if (*(unsigned long  *)( ahi   )>*(unsigned long  *)bhi    ) return  1;
	if (*(unsigned long  *)(&alo[2])<*(unsigned long  *)&blo[2]) return -1;
	if (*(unsigned long  *)(&alo[2])>*(unsigned long  *)&blo[2]) return  1;
	// . we ignore the half bit AND tfn bits (see Tfndb.h key bitmap)
	// . now we also treat negative and positive keys as the same
	if ( ((*(unsigned short *)( alo   ))|0x03ff) <
	     ((*(unsigned short *)  blo    )|0x03ff)  ) return -1;
	if ( ((*(unsigned short *)( alo   ))|0x03ff) >
	     ((*(unsigned short *)  blo    )|0x03ff)  ) return  1;
	return 0;
};

/*
inline char cmp2b ( char *alo , char *ahi , char *blo , char *bhi ) {
	if (*(unsigned long  *)(&ahi[2])!=*(unsigned long *)&bhi[2]) return 0;
	if (*(unsigned short *)( ahi   )!=*(unsigned short *)bhi   ) return 0;
	if (*(unsigned long  *)(&alo[2])!=*(unsigned long *)&blo[2]) return 0;
	// we now ignore the half bit AND f bits and C bit (see Tfndb.h bitmap)
	// AND set negative bit on blo
	if ( ((*(unsigned short *)( alo   ))|0x03fe) !=
	     ((*(unsigned short *)  blo    )|0x03ff)  ) return 0;
	return 1;
};
*/


// this is for 16-byte keys
#define bfcmp(alo,ahi,blo,bhi) \
          (*(unsigned long  *)&((char *)ahi)[2] <      \
	   *(unsigned long  *)&((char *)bhi)[2]  ? -1  \
       :  (*(unsigned long  *)&((char *)ahi)[2] >      \
	   *(unsigned long  *)&((char *)bhi)[2]  ?  1  \
       :  (*(unsigned short *) ((char *)ahi)    <      \
	   *(unsigned short *) ((char *)bhi)     ? -1  \
       :  (*(unsigned short *) ((char *)ahi)    >      \
	   *(unsigned short *) ((char *)bhi)     ?  1  \
       :  (*(unsigned long long *)&((char *)alo)[2] <      \
	   *(unsigned long long *)&((char *)blo)[2]  ? -1  \
       :  (*(unsigned long long *)&((char *)alo)[2] >      \
	   *(unsigned long long *)&((char *)blo)[2]  ?  1  \
       :(((*(unsigned short *) ((char *)alo)    )|0x02) <        \
	 ((*(unsigned short *) ((char *)blo)    )|0x02)    ? -1  \
       :(((*(unsigned short *) ((char *)alo)    )|0x02) >        \
	 ((*(unsigned short *) ((char *)blo)    )|0x02)    ?  1  \
	 : 0 ))))))))

// like above but we treat positive and negative keys as identical
#define bfcmp2(alo,ahi,blo,bhi) \
          (*(unsigned long  *)&((char *)ahi)[2] <      \
	   *(unsigned long  *)&((char *)bhi)[2]  ? -1  \
       :  (*(unsigned long  *)&((char *)ahi)[2] >      \
	   *(unsigned long  *)&((char *)bhi)[2]  ?  1  \
       :  (*(unsigned short *) ((char *)ahi)    <      \
	   *(unsigned short *) ((char *)bhi)     ? -1  \
       :  (*(unsigned short *) ((char *)ahi)    >      \
	   *(unsigned short *) ((char *)bhi)     ?  1  \
       :  (*(unsigned long long *)&((char *)alo)[2] <      \
	   *(unsigned long long *)&((char *)blo)[2]  ? -1  \
       :  (*(unsigned long long *)&((char *)alo)[2] >      \
	   *(unsigned long long *)&((char *)blo)[2]  ?  1  \
       :(((*(unsigned short *) ((char *)alo)    )|0x03) <        \
	 ((*(unsigned short *) ((char *)blo)    )|0x03)    ? -1  \
       :(((*(unsigned short *) ((char *)alo)    )|0x03) >        \
	 ((*(unsigned short *) ((char *)blo)    )|0x03)    ?  1  \
	 : 0 ))))))))


inline char bfcmpPosdb ( char *alo , char *ame , char *ahi , 
			 char *blo , char *bme , char *bhi ) {
	if (*(unsigned long  *)( ahi+2 )<*(unsigned long  *)(bhi+2)) return -1;
	if (*(unsigned long  *)( ahi+2 )>*(unsigned long  *)(bhi+2)) return  1;
	if (*(unsigned short *)( ahi   )<*(unsigned short *)(bhi  )) return -1;
	if (*(unsigned short *)( ahi   )>*(unsigned short *)(bhi  )) return  1;

	if (*(unsigned long  *)( ame+2 )<*(unsigned long  *)(bme+2)) return -1;
	if (*(unsigned long  *)( ame+2 )>*(unsigned long  *)(bme+2)) return  1;
	if (*(unsigned short *)( ame   )<*(unsigned short *)(bme  )) return -1;
	if (*(unsigned short *)( ame   )>*(unsigned short *)(bme  )) return  1;

	if (*(unsigned long  *)( alo+2 )<*(unsigned long  *)(blo+2)) return -1;
	if (*(unsigned long  *)( alo+2 )>*(unsigned long  *)(blo+2)) return  1;

	if ( ((*(unsigned short *)( alo   ))|0x0007) <
	     ((*(unsigned short *)  blo    )|0x0007)  ) return -1;
	if ( ((*(unsigned short *)( alo   ))|0x0007) >
	     ((*(unsigned short *)  blo    )|0x0007)  ) return  1;

	return 0;
};

#endif

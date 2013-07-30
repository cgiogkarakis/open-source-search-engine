// Matt Wells,  Copyright, Apr 2001

// like RdbTree but custom for MemPool.h

#ifndef _MEMPOOLTREE_H_
#define _MEMPOOLTREE_H_

#include "Mem.h"          // for g_mem.calloc and g_mem.malloc

class MemKey : public key_t {
 public:
	// . bitmap of "size" key needed by MemPool::malloc()
	// . 1U000000 00000000 00000000 00000000  U = in use?
	// . ssssssss ssssssss ssssssss ssssssss  s = size
	// . ffffffff ffffffff ffffffff ffffffff  f = ptr into m_mem
	void setSizeKey   ( long size , void *ptr , bool inUse ) {
		if ( inUse ) n1 = 0xc0000000;
		else         n1 = 0x80000000;
		// least significant long comes first
		*(((long *)&n0)+0) = (long) ptr; 
		*(((long *)&n0)+1) = (long) size;};
	// . bitmap of "offset" key needed by MemPool::free()
	// . 0U000000 00000000 00000000 00000000  U = in use? 
	// . ffffffff ffffffff ffffffff ffffffff  f = ptr into m_mem
	// . ssssssss ssssssss ssssssss ssssssss  s = size
	void setOffsetKey ( void *ptr , long size , bool inUse ) {
		if ( inUse ) n1 = 0x40000000;
		else         n1 = 0;
		// least significant long comes first
		*(((long *)&n0)+0) = (long) size; 
		*(((long *)&n0)+1) = (long) ptr; };

	bool  inUse      () { if (n1 & 0x40000000) return true; return false;};
	bool  isSizeKey  () { if (n1 & 0x80000000) return true; return false;};
	long  getSize    () {
		if ( isSizeKey() ) return n0 >> 32;
		else               return n0 & 0xffffffff; };
	char *getOffset  () {
		if ( isSizeKey() ) return (char *)(long)(n0 & 0xffffffff);
		else               return (char *)(long)(n0 >> 32); };
	void  setInUse ( bool inuse ) {
		if ( inuse ) n1 = 0x40000000;
		else         n1 = 0;
	};
};

// TODO: go back to using longs instead of MemNodes in case we get
// 64bit ptrs, we'd use double the ram!!
class MemNode {
 public:
	MemKey         m_key;  
	class MemNode *m_left;
	class MemNode *m_right;
	class MemNode *m_parent;
	char           m_depth;
};

class MemPoolTree {

 public:

	// these alloc and free m_mem
	 MemPoolTree       ( );
	~MemPoolTree       ( );

	// . pass in the chunk of memory we'll use
	// . we grow our MemNode array from top down, like a stack
	bool init ( char *mem , long memSize );

	// returns -1 and sets errno on error
	MemNode *addNode ( MemKey &key );

	// returns NULL if not found
	MemNode *getNode ( MemKey &key );

        // . get the node whose key is >= key 
        // . much much slower than getNextNode() below
        MemNode *getNextNode ( MemKey &key   );
        MemNode *getNextNode ( MemNode *node );
	
	// . get the node whose key is <= "key"
        MemNode *getPrevNode ( MemKey &key   );
        MemNode *getPrevNode ( MemNode *node );

	void deleteNode ( MemNode *node ) ;

	void deleteNode ( MemKey &key ) {
		MemNode *i = getNode ( key ); deleteNode(i); };

	// get # of available nodes that are above m_floor
	long getNumEmptyNodesAboveFloor ( ) {
		return (long)((MemNode *)(m_mem+m_memSize) - m_floor) - 
			m_numUsedNodes; };

	char *getFloor () { return (char *)m_floor; };

	MemNode *addSizeNode   ( long size , void *ptr , bool occupied ) {
		if ( size <= 0 ) return (MemNode *)0x7fffffff;
		MemKey k; k.setSizeKey ( size , ptr , occupied ) ;
		return addNode ( k ); };

	MemNode *addOffsetNode ( void *ptr , long size , bool occupied ) {
		if ( size <= 0 ) return (MemNode *)0x7fffffff;
		MemKey k ; k.setOffsetKey ( ptr , size , occupied ) ;
		return addNode ( k ); };

	void deleteOffsetNode  ( void *ptr , long size , bool occupied ) {
		if ( size <= 0 ) return;
		MemKey k ; k.setOffsetKey ( ptr , size , occupied ) ;
		deleteNode ( k ); };

	void deleteSizeNode    ( long size , void *ptr , bool occupied ) {
		if ( size <= 0 ) return;
		MemKey k ; k.setSizeKey ( size , ptr , occupied ) ;
		deleteNode ( k ); };

 private:

	void setDepths         ( MemNode *bottomNode );
	MemNode * rotateRight  ( MemNode *pivotNode  );
	MemNode * rotateLeft   ( MemNode *pivotNode  );

	char    *m_mem;
	long     m_memSize;
	// switch between picking left and right kids to replace deleted nodes
	// in order to keep the tree more balanced
	char     m_pickRight;
	// the node at the top of the tree
	MemNode *m_headNode;
	// how many are in use?
	long     m_numUsedNodes ;
	// node of the next available/empty node
	MemNode *m_nextNode;
	// maximum node # that was ever used at some point in time
	MemNode *m_floor;
};

#endif

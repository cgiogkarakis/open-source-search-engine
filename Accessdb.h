//
// Author: Matt Wells
//
// Rdb to store the access log

#ifndef _ACCESSDB_H_
#define _ACCESSDB_H_

bool sendPageAccount ( class TcpSocket *s , class HttpRequest *r ) ;

#include "Rdb.h"
#include "Msg4.h"

class AccessRec {
 public:
	key128_t  m_key128;
	long      m_ip;
	long long m_fbId; // facebook id, 0 if none
};

class Accessdb {

 public:
	// reset m_rdb
	void reset() { m_rdb.reset(); };

	// initialize m_rdb
	bool init( );

	bool addColl ( char *coll, bool doVerify = false );

	Rdb *getRdb() { return &m_rdb; }

	key128_t makeKey1 ( long long now, long long widgetId64 ) ;
	key128_t makeKey2 ( long long now, long long widgetId64 ) ;

	bool addAccess ( class HttpRequest *r , long ip );

	bool registerHandler ( ) ;

	Rdb 	  m_rdb;
	
	long m_niceness;
	
	AccessRec m_arec[2];

	Msg4 m_msg4;
	bool m_msg4InUse;
};

extern class Accessdb g_accessdb;

#endif

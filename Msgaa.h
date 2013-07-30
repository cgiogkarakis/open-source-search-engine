#ifndef _MSGAA_H_
#define _MSGAA_H_

#include "gb-include.h"
#include "Url.h"
#include "SearchInput.h"

#define MAX_QBUF_SIZE (MAX_URL_LEN+200)

class Msgaa {

public:
	Msgaa();
	~Msgaa(); 

	bool addSitePathDepth ( class TagRec *gr  , 
				class Url    *url ,
				char *coll ,
				void *state ,
				void (* callback)(void *state) ) ;

	bool gotResults ( ) ;

	SearchInput m_si;
	class TagRec *m_gr;
	class Url    *m_url;
	char   *m_coll;
	void   *m_state;
	void  (*m_callback) (void *state );
	long    m_sitePathDepth;

	class Msg40 *m_msg40;
	long m_pathDepth;
	char m_qbuf[MAX_QBUF_SIZE];
	//long m_niceness;

	long m_oldSitePathDepth;
	long m_newSitePathDepth;

};

#endif

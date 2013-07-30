
#ifndef _POSTQUERYRERANK_H_
#define _POSTQUERYRERANK_H_

#include "HashTableT.h"
#include "Msg20.h"
#include "Sanity.h"
#include "fctypes.h"

class Msg40;
#include "SearchInput.h"
struct M20List;

// type for saving Msg20s from results prior to first result
struct savedM20Data {
	long score;
	int tier;
	long long docId;
	char clusterLevel;
};


struct ComTopInDmozRec {
	long cnt;      // count of pages with this same topic
	float demFact; // decay factor for this topic
};

typedef float rscore_t;

#define MINSCORE      1
#define MIN_SAVE_SIZE 100
#define PQR_BUF_SIZE  MAX_QUERY_LEN

class PostQueryRerank {
public:
	static bool init ( );

	PostQueryRerank  ( );
	~PostQueryRerank ( );

	bool set1 ( Msg40 *, SearchInput * );
	bool set2 ( long resultsNeeded );

	bool isEnabled ( ) { return m_enabled; };

	bool preRerank    ( );
	bool rerank       ( );
	bool postRerank   ( );
	void rerankFailed ( );

private:
	rscore_t rerankLowerDemotesMore ( rscore_t score, 
				      float value, float maxValue,
				      float factor,
				      char *method, char *reason ) {
		//log( LOG_DEBUG, "query: rerankLowerDemotesMore -- "
		//     "score:%ld, value:%3.3f, maxValue:%3.3f, factor:%3.3f AWL",
		//     score, value, maxValue, factor );

		if ( value >= maxValue ) return score;

		rscore_t temp = score;
		score = (rscore_t)(score *
				    (1.0 - 
				     ((maxValue-value)*factor/maxValue)));
		if ( score < MINSCORE ) score = MINSCORE;
		if(m_si->m_debug) 
		  logf( LOG_DEBUG, "query: pqr: result demoted "
		     "%3.3f%% because it has %3.1f / %3.1f %s. method: '%s'",
		     100-100*(float)score/temp, 
		     value, maxValue,
		     reason, method );
				
		return score;
	};
	rscore_t rerankHigherDemotesMore ( rscore_t score,
				       float value, float maxValue,
				       float factor,
				       char *method, char *reason ) {
		//log( LOG_DEBUG, "query: rerankHigherDemotesMore -- "
		//     "score:%ld, value:%3.3f, maxValue:%3.3f, factor:%3.3f AWL",
		//     score, value, maxValue, factor );

		rscore_t temp = score;
		if ( value >= maxValue ) 
			score = (rscore_t)((1.0-factor)*score);
		else
			score = (rscore_t)(score * 
					    (1.0 - value*factor/maxValue));
		if ( score < MINSCORE ) score = MINSCORE;
		if(m_si->m_debug) 
		  logf( LOG_DEBUG, "query: pqr: result demoted "
		     "%3.3f%% because it has %3.1f / %3.1f %s. method: '%s'",
		     100-100*(float)score/temp, 
		     value, maxValue,
		     reason, method );

		return score;
	};
	rscore_t rerankAssignPenalty ( rscore_t score, float factor,
				   char *method, char *reason ) {
		//log( LOG_DEBUG, "query: rerankAssignPenalty -- "
		//     "score:%ld, factor:%3.3f AWL",
		//     score, factor );

		rscore_t temp = score;
		score = (rscore_t)(score * (1.0 - factor));
		if (score < MINSCORE) score = MINSCORE;
		if(m_si->m_debug || g_conf.m_logDebugPQR ) 
		  logf( LOG_DEBUG, "query: pqr: result demoted "
		     "%3.3f%% because %s. method '%s'",
		     100-100*(float)score/temp, reason, method );

		return score;
	};

	rscore_t rerankLanguageAndCountry ( rscore_t score, 
					uint8_t lang, uint8_t summaryLang,
					    uint16_t country ,
					    class Msg20 *msg20 );
	inline 
	rscore_t rerankLanguage ( rscore_t score, 
			      uint8_t lang );
	rscore_t rerankQueryTermsOrGigabitsInUrl ( rscore_t score,
					       Url *pageUrl );
	inline 
	rscore_t rerankQuality ( rscore_t score, 
			     unsigned char quality );
	inline 
	rscore_t rerankPathsInUrl ( rscore_t score,
				char *url,
				long urlLen );
	inline 
	rscore_t rerankNoCatId ( rscore_t score,
			     long numCatIds,
			     long numIndCatIds );
	rscore_t rerankSmallestCatIdHasSuperTopics ( rscore_t score,
						 Msg20 *msg20 );
	inline 
	rscore_t rerankPageSize ( rscore_t score,
			      long docLen );
	//bool getLocation ( char *resBuf, long resBufLen,
	//		   long *resLen, long *resPop,
	//		   char *buf, long bufLen);
	//bool preRerankNonLocationSpecificQueries ( );
	//rscore_t rerankNonLocationSpecificQueries ( rscore_t score,
	//					Msg20 *msg20 );
	inline 
	rscore_t rerankContentType ( rscore_t score,
				 char contentType );
	bool preRerankOtherPagesFromSameHost( Url *pageUrl );
	rscore_t rerankOtherPagesFromSameHost ( rscore_t score, 
					    Url *pageUrl );
	bool preRerankCommonTopicsInDmoz( Msg20Reply *mr );
	rscore_t rerankCommonTopicsInDmoz ( rscore_t score,
					Msg20 *msg20 );
	rscore_t rerankDmozCategoryNamesDontHaveQT ( rscore_t score, 
						 Msg20 *msg20 );
	rscore_t rerankDmozCategoryNamesDontHaveGigabits ( rscore_t score,
						       Msg20 *msg20 );
	inline
	rscore_t rerankDatedbDate( rscore_t score,
			       time_t datedbDate );
	inline
	rscore_t rerankProximity( rscore_t score,
			      float proximityScore ,
			      float maxScore);
	inline
	rscore_t rerankInSection( rscore_t score,
			      long summaryScore,
			      float maxScore);
	
	inline
	rscore_t rerankSubPhrase( rscore_t score,
			      float diversity,
			      float maxDiversity);

	bool attemptToCluster( );

private:
	Msg40 *m_msg40;
	SearchInput *m_si;

	bool m_enabled;
	long m_maxResultsToRerank;

	long     m_numToSort;
	M20List *m_m20List;
	long    *m_positionList;

	// Urls for pqrqttiu, pqrfsh and clustering
	Url *m_pageUrl;

	// for rerankNonLocationSpecificQueries
	//uint64_t                   m_querysLoc;
	//HashTableT<uint64_t, bool> m_ignoreLocs;

	// for rerankOtherPagesFromSameHost
        HashTableT<uint64_t, long> m_hostCntTable;

	// for rerankCommonTopicsInDmoz
	HashTableT<long, ComTopInDmozRec> m_dmozTable;

	// for rerankDatedbDate
	time_t m_now;

	char m_buf[PQR_BUF_SIZE];
	long  m_maxUrlLen;
	char *m_cvtUrl;

	long m_maxCommonInlinks;
};

#endif // _POSTQUERYRERANK_H_

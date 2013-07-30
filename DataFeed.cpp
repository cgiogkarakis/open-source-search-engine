#include "gb-include.h"

#include "DataFeed.h"

DataFeed::DataFeed() : MetaContainer() {
	m_customerId = -1;
	m_id = -1;
	m_passcodeLen = 0;
	m_passcode[0] = '\0';
	m_isLocked = false;
}

DataFeed::~DataFeed() {
}

void DataFeed::setUrl ( char *name,
			long  nameLen ) {
	if (!name || nameLen == 0)
		return;
	if (nameLen < 11 ||
	    strncasecmp(name, "datafeed://", 11) != 0) {
		char tempUrl[MAX_USERNAMELEN+1];
		setstr(tempUrl, MAX_USERNAMELEN-12, name, nameLen);
		m_urlLen = sprintf(m_url, "datafeed://%s/", tempUrl);
	}
	else
		m_urlLen = setstr(m_url, MAX_USERNAMELEN, name, nameLen);
	// base name
	long i;
	for (i = 0; m_url[i+11] != '/'; i++)
		m_baseName[i] = m_url[i+11];
	m_baseName[i] = '\0';
	m_baseNameLen = i;
}

void DataFeed::set ( long  creationTime,
		     char *dataFeedUrl,
		     long  dataFeedUrlLen,
		     char *passcode,
		     long  passcodeLen,
		     bool  isActive,
		     bool  isLocked ) {
	setUrl(dataFeedUrl, dataFeedUrlLen);
	m_passcodeLen = setstr(m_passcode, MAX_PASSCODELEN,
			       passcode, passcodeLen);
	// flags
	m_isActive = isActive;
	m_isLocked = isLocked;
	// creation time
	m_creationTime = creationTime;
}

void DataFeed::parse ( char *dataFeedPage,
		       long  dataFeedPageLen ) {
	// use Xml Class to parse up the page
	Xml xml;
	xml.set ( csUTF8, dataFeedPage, dataFeedPageLen, false, 0, false,
		  TITLEREC_CURRENT_VERSION );
	// get the nodes
	long numNodes  = xml.getNumNodes();
	XmlNode *nodes = xml.getNodes();
	// to count the tiers, result levels, and level costs
	long currTier = 0;
	long currResultLevel = 0;
	long currLevelCost = 0;
	// pull out the keywords for the data feed
	for (long i = 0; i < numNodes; i++) {
		// skip if this isn't a meta tag, shouldn't happen
		if (nodes[i].m_nodeId != 68)
			continue;
		// get the meta tag name
		//long tagLen;
		//char *tag = xml.getString(i, "name", &tagLen);
		long  ucTagLen;
		char *ucTag = xml.getString(i, "name", &ucTagLen);
		char tag[256];
		long tagLen = utf16ToLatin1 ( tag, 256,
					      (UChar*)ucTag, ucTagLen>>1 );
		// skip if empty
		if (!tag || tagLen <= 0)
			continue;
		// get the content
		long ucConLen;
		char *ucCon = xml.getString(i, "content", &ucConLen);
		char con[1024];
		long conLen = utf16ToLatin1 ( con, 1024,
					      (UChar*)ucCon, ucConLen>>1 );
		if (!con || conLen <= 0)
			continue;
		// match the meta tag to its local var and copy content
		if (tagLen == 10 && strncasecmp(tag, "customerid", 10) == 0)
			m_customerId = atoll(con);
		else if (tagLen == 11 && strncasecmp(tag, "datafeedurl", 11) == 0)
			setUrl(con, conLen);
		else if (tagLen == 8 && strncasecmp(tag, "passcode", 8) == 0)
			m_passcodeLen = setstr(m_passcode, MAX_PASSCODELEN, con, conLen);
		else if (tagLen == 6 && strncasecmp(tag, "status", 6) == 0)
			m_isActive = (bool)atoi(con);
		else if (tagLen == 6 && strncasecmp(tag, "locked", 6) == 0)
			m_isLocked = (bool)atoi(con);
		else if (tagLen == 14 && 
			 strncasecmp(tag, "dfcreationtime", 14) == 0)
			m_creationTime = atol(con);
		else if (tagLen == 8 && strncasecmp(tag, "numtiers", 8) == 0)
			m_priceTable.m_numTiers = atol(con);
		else if (tagLen == 15 && strncasecmp(tag, "numresultlevels", 15) == 0)
			m_priceTable.m_numResultLevels = atol(con);
		else if (tagLen == 10 && strncasecmp(tag, "monthlyfee", 10) == 0)
			m_priceTable.m_monthlyFee = atol(con);
		else if (tagLen == 7 && strncasecmp(tag, "tiermax", 7) == 0) {
			m_priceTable.m_tierMax[currTier] = (unsigned long)atol(con);
			currTier++;
		}
		else if (tagLen == 11 && strncasecmp(tag, "resultlevel", 11) == 0) {
			m_priceTable.m_resultLevels[currResultLevel] = (unsigned long)atol(con);
			currResultLevel++;
		}
		else if (tagLen == 9 && strncasecmp(tag, "levelcost", 9) == 0) {
			m_priceTable.m_levelCosts[currLevelCost] = (unsigned long)atol(con);
			currLevelCost++;
		}
		else
			log(LOG_INFO, "datafeed: Invalid Meta Tag Parsed [%li]:"
			    " %s", tagLen, tag);
	}
}

long DataFeed::buildPage ( char *page ) {
	// fill the page buffer with the data feed page
	char *p = page;
	p += sprintf(p, "<meta name=customerid content=\"%lli\">\n"
			"<meta name=datafeedurl content=\"%s\">\n"
			"<meta name=passcode content=\"%s\">\n"
			"<meta name=status content=\"%d\">\n"
			"<meta name=locked content=\"%d\">\n"
			"<meta name=dfcreationtime content=\"%li\">\n",
			m_customerId,
			m_url,
			m_passcode,
			m_isActive,
			m_isLocked,
			m_creationTime );
	// write the pricetable
	p += sprintf(p, "<meta name=numtiers content=\"%li\">\n"
			"<meta name=numresultlevels content=\"%li\">\n"
			"<meta name=monthlyfee content=\"%li\">\n",
			m_priceTable.m_numTiers,
			m_priceTable.m_numResultLevels,
			m_priceTable.m_monthlyFee );
	// write the tiers
	for (long i = 0; i < m_priceTable.m_numTiers; i++)
		p += sprintf(p, "<meta name=tiermax content=\"%lu\">\n",
				m_priceTable.m_tierMax[i] );
	// write the result levels
	for (long i = 0; i < m_priceTable.m_numResultLevels; i++)
		p += sprintf(p, "<meta name=resultlevel content=\"%lu\">\n",
				m_priceTable.m_resultLevels[i] );
	// write the costs
	long numCosts = m_priceTable.m_numTiers * m_priceTable.m_numResultLevels * 2;
	for (long i = 0; i < numCosts; i++)
		p += sprintf(p, "<meta name=levelcost content=\"%lu\">\n",
				m_priceTable.m_levelCosts[i] );
	// return the length
	return (p - page);
}

void DataFeed::buildPage ( SafeBuf *sb ) {
	sb->safePrintf("<meta name=customerid content=\"%lli\">\n"
		       "<meta name=datafeedurl content=\"%s\">\n"
		       "<meta name=passcode content=\"%s\">\n"
		       "<meta name=status content=\"%d\">\n"
		       "<meta name=locked content=\"%d\">\n"
		       "<meta name=dfcreationtime content=\"%li\">\n",
		       m_customerId,
		       m_url,
		       m_passcode,
		       m_isActive,
		       m_isLocked,
		       m_creationTime );
	// write the pricetable
	sb->safePrintf("<meta name=numtiers content=\"%li\">\n"
		       "<meta name=numresultlevels content=\"%li\">\n"
		       "<meta name=monthlyfee content=\"%li\">\n",
		       m_priceTable.m_numTiers,
		       m_priceTable.m_numResultLevels,
		       m_priceTable.m_monthlyFee );
	// write the tiers
	for (long i = 0; i < m_priceTable.m_numTiers; i++)
		sb->safePrintf("<meta name=tiermax content=\"%lu\">\n",
			       m_priceTable.m_tierMax[i] );
	// write the result levels
	for (long i = 0; i < m_priceTable.m_numResultLevels; i++)
		sb->safePrintf("<meta name=resultlevel content=\"%lu\">\n",
			       m_priceTable.m_resultLevels[i] );
	// write the costs
	long numCosts = m_priceTable.m_numTiers*m_priceTable.m_numResultLevels*2;
	for (long i = 0; i < numCosts; i++)
		sb->safePrintf("<meta name=levelcost content=\"%lu\">\n",
			       m_priceTable.m_levelCosts[i] );
}

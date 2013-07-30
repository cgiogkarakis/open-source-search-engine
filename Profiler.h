
#define MAX_TOPICS_PER_TERM 28
#define MAX_ALLOWED_TOPICS 100
#define EI_NIDENT 16
#ifndef _PROFILER_H_
#define _PROFILER_H_
#define SHT_SYMTAB 2
#define SHT_DYNSYM 11

#include "TcpServer.h"
#include "HttpRequest.h"
#include "Parms.h"
#include "SafeBuf.h"
#include "Pages.h"
//#include "HashTableT.h"
#include "HashTableX.h"

typedef struct elf_internal_ehdr {
	unsigned char	e_ident[EI_NIDENT];   	/* ELF "magic number" */
	unsigned long	e_entry;	/* Entry point virtual address */
	unsigned long	e_phoff;	/* Program header table file offset */
	unsigned long   e_shoff;     	/* Section header table file offset */
	unsigned long   e_version;     	/* Identifies object file version */
	unsigned long	e_flags;       	/* Processor-specific flags */
	unsigned short	e_type;	       	/* Identifies object file type */
	unsigned short	e_machine;     	/* Specifies required architecture */
	unsigned short	e_ehsize;      	/* ELF header size in bytes */
	unsigned short	e_phentsize;   	/* Program header table entry size */
	unsigned short	e_phnum;       	/* Program header table entry count */
	unsigned short	e_shentsize;   	/* Section header table entry size */
	unsigned short	e_shnum;       	/* Section header table entry count */
	unsigned short	e_shstrndx;    	/* Section header string table index */
} Elf_Internal_Ehdr;


typedef struct {
	unsigned char	e_ident[16];   	/* ELF "magic number" */
	unsigned char	e_type[2];     	/* Identifies object file type */
	unsigned char	e_machine[2];  	/* Specifies required architecture */
	unsigned char	e_version[4];  	/* Identifies object file version */
	unsigned char	e_entry[4];    	/* Entry point virtual address */
	unsigned char	e_phoff[4];    	/* Program header table file offset */
	unsigned char	e_shoff[4];    	/* Section header table file offset */
	unsigned char	e_flags[4];    	/* Processor-specific flags */
	unsigned char	e_ehsize[2];   	/* ELF header size in bytes */
	unsigned char	e_phentsize[2];	/* Program header table entry size */
	unsigned char	e_phnum[2];    	/* Program header table entry count */
	unsigned char	e_shentsize[2];	/* Section header table entry size */
	unsigned char	e_shnum[2];    	/* Section header table entry count */
	unsigned char	e_shstrndx[2]; 	/* Section header string table index */
} Elf32_External_Ehdr;

typedef struct elf_internal_shdr {
	unsigned int	sh_name;       	/* Section name, index in string tbl */
	unsigned int	sh_type;       	/* Type of section */
	unsigned long	sh_flags;      	/* Miscellaneous section attributes */
	unsigned long    sh_addr;      	/* Section virtual addr at execution */
	unsigned long	sh_size;       	/* Size of section in bytes */
	unsigned long	sh_entsize;    	/* Entry size if section holds table */
	unsigned long	sh_link;       	/* Index of another section */
	unsigned long	sh_info;       	/* Additional section information */
	long int	sh_offset;     	/* Section file offset */
	unsigned int	sh_addralign;  	/* Section alignment */
	
	/* The internal rep also has some cached info associated with it. 
	   asection *	bfd_section;		Associated BFD section. 
	   PTR		contents;		Section contents.  */
} Elf_Internal_Shdr;

typedef struct {
	unsigned char	sh_name[4];    	/* Section name, index in string tbl */
	unsigned char	sh_type[4];    	/* Type of section */
	unsigned char	sh_flags[4];   	/* Miscellaneous section attributes */
	unsigned char	sh_addr[4];    	/* Section virtual addr at execution */
	unsigned char	sh_offset[4];  	/* Section file offset */
	unsigned char	sh_size[4];    	/* Size of section in bytes */
	unsigned char	sh_link[4];    	/* Index of another section */
	unsigned char	sh_info[4];    	/* Additional section information */
	unsigned char	sh_addralign[4];/* Section alignment */
	unsigned char	sh_entsize[4]; 	/* Entry size if section holds table */
} Elf32_External_Shdr;

typedef struct elf_internal_sym {
	unsigned long st_value;        	/* Value of the symbol */
	unsigned long st_size;	        /* Associated symbol size */
	unsigned long	st_name;       	/* Symbol name, index in string tbl */
	unsigned char	st_info;       	/* Type and binding attributes */
	unsigned char	st_other;      	/* No defined meaning, 0 */
	unsigned short st_shndx;       	/* Associated section index */
} Elf_Internal_Sym;

typedef struct {
	unsigned char	st_name[4];    	/* Symbol name, index in string tbl */
	unsigned char	st_value[4];   	/* Value of the symbol */
	unsigned char	st_size[4];    	/* Associated symbol size */
	unsigned char	st_info[1];    	/* Type and binding attributes */
	unsigned char	st_other[1];   	/* No defined meaning, 0 */
	unsigned char	st_shndx[2];   	/* Associated section index */
} Elf32_External_Sym;

struct FnInfo {
	char m_fnName[256];
	unsigned long m_timesCalled;
	unsigned long m_totalTimeTaken;
	unsigned long m_maxTimeTaken;
	unsigned long m_numCalledFromThread;
	unsigned long m_maxBlockedTime;
	const char   *m_lastQpoll; 
	const char   *m_prevQpoll; 
	

	long long     m_startTimeLocal;
	long long     m_startTime;
	long long     m_lastPauseTime;
	long          m_inFunction;
};


struct QuickPollInfo {
	const char *m_caller;
	long        m_lineno;
	const char *m_last;
	long        m_lastlineno;
	long        m_timeAcc;
	long        m_maxTime;
	long        m_times;
};

struct HitEntry {
	char *funcName;
	char *file;
	uint32_t fileHash;
	uint32_t line;
	uint32_t numHits;
	uint32_t numHitsPerFunc;
	uint32_t address;
	uint32_t baseAddress;
	uint32_t missedQuickPolls;
	uint32_t missedQuickPollsPerFunc;
};

class FrameTrace {
	public:
		FrameTrace();
		~FrameTrace();
		FrameTrace *set(const uint32_t addr);
		FrameTrace *add(const uint32_t baseAddr);
		void dump(	SafeBuf *out,
				const uint32_t level = 0,
				uint32_t printStart = 0) const;
		uint32_t getPrintLen(const uint32_t level = 0) const;
		uint32_t address;
		static const uint16_t MAX_CHILDREN = 64;
		FrameTrace *m_children[MAX_CHILDREN];
		uint32_t m_numChildren;
		uint32_t hits;
		uint32_t missQuickPoll;
};

class Profiler {
 public:
	Profiler();
	~Profiler();

	bool reset();
	bool init();

	bool addQuickPoll(long long took, const char* caller);
	bool startTimer(long address, const char* caller);
	bool pause(const char *caller, long lineno, long took);
	bool unpause();

	//This function should get the function name from nm / addr2line
	bool endTimer(long address, 
		      const char *caller,
		      bool isThread=false);

	bool printInfo(SafeBuf *sb,
		       //long user, 
		       char *username,
		       char *pwd, 
		       char *coll, 
		       int sorts,
		       int sort10,
		       int qpreset,
		       int profilerreset);
	bool printRealTimeInfo(SafeBuf *sb,
			       //long user,
			       char *username,
			       char *pwd,
			       char *coll,
			       int realTimeSortMode,
			       int realTimeShowAll);
	void resetLastQpoll() {m_lastQpoll = "main loop"; m_lastQpollLine =0;}

	char *getFnName(unsigned long address,long *nameLen=NULL);

	//Functions for reading the symbol table of gb,
	//which is a 32-bit ELF file. These functions are 
	//a customized version of readelf program, found in
	//GNU binutils, and works for 32-bit ELF files.
	bool readSymbolTable();
	static FrameTrace *getFrameTrace( 	void **trace,
						const uint32_t numFrames);

	static FrameTrace *updateRealTimeData( 	void **trace,
						const uint32_t numFrames,
						uint32_t **ptr);

	static void checkMissedQuickPoll( 	FrameTrace *frame,
						const uint32_t stackPtr,
						uint32_t *ptr);

	static void getStackFrame(int sig);

	void startRealTimeProfiler();

	void stopRealTimeProfiler(const bool keepData=false);

	void cleanup();
	
	FrameTrace *getNewFrameTrace(const uint32_t addr);

	bool m_realTimeProfilerRunning;

protected:
	bool processSymbolTable (FILE *file);

	bool processSectionHeaders (FILE * file);

	bool get32bitSectionHeaders (FILE * file);

	bool getFileHeader (FILE * file);

	//bool fnReset (HashTableT<unsigned long, FnInfo*> *m_fn);
	
	FILE *m_file;
	char *m_stringTable;
	long m_stringTableSize;

	Elf_Internal_Ehdr       m_elfHeader;
	Elf_Internal_Shdr*     m_sectionHeaders;

	Elf_Internal_Sym* get32bitElfSymbols (FILE *file,
					      unsigned long offset,
					      unsigned long number);

	unsigned long getByte (unsigned char * field,int size);

	//HashTableT<unsigned long, FnInfo> m_fn;
	//HashTableT<unsigned long, FnInfo> m_fnTmp[11];
	//HashTableT<unsigned long, FnInfo*> m_activeFns;
	//HashTableT<unsigned long, QuickPollInfo*> m_quickpolls;

	HashTableX m_fn;
	HashTableX m_fnTmp[11];
	HashTableX m_activeFns;
	HashTableX m_quickpolls;

	const char* m_lastQpoll;
	long m_lastQpollLine;
	QuickPollInfo m_quickPollInfos[512];
	long          m_lastQPUsed;
	
	unsigned long long m_fnTime[11];
private:
	// Realtime profiler stuff
	uint32_t getFuncBaseAddr(const uint32_t address);
	uint32_t getFuncBaseAddr(const char *funcName);
	static int rtDataLocationCmp(const void *A, const void *B);
	static int rtHitCmp(const void *A, const void *B);
	static int rtFuncHitCmp(const void *A, const void *B);
	static int rtAddressCmp(const void *A, const void *B);
	static int rtMissedQuickPolls(const void *A, const void *B);
	static int rtMissedQuickPollPerFunc(const void *A, const void *B);
	void sortRealTimeData(const uint8_t type, const uint32_t num);
	uint32_t rtNumEntries;
	HitEntry *hitEntries;
	uint32_t *m_addressMap;
	uint32_t m_addressMapSize;
	uint32_t m_lastAddressMapIndex;
	FrameTrace *m_rootFrame;
	uint32_t m_lastDeltaAddress;
	uint64_t m_lastDeltaAddressTime;
	static const uint32_t MAX_FRAME_TRACES = 1024 * 64;
	FrameTrace *m_frameTraces;
	uint32_t m_numUsedFrameTraces;
};

extern Profiler g_profiler;


#endif

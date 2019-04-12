#ifdef _BREGEXP_
#define BREGEXPAPI __declspec(dllexport) 
#else
#define BREGEXPAPI __declspec(dllimport) 
#endif

typedef struct bregexp {
	const char *outp;			/* result string start ptr   */
	const char *outendp;		/* result string end ptr     */ 
	const int  splitctr;		/* split result counter     */ 
	const char **splitp;		/* split result pointer ptr     */ 
	int     rsv1;				/* reserved for external use    */ 
} BREGEXP;

#if defined(__cplusplus)
extern "C"
{
#endif

BREGEXPAPI
int BMatch(char* str,char *target,char *targetendp,
								BREGEXP **rxp,char *msg) ;
BREGEXPAPI
int BSubst(char* str,char *target,char *targetendp,
								BREGEXP **rxp,char *msg) ;
BREGEXPAPI
int BTrans(char* str,char *target,char *targetendp,
								BREGEXP **rxp,char *msg) ;
BREGEXPAPI
int BSplit(char* str,char *target,char *targetendp,
						int limit,BREGEXP **rxp,char *msg);
BREGEXPAPI
void BRegfree(BREGEXP* rx);

BREGEXPAPI
char* BRegexpVersion(void);

#if defined(__cplusplus)
}
#endif


#undef BREGEXPAPI

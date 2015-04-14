#if FALSE
/************************************************************************/
/*	c8type.h		Prototypes for safe implementations of "ctype" functions	*/
/************************************************************************/

int __far isalnum(int);
int __far isalpha(int);
int __far isascii(int);
int __far iscntrl(int);
int __far isdigit(int);
int __far isgraph(int);
int __far islower(int);
int __far isprint(int);
int __far ispunct(int);
int __far isspace(int);
int __far isupper(int);
int __far isxdigit(int);
int __far tolower(int);
int __far toupper(int);

/* END */
#else
#include <ctype.h>
#endif

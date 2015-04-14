typedef unsigned long	UCS4;
typedef unsigned char	UTF8;
#define MAXUTF8BYTES 6

#ifdef __cplusplus
extern "C"
{
#endif
    int UCS4toUTF8(UCS4 ch, UTF8 * pszUTF8);
    int UTF8AdditionalBytes(UTF8 InitialCharacter);
#ifdef __cplusplus
};   // extern "C"
#endif

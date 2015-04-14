#include <stdio.h>
#include <string.h>
#include "utf8.h"

#define UTF8SIZE 6

int UCS4toUTF8(UCS4 c, UTF8 * pszUTF8)
{
    int count;
    UTF8 UTF8Buffer[UTF8SIZE];

    count= 1;

    if (c <= 0x7F)
        pszUTF8[0]= (UTF8) c;
    else
    {
        UCS4 prefix= 0xC0;

        do
        {
            UTF8Buffer[UTF8SIZE - count]= (UTF8)(0x80 | (c & 0x3F));
            c= c >> 6;
            count++;
            prefix= (prefix >> 1) | 0x80;
        } while (c  > ((~prefix) & 0xFF) );

        UTF8Buffer[UTF8SIZE - count]= (UTF8)(c | (prefix << 1));
        memcpy(pszUTF8, &UTF8Buffer[UTF8SIZE - count], count);
    }

    return count;
}

char bytesFromUTF8[256] = {
                              0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                              0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                              1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                              2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5};

int UTF8AdditionalBytes(UTF8 InitialCharacter)
{
    return bytesFromUTF8[InitialCharacter];
}

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "mystring.h"

char * strReverse(char *srcStr, char* dstStr)
{
    int i, length;
    if (srcStr == NULL || dstStr == NULL)
            return NULL;

    length = strlen(srcStr);
    for ( i =0; i < length; i++)
            dstStr[i] = srcStr[length - i - 1 ];
    dstStr[length] = '\0';

    return dstStr;
}

char * strtrim(char *input)
{
    char * start, *end;

    if (input ==NULL)
        return NULL;
    start=input;
            
    while(isspace(*start)) start++;
     // Trim trailing space
     end = input + strlen(input) - 1;

     while(end > start && isspace(*end)) end--;
     end++;
     *end = '\0';

     return start;
}

size_t strtrim2(char *out, size_t len, const char *src)
{
      if(len == 0 || src == NULL)
              return 0;

        const char *end;
        size_t out_size;

         // Trim leading space
         while(isspace(*src)) src++;
           
         if(*src == 0)  // All spaces?
         {
            *out = 0;
            return 1;
         }
         
         // Trim trailing space
         end = src + strlen(src) - 1;
         while(end > src && isspace(*end)) end--;
         end++;
        
          // Set output size to minimum of trimmed srcing length and buffer size minus 1
          out_size = (end - src) < len-1 ? (end - src) : len-1;
          
           // Copy trimmed srcing and add null terminator
           memcpy(out, src, out_size);
           out[out_size] = 0;
           
           return out_size;
}

/*
int main( )
{
    char input[]=" \t ab cd ef\t gh \t\r\n";
    char *str;

    printf("Input:%s, length=%d\n", input, strlen(input));
    str=strtrim(input);
    printf("After strtrim: %s, length=%d\n", str, strlen(str));
    printf("After strtrim, input= %s, length=%d\n", input, strlen(input));

}
*/

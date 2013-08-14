#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "testmutablestring.h"
#include "mutablestring.h"

void testMutableString()
{    
    //single large appends
    {
        skMutableString s;

        const int numCases = 6;
        const int lengths[numCases] =
        {
            SK_MUTABLE_STRING_STATIC_SIZE - 2,
            SK_MUTABLE_STRING_STATIC_SIZE - 1,
            SK_MUTABLE_STRING_STATIC_SIZE,
            SK_MUTABLE_STRING_STATIC_SIZE + 1,
            SK_MUTABLE_STRING_STATIC_SIZE + 2,
            512
        };
        
        for (int i = 0; i < numCases; i++)
        {
            skMutableString_init(&s);
            
            const int len = lengths[i];

            char* toAppend = (char*)malloc(len + 1);;
            for (int i = 0; i <= len; i++)
            {
                toAppend[i] = 'x';
            }
            toAppend[len] = '\0';
            
            skMutableString_append(&s, toAppend);
            free(toAppend);
            assert(strlen(skMutableString_getString(&s)) == len);
            assert(s.charCount == len);
            
            if (len <= SK_MUTABLE_STRING_STATIC_SIZE)
            {
                assert(s.dynamicData == 0);
            }
            else
            {
                assert(s.dynamicData != 0);
            }
            
            skMutableString_deinit(&s);
        }
    }
    
    //multiple appends
    {
        const int len = 1024;
        char refString[len + 1];
        char refString1[len + 1];
        for (int i = 0; i < len; i++)
        {
            refString[i] = 65 + (i % 26);
            refString1[i] = 65 + (i % 26);
        }
        
        refString[len] = '\0';
        refString1[len] = '\0';
        
        const int numCases = 6;
        const int chunkSizes[numCases] =
        {
            1,
            2,
            SK_MUTABLE_STRING_STATIC_SIZE - 1,
            SK_MUTABLE_STRING_STATIC_SIZE,
            SK_MUTABLE_STRING_STATIC_SIZE + 1,
            SK_MUTABLE_STRING_STATIC_SIZE * 2,
        };
        
        skMutableString s;
        
        for (int i = 0; i < numCases; i++)
        {
            skMutableString_init(&s);
            int charsWritten = 0;
            const int chunkSize = chunkSizes[i];
            
            while (charsWritten < len)
            {
                const int charsLeft = len - charsWritten;
                const int currChunkSize = charsLeft < chunkSize ? charsLeft : chunkSize;
                char temp = refString[charsWritten + currChunkSize];
                refString[charsWritten + currChunkSize] = '\0';
                skMutableString_append(&s, &refString[charsWritten]);
                refString[charsWritten + currChunkSize] = temp;
                
                charsWritten += currChunkSize;
            }
            
            const char* str = skMutableString_getString(&s);
            
            assert(strlen(str) == len);
            assert(strlen(str) == strlen(refString));
            assert(strlen(str) == strlen(refString1));
            
            for (int i = 0; i < len; i++)
            {
                assert(str[i] == refString[i]);
                assert(refString[i] == refString1[i]);
            }
            
            skMutableString_deinit(&s);
        }
        
    }
    
}


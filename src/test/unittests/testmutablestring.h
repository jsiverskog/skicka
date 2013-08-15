#ifndef SK_TEST_MUTABLE_STRING
#define SK_TEST_MUTABLE_STRING

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../skicka/mutablestring.h"

static void testMutableStringSingleAppend()
{
    
    //single large appends
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
        
        sput_fail_unless(s.charCount == len,
                         "charCount mismatch");
        sput_fail_unless(strlen(skMutableString_getString(&s)) == len,
                         "strlen mismatch");
        
        if (len <= SK_MUTABLE_STRING_STATIC_SIZE)
        {
            sput_fail_unless(s.dynamicData == 0,
                             "Statically allocated string should have null dynamic data");
        }
        else
        {
            sput_fail_unless(s.dynamicData != 0,
                             "Dynamically allocated string should have null static data");
        }
        
        skMutableString_deinit(&s);
    }
}

static void testMutableStringMutipleAppend()
{
    //multiple appends
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
        
        sput_fail_unless(strlen(str) == len, "Mutable string length mismatch");
        sput_fail_unless(strlen(str) == strlen(refString), "Mutable string length mismatch");
        sput_fail_unless(strlen(str) == strlen(refString1), "Mutable string length mismatch");
        
        for (int i = 0; i < len; i++)
        {
            if (str[i] != refString[i])
            {
                sput_fail_unless(0, "Mutable string does not match reference string");
                break;
            }
            if (refString[i] != refString1[i])
            {
                sput_fail_unless(0, "Reference string mismatch");
                break;
            }
        }
        
        skMutableString_deinit(&s);
    }
    
}

#endif //SK_TEST_MUTABLE_STRING
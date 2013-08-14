#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "testmutablestring.h"
#include "mutablestring.h"

void testMutableString()
{
    //static only
    {
        skMutableString s;
        skMutableString_init(&s);
        
        for (int i = 0; i < 5; i++)
        {
            skMutableString_append(&s, "abc");
            
            assert(strlen(skMutableString_getString(&s)) == 3 * (i + 1));
            assert(s.charCount == 3 * (i + 1));
            assert(s.dynamicData == 0);
        }
        
        skMutableString_deinit(&s);
    }
    
    //dynamic (one large append)
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
    
    //dynamic (multiple small appends)
    {

    }
    
}


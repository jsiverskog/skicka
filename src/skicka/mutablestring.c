#include <assert.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include "mutablestring.h"

void skMutableString_init(skMutableString* ms)
{
    memset(ms, 0, sizeof(skMutableString));
}

void skMutableString_deinit(skMutableString* ms)
{
    if (ms->dynamicData)
    {
        free(ms->dynamicData);
    }
    
    memset(ms, 0, sizeof(skMutableString));
}

void skMutableString_append(skMutableString* ms, const char* toAppend)
{
    skMutableString_appendBytes(ms, toAppend, (int)strlen(toAppend));
}

void skMutableString_appendInt(skMutableString* ms, int toAppend)
{
    char temp[64];
    sprintf(temp, "%d", toAppend);
    skMutableString_append(ms, temp);
}

void skMutableString_appendBytes(skMutableString* ms, const char* toAppend, int numBytes)
{
    if (!toAppend)
    {
        return;
    }
    
    const size_t newCharCount = numBytes + ms->charCount;
    
    if (newCharCount > SK_MUTABLE_STRING_STATIC_SIZE)
    {
        //too big to be static. copy the data
        if (ms->charCount <= SK_MUTABLE_STRING_STATIC_SIZE)
        {
            assert(ms->dynamicData == 0);
        }
        
        ms->dynamicData = realloc(ms->dynamicData, newCharCount + 1);
        
        if (ms->charCount <= SK_MUTABLE_STRING_STATIC_SIZE)
        {
            memcpy(ms->dynamicData, ms->staticData, ms->charCount);
            ms->dynamicData[ms->charCount] = '\0';
            memset(ms->staticData, 0, SK_MUTABLE_STRING_STATIC_SIZE);
        }
    }
    
    char* targetData = newCharCount > SK_MUTABLE_STRING_STATIC_SIZE ? ms->dynamicData : ms->staticData;
    
    memcpy(&targetData[ms->charCount], toAppend, numBytes);
    ms->charCount = newCharCount;
    targetData[ms->charCount] = '\0';
}

const char* skMutableString_getString(skMutableString* ms)
{
    return ms->dynamicData != 0 ? ms->dynamicData : ms->staticData;
}

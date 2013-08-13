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
    if (ms->data)
    {
        free(ms->data);
    }
    
    memset(ms, 0, sizeof(skMutableString));
}

void skMutableString_append(skMutableString* ms, const char* toAppend)
{
    if (!toAppend)
    {
        return;
    }
    
    const size_t newCharCount = strlen(toAppend) + ms->charCount;
    ms->data = realloc(ms->data, newCharCount + 1);
    memcpy(&ms->data[ms->charCount], toAppend, strlen(toAppend));
    ms->charCount = newCharCount;
    ms->data[ms->charCount] = '\0';
}

const char* skMutableString_getString(skMutableString* ms)
{
    return ms->data;
}
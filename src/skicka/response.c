#include <stdlib.h>
#include <string.h>
#include "response.h"

void skResponse_init(skResponse* response)
{
    memset(response, 0, sizeof(skResponse));
}


void skResponse_deinit(skResponse* response)
{
    if (response->bytes && response->size > 0)
    {
        free(response->bytes);
    }
    
    memset(response, 0, sizeof(skResponse));
}


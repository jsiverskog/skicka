#include <assert.h>
#include "testrequest.h"
#include "request.h"

void testGET()
{
    skRequest r;
    skRequest_init(&r);
    skRequest_setURL(&r, "http://www.example.com");
    
    skRequest_send(&r, 0);
    const skResponse* resp = &r.response;
    assert(resp->httpStatusCode == 200);
    
    skRequest_deinit(&r);
}

void testGETAsync()
{
    skRequest r;
    skRequest_init(&r);
    skRequest_setURL(&r, "http://www.example.com");
    
    skRequest_send(&r, 1);
    
    while (skRequest_getState(&r) == SK_IN_PROGRESS)
    {
        //wait until the request has finished
    }
    
    const skResponse* resp = &r.response;
    assert(resp->httpStatusCode == 200);
    
    skRequest_deinit(&r);
}


void testPOST()
{
    
}
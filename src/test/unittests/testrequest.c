#include <assert.h>
#include "testrequest.h"
#include "request.h"

static void responseCallback(skRequest* request, skResponse* response)
{
    int* done = (int*)request->userData;
    *done = 1;
}

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
    
    int done = 0;
    r.userData = &done;
    r.responseCallback = responseCallback;
    
    while (!done)
    {
        skRequest_poll(&r);
    }
    
    const skResponse* resp = &r.response;
    assert(resp->httpStatusCode == 200);
    
    skRequest_deinit(&r);
}


void testPOST()
{
    
}
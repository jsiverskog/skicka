#ifndef SK_TEST_REQUEST_H
#define SK_TEST_REQUEST_H

#include "sput.h"
#include "../../skicka/request.h"

static void responseCallback(skRequest* request, skResponse* response)
{
    int* done = (int*)request->userData;
    *done = 1;
}

static void testGET()
{
    skRequest r;
    skRequest_init(&r);
    skRequest_setURL(&r, "http://www.example.com");
    
    skRequest_send(&r, 0);
    const skResponse* resp = &r.response;
    sput_fail_unless(resp->httpStatusCode == 200, "Wrong response status code");
    
    skRequest_deinit(&r);
}

static void testGETAsync()
{
    //test polling while an async request is in progress
    {
        skRequest r;
        skRequest_init(&r);
        skRequest_setURL(&r, "http://www.example.com");
        
        int done = 0;
        r.userData = &done;
        r.responseCallback = responseCallback;
        
        skRequest_send(&r, 1);
        
        while (!done)
        {
            skRequest_poll(&r);
        }
        
        const skResponse* resp = &r.response;
        assert(resp->httpStatusCode == 200);
        
        skRequest_deinit(&r);
    }
    
    //test polling after an async request has been completed
    {
        skRequest r;
        skRequest_init(&r);
        skRequest_setURL(&r, "http://www.example.com");
        
        int done = 0;
        r.userData = &done;
        r.responseCallback = responseCallback;
        
        skRequest_send(&r, 1);
        
        while (skRequest_getState(&r) == SK_IN_PROGRESS)
        {
            //wait...
        }
        
        while (!done)
        {
            skRequest_poll(&r);
        }
        
        const skResponse* resp = &r.response;
        sput_fail_unless(resp->httpStatusCode == 200, "Wrong response status code");
        
        skRequest_deinit(&r);
    }
}


static void testPOST()
{
    
}

#endif //SK_TEST_REQUEST_H


#ifndef SK_TEST_CLIENT_H
#define SK_TEST_CLIENT_H

#include "sput.h"
#include "../../skicka/restclient.h"

static void testClientRequestCount()
{
    skRESTClient c;
    skRESTClient_init(&c, "http://www.example.com");
    
    for (int i = 0; i < 5; i++)
    {
        skRequest* r = skRESTClient_getRequestFromPool(&c);
        skRESTClient_sendRequest(&c, r, "/", NULL, NULL, 1);
        const int numActiveRequests = skRESTClient_getNumActiveRequests(&c);
        sput_fail_unless(numActiveRequests == i + 1, "Active request count check");
    }
    
    skRESTClient_cancelAllRequests(&c, 1);
    
    skRESTClient_deinit(&c);
}

static void testClientRequestPoolSize()
{
    skRESTClient c;
    skRESTClient_init(&c, "http://www.example.com");
    
    skRequest* r = NULL;
    do
    {
        r = skRESTClient_getRequestFromPool(&c);
        skRESTClient_sendRequest(&c, r, "/", NULL, NULL, 1);
    }
    while (r != NULL);
    
    skRESTClient_waitForAllRequestsToFinish(&c);
    
    skRESTClient_deinit(&c);
}

#endif //SK_TEST_CLIENT_H


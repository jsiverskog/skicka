#ifndef SK_TEST_CLIENT_H
#define SK_TEST_CLIENT_H

#include "sput.h"
#include "../../skicka/restclient.h"


static void resendErrorCallback(struct skRequest* request, skError errorCode)
{
    sput_fail_if(1, "Error callback should not be called during send from callback test");
}

static void resendResponseCallback3(struct skRequest* request, skResponse* response)
{
    printf("final response\n");
    sput_fail_unless(response->httpStatusCode < 300 &&
                     response->httpStatusCode >= 200,
                     "Send from callback should give a 2xx response status code");
}

static void resendResponseCallback2(struct skRequest* request, skResponse* response)
{
    sput_fail_unless(response->httpStatusCode < 300 &&
                     response->httpStatusCode >= 200,
                     "Send from callback should give a 2xx response status code");
    printf("resending 2\n");
    skRESTClient* c = (skRESTClient*)request->userData[0];
    skRESTClient_setRequestManualDeinit(c, request, 1);
    skRequest* r = skRESTClient_getRequestFromPool(c);
    skRESTClient_setRequestManualDeinit(c, request, 0);
    r->userData[0] = c;
    r->responseCallback = resendResponseCallback3;
    r->errorCallback = resendErrorCallback;
    skRESTClient_sendRequest(c, r, "/", 1);
}

static void resendResponseCallback1(struct skRequest* request, skResponse* response)
{
    sput_fail_unless(response->httpStatusCode < 300 &&
                     response->httpStatusCode >= 200,
                     "Send from callback should give a 2xx response status code");
    printf("resending 1\n");
    skRESTClient* c = (skRESTClient*)request->userData[0];
    skRESTClient_setRequestManualDeinit(c, request, 1);
    skRequest* r = skRESTClient_getRequestFromPool(c);
    skRESTClient_setRequestManualDeinit(c, request, 0);
    r->userData[0] = c;
    r->responseCallback = resendResponseCallback2;
    r->errorCallback = resendErrorCallback;
    skRESTClient_sendRequest(c, r, "/", 1);
}

static void testClientResendRequestFromCallback()
{
    //test sending requests from within response callbacks
    skRESTClient c;
    skRESTClient_init(&c, "http://www.example.com");
    
    skRequest* r = skRESTClient_getRequestFromPool(&c);
    r->userData[0] = &c;
    r->responseCallback = resendResponseCallback1;
    r->errorCallback = resendErrorCallback;
    skRESTClient_sendRequest(&c, r, "/", 1);
    
    r = skRESTClient_getRequestFromPool(&c);
    r->userData[0] = &c;
    skRESTClient_sendRequest(&c, r, "/", 1);
    
    skRESTClient_waitForAllRequestsToFinish(&c);
    
    sput_fail_unless(skRESTClient_getNumActiveRequests(&c) == 0,
                     "There should be 0 acive requests after call to skRESTClient_waitForAllRequestsToFinish");
    
    skRESTClient_deinit(&c);
}

static void testClientRequestCount()
{
    skRESTClient c;
    skRESTClient_init(&c, "http://www.example.com");
    
    for (int i = 0; i < 5; i++)
    {
        skRequest* r = skRESTClient_getRequestFromPool(&c);
        skRESTClient_sendRequest(&c, r, "/", 1);
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
        skRESTClient_sendRequest(&c, r, "/", 1);
    }
    while (r != NULL);
    
    skRESTClient_waitForAllRequestsToFinish(&c);
    
    skRESTClient_deinit(&c);
}

#endif //SK_TEST_CLIENT_H


#ifndef SK_TEST_REQUEST_H
#define SK_TEST_REQUEST_H

#include "sput.h"
#include "../../skicka/request.h"


static const int ERROR_CALLBACK_CALLED = 1 << 0;
static const int RESPONSE_CALLBACK_CALLED = 1 << 1;
static const int ERROR_CALLBACK_CALLED_WITH_CANCEL = 1 << 2;
static const int ERROR_CALLBACK_CALLED_WITH_ERROR = 1 << 3;

static void errorCallback(skRequest* request, skError error)
{
    sput_fail_unless(error != SK_NO_ERROR, "Error callback error code should not be SK_NO_ERROR");
    
    int* flags = (int*)request->userData;
    *flags |= ERROR_CALLBACK_CALLED;
    
    if (error == SK_REQUEST_CANCELLED)
    {
        *flags |= ERROR_CALLBACK_CALLED_WITH_CANCEL;
    }
    else if (error != SK_NO_ERROR)
    {
        *flags |= ERROR_CALLBACK_CALLED_WITH_ERROR;
    }
}

static void responseCallback(skRequest* request, skResponse* response)
{
    int* flags = (int*)request->userData;
    *flags |= RESPONSE_CALLBACK_CALLED;
}

static void testSuccessfulGET()
{
    skRequest r;
    skRequest_init(&r);
    skRequest_setURL(&r, "http://www.example.com");
    
    int flags = 0;
    r.userData = &flags;
    r.responseCallback = responseCallback;
    r.errorCallback = errorCallback;
    
    skRequest_send(&r, 0);
    const skResponse* resp = &r.response;
    sput_fail_unless(r.errorCode == SK_NO_ERROR, "Successful request should not have an error code set.");
    sput_fail_unless(resp->httpStatusCode == 200, "Wrong response status code");
    sput_fail_unless((flags & ERROR_CALLBACK_CALLED) == 0,
                     "Successful request should not invoke error callback.");
    sput_fail_unless((flags & RESPONSE_CALLBACK_CALLED) != 0,
                     "Successful request should invoke response callback.");
    
    skRequest_deinit(&r);
}

static void testFailedGET()
{
    skRequest r;
    skRequest_init(&r);
    skRequest_setURL(&r, "http://www.example.lol");
    
    int flags = 0;
    r.userData = &flags;
    r.responseCallback = responseCallback;
    r.errorCallback = errorCallback;
    
    skRequest_send(&r, 0);
    sput_fail_unless(r.errorCode != SK_NO_ERROR, "Successful request should not have an error code set.");
    sput_fail_unless((flags & ERROR_CALLBACK_CALLED_WITH_CANCEL) == 0,
                     "Failed request should result in error callback with cancelled error code.");
    sput_fail_unless((flags & ERROR_CALLBACK_CALLED_WITH_ERROR) != 0,
                     "Failed request should not result in error code other than 'cancelled'");
    sput_fail_unless((flags & RESPONSE_CALLBACK_CALLED) == 0,
                     "Failed callback should not be called for a cancelled request.");
    
    skRequest_deinit(&r);
}

static void testFailedGETAsync()
{
    //test connecting to a bougs url, making
    //sure the right callbacks are invoked and
    //the right error codes are being passed.
    {
        skRequest r;
        skRequest_init(&r);
        skRequest_setURL(&r, "http://www.example.lol");
        
        int flags = 0;
        r.userData = &flags;
        r.responseCallback = responseCallback;
        r.errorCallback = errorCallback;
        
        skRequest_send(&r, 1);
        
        skRequest_waitUntilDone(&r);
        
        sput_fail_unless((flags & ERROR_CALLBACK_CALLED_WITH_CANCEL) == 0,
                         "Failed request should result in error callback with cancelled error code.");
        sput_fail_unless((flags & ERROR_CALLBACK_CALLED_WITH_ERROR) != 0,
                         "Failed request should not result in error code other than 'cancelled'");
        sput_fail_unless((flags & RESPONSE_CALLBACK_CALLED) == 0,
                         "Failed callback should not be called for a cancelled request.");
        
        skRequest_deinit(&r);
    }
}

static void testCancelledGETAsync()
{
    //test cancelling an async request, making
    //sure the right callbacks are invoked and
    //the right error codes are being passed.
    {
        skRequest r;
        skRequest_init(&r);
        skRequest_setURL(&r, "http://www.example.com");
        
        int flags = 0;
        r.userData = &flags;
        r.responseCallback = responseCallback;
        r.errorCallback = errorCallback;
        
        skRequest_send(&r, 1);
        skRequest_cancel(&r, 0);
        
        while (skRequest_isRunning(&r))
        {
            skRequest_poll(&r);
        }
    
        sput_fail_unless(flags & ERROR_CALLBACK_CALLED_WITH_CANCEL,
                         "Cancelled flag set by error callback");
        sput_fail_unless((flags & ERROR_CALLBACK_CALLED_WITH_ERROR) == 0,
                         "Cancelled request should invoke the error callback with 'cancelled' code");
        sput_fail_unless((flags & RESPONSE_CALLBACK_CALLED) == 0,
                         "Response callback should not be called for a cancelled request.");
        
        skRequest_deinit(&r);
    }
}

static void testSuccessfulGETAsync()
{
    //test polling while an async request is in progress
    {
        skRequest r;
        skRequest_init(&r);
        skRequest_setURL(&r, "http://www.example.com");
        
        int flags = 0;
        r.userData = &flags;
        r.responseCallback = responseCallback;
        r.errorCallback = errorCallback;
        
        skRequest_send(&r, 1);
        
        while (skRequest_isRunning(&r))
        {
            skRequest_poll(&r);
        }
        
        const skResponse* resp = &r.response;
        sput_fail_unless(resp->httpStatusCode == 200,
                         "Successful async request should have response status 200.");
        sput_fail_unless((flags & ERROR_CALLBACK_CALLED) == 0,
                         "Successful request should not invoke error callback.");
        sput_fail_unless((flags & RESPONSE_CALLBACK_CALLED) != 0,
                         "Successful request should invoke response callback.");
        
        skRequest_deinit(&r);
    }
    
    //test polling after an async request has been completed
    {
        skRequest r;
        skRequest_init(&r);
        skRequest_setURL(&r, "http://www.example.com");
        
        int flags = 0;
        r.userData = &flags;
        r.responseCallback = responseCallback;
        
        skRequest_send(&r, 1);
        
        while (skRequest_getState(&r) == SK_IN_PROGRESS)
        {
            //wait...
        }
        
        while (!flags)
        {
            skRequest_poll(&r);
        }
        
        const skResponse* resp = &r.response;
        sput_fail_unless(resp->httpStatusCode == 200,
                         "Successful async request should have response status 200.");
        sput_fail_unless((flags & ERROR_CALLBACK_CALLED) == 0,
                         "Successful request should not invoke error callback.");
        sput_fail_unless((flags & RESPONSE_CALLBACK_CALLED) != 0,
                         "Successful request should invoke response callback.");
        
        skRequest_deinit(&r);
    }
}

#endif //SK_TEST_REQUEST_H


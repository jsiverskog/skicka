#ifndef SK_REQUEST_H
#define SK_REQUEST_H

#include <curl/curl.h>
#include "tinycthread.h"
#include "http_parser.h"
#include "response.h"
#include "mutablestring.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    struct skRequest;
    
    /**
     * Response callback. Arguments are only valid until this function returns.
     * Deep copy any data that should exist past the call to this function.
     */
    typedef void (*skResponseCallback)(struct skRequest* request, skResponse* response);
    
    /**
     *
     */
    typedef enum skRequestState
    {
        SK_IN_PROGRESS = 0,
        SK_CANCELLED,
        SK_FAILED,
        SK_SUCCEEDED
    } skRequestState;
    
    /**
     *
     */
    typedef enum skHTTPMethod
    {
        SK_HTTP_GET = 0,
        SK_HTTP_POST,
        SK_HTTP_DELETE,
        SK_HTTP_PATCH,
        SK_HTTP_PUT
    } skHTTPMethod;
    
    /**
     * 
     */
    typedef struct skRequest
    {
        /** */
        skHTTPMethod method;
        /** */
        int isRunning;
        /** */
        skRequestState state;
        /** */
        mtx_t mutex;
        /** */
        int shouldCancel;
        /** */
        CURL *curl;
        /** */
        thrd_t thread;
        /** */
        http_parser_settings httpParserSettings;
        /** */
        http_parser httpParser;
        /** */
        skResponse response;
        /** */
        skMutableString requestBody;
        /** */
        void* userData;
        /** */
        skResponseCallback responseCallback;
    } skRequest;
    
    
    
    /**
     *
     */
    void skRequest_init(skRequest* request);
    
    /**
     *
     */
    void skRequest_deinit(skRequest* request);
    
    /**
     *
     */
    void skRequest_setURL(skRequest* request, const char* url);
    
    /**
     *
     */
    void skRequest_setMethod(skRequest* request, skHTTPMethod method);

    /** 
     *
     */
    void skRequest_appendToBody(skRequest* request, const char* string, int urlEncode);
    
    /**
     *
     */
    void skRequest_send(skRequest* request, int async);
    
    /**
     *
     */
    void skRequest_poll(skRequest* request);
    
    /**
     *
     */
    void skRequest_cancel(skRequest* request, int waitUntilCanceled);
    
    /**
     * Waits until an asyncronous request stops running 
     * (i.e succeeds, fails or gets cancelled).
     */
    void skRequest_waitUntilDone(skRequest* request);
    
    /**
     *
     */
    skRequestState skRequest_getState(skRequest* request);
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //SK_REQUEST_H

#ifndef SK_CLIENT_H
#define SK_CLIENT_H

#include <curl/curl.h>
#include "tinycthread.h"
#include "response.h"
#include "request.h"
#include "http_parser.h"
#include "jansson.h"

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
    
#define SK_MAX_NUM_SIMULTANEOUS_REQUESTS 5
    
    /**
     * 
     */
    typedef enum skError
    {
        SK_NO_ERROR = 0,
        SK_COULD_NO_CONNECT_TO_SERVER,
        SK_REQUEST_IS_IN_PROGRESS,
        SK_UNEXPECTED_ERROR,
        SK_INVALID_PARAMETER
    } skError;
    
    /**
     *
     */
    typedef enum skResponseFormat
    {
        SK_FORMAT_RAW = 0,
        SK_FORMAT_JSON
    } skResponseFormat;
    
    /**
     * Response callback. Arguments are only valid until this function returns. 
     * Deep copy any data that should exist past the call to this function.
     */
    typedef void (*skJSONResponseCallback)(skRequest* request, json_t* response);
    
    /**
     *
     */
    typedef struct skRequestPoolEntry
    {
        /** */
        struct skRequest request;
        /** */
        skResponseFormat expectedResponseFormat;
        
        skResponseCallback rawResponseCallback;
        
        skJSONResponseCallback jsonResponseCallback;
    } skRequestPoolEntry;
    
    /**
     * 
     */
    typedef struct skRESTClient
    {
        /** A pool of reusable requests. */
        skRequestPoolEntry requestPool[SK_MAX_NUM_SIMULTANEOUS_REQUESTS];

        /** The base URL, always with a trailing '/'.*/
        char* baseURL;
    
        
        skResponseCallback debugResponseCallback;

    } skRESTClient;

    /**
     *
     */
    skError skRESTClient_init(skRESTClient* client,
                              const char* baseURL);
    
    /**
     *
     */
    void skRESTClient_deinit(skRESTClient* client);
    
    /**
     * 
     */
    skRequest* skRESTClient_getRequestFromPool(skRESTClient* client);
    
    /**
     *
     * @param request A request retreived using \c skRESTClient_getRequestFromPool.
     */
    skError skRESTClient_sendRequestWithJSONResponse(skRESTClient* client,
                                                     skRequest* request,
                                                     const char* path,
                                                     skJSONResponseCallback jsonResponseCallback,
                                                     int async);
    
    /**
     * 
     * @param client
     */
    void skRESTClient_poll(skRESTClient* client);
    
    /**
     *
     */
    void skRESTClient_cancelAllRequests(skRESTClient* client, int waitUntilCancelled);
    
    /**
     * 
     */
    void skRESTClient_waitForAllRequestsToFinish(skRESTClient* client);
    
    /**
     * 
     */
    int skRESTClient_getNumActiveRequests(skRESTClient* client);
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //SK_CLIENT_H

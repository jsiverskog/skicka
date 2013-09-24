#ifndef SK_CLIENT_H
#define SK_CLIENT_H

#include "../extern/tinycthread/tinycthread.h"
#include "../extern/http_parser/http_parser.h"
#include "../extern/jansson/jansson.h"

#include "errorcodes.h"
#include "response.h"
#include "request.h"

/*! \file */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
    
#define SK_MAX_NUM_SIMULTANEOUS_REQUESTS 5
    
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
     * An entry in a pool of reusable requests.
     */
    typedef struct skRequestPoolEntry
    {
        /** The request. */
        struct skRequest request;
        /** The expected response format */
        skResponseFormat expectedResponseFormat;
        /** Gets invoked if the request succeeds and \c skResponseFormat is \c SK_FORMAT_RAW. */
        skResponseCallback rawResponseCallback;
        /** Gets invoked if the request succeeds and \c skResponseFormat is \c SK_FORMAT_JSON. */
        skJSONResponseCallback jsonResponseCallback;
        /** */
        int donNotReturnToPool;
    } skRequestPoolEntry;
    
    /**
     * Represents a client interacting with a REST service.
     */
    typedef struct skRESTClient
    {
        /** A pool of reusable requests. */
        skRequestPoolEntry requestPool[SK_MAX_NUM_SIMULTANEOUS_REQUESTS];
        /** The base URL, always with a trailing '/'.*/
        char* baseURL;
        /** */
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
     * Returns an initialized \c skRequest instance from 
     * a given client's request pool.
     * @param client The REST client.
     * @return An initialized request, or NULL there are no 
     * inactive request in the pool.
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
                                                     skErrorCallback errorCallback,
                                                     int async);
    
    /**
     *
     * @param request A request retreived using \c skRESTClient_getRequestFromPool.
     */
    skError skRESTClient_sendRequest(skRESTClient* client,
                                     skRequest* request,
                                     const char* path,
                                     skResponseCallback responseCallback,
                                     skErrorCallback errorCallback,
                                     int async);
    
    /**
     *
     * @param client
     */
    void skRESTClient_poll(skRESTClient* client);
    
    /**
     * Cancels all currently running requests.
     * @param client
     * @param waitUntilCancelled 
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

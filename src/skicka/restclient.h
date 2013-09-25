#ifndef SK_CLIENT_H
#define SK_CLIENT_H

#include "../extern/tinycthread/tinycthread.h"
#include "../extern/http_parser/http_parser.h"

#include "errorcodes.h"
#include "response.h"
#include "request.h"

/*! \file */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
    
#define SK_MAX_NUM_SIMULTANEOUS_REQUESTS 25
    
    /**
     * An entry in a pool of reusable requests.
     */
    typedef struct skRequestPoolEntry
    {
        /** The request. */
        struct skRequest request;
        /** */
        int manualDeinit;
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
     */
    skError skRESTClient_setRequestManualDeinit(skRESTClient* client, skRequest* r, int manualDeinit);

    /**
     * Invokes \c responseCallback and \c errorCallback of \c request on completion.
     * @param client
     * @param request A request retreived using \c skRESTClient_getRequestFromPool.
     * @param path The path to the REST resource. If NULL, the currently set URL of 
     * \c request will be used.
     */
    skError skRESTClient_sendRequest(skRESTClient* client,
                                     skRequest* request,
                                     const char* path,
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

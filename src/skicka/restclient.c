#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "restclient.h"
#include "request.h"

static void onRequestFinished(skRESTClient* client, skRequestPoolEntry* rpe)
{
    skRequest* r = &rpe->request;
    
    r->isRunning = 0;
    
    if (r->state == SK_SUCCEEDED)
    {
        r->response.body = &r->response.bytes[r->response.bodyStart];
        
        if (r->responseCallback)
        {
            r->responseCallback(r, &r->response);
        }
    }
    else if (r->errorCallback)
    {
        r->errorCallback(r, r->errorCode);
    }
    
    if (!rpe->donNotReturnToPool)
    {
        skRequest_deinit(r);
    }
}


skError skRESTClient_init(skRESTClient* client,
                          const char* baseURL)
{
    //store the base URL and append a trailing '/' if missing
    {
        memset(client, 0, sizeof(skRESTClient));
        const size_t baseUrlLength = strlen(baseURL);
        
        client->baseURL = (char*)malloc(baseUrlLength + 2); //+2 in case we need to add a /
        
        memcpy(client->baseURL, baseURL, baseUrlLength + 1);
        if (client->baseURL[baseUrlLength - 1] != '/')
        {
            client->baseURL[baseUrlLength] = '/';
            client->baseURL[baseUrlLength + 1] = '\0';
        }
    }
    
    //init CURL
    {
        CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
        
        if (res != CURLE_OK)
        {
            return SK_UNEXPECTED_ERROR;
        }
    }
        
    return SK_NO_ERROR;
}

void skRESTClient_deinit(skRESTClient* client)
{
    //CURL cleanup
    curl_global_cleanup();
    
    if (client->baseURL)
    {
        free(client->baseURL);
    }
    
    memset(client, 0, sizeof(skRESTClient));
}

skRequest* skRESTClient_getRequestFromPool(skRESTClient* client)
{
    int freeRequestSlot = -1;
    for (int i = 0; i < SK_MAX_NUM_SIMULTANEOUS_REQUESTS; i++)
    {
        if (!client->requestPool[i].request.isRunning &&
            !client->requestPool[i].donNotReturnToPool)
        {
            freeRequestSlot = i;
            break;
        }
    }
    
    if (freeRequestSlot < 0)
    {
        return NULL;
    }
    
    memset(&client->requestPool[freeRequestSlot], 0, sizeof(skRequestPoolEntry));
    skRequest* r = &client->requestPool[freeRequestSlot].request;
    skRequest_init(r);
    
    return r;
}

skError skRESTClient_setRequestRecyclable(skRESTClient* client, skRequest* r, int isRecyclable)
{
    skRequestPoolEntry* rpe = NULL;
    for (int i = 0; i < SK_MAX_NUM_SIMULTANEOUS_REQUESTS; i++)
    {
        if (&client->requestPool[i].request == r)
        {
            rpe = &client->requestPool[i];
            break;
        }
    }
    
    if (!rpe)
    {
        return SK_INVALID_PARAMETER;
    }
    
    rpe->donNotReturnToPool = !isRecyclable;
    
    return SK_NO_ERROR;
}

skError skRESTClient_sendRequest(skRESTClient* client,
                                 skRequest* request,
                                 const char* path,
                                 int async)
{
    if (!request)
    {
        return SK_INVALID_PARAMETER;
    }
    
    int poolIndex = -1;
    for (int i = 0; i < SK_MAX_NUM_SIMULTANEOUS_REQUESTS; i++)
    {
        if (&client->requestPool[i].request == request)
        {
            poolIndex = i;
            break;
        }
    }
    
    if (poolIndex < 0)
    {
        return SK_INVALID_PARAMETER;
    }
    
    //construct request URL
    long pathLength = 0;
    int pathStart = 0;
    if (path)
    {
        pathLength = strlen(path);
        
        if (pathLength > 0)
        {
            if (path[0] == '/')
            {
                pathStart = 1;
            }
        }
    }
    
    skMutableString url;
    skMutableString_init(&url);
    skMutableString_append(&url, client->baseURL);
    
    if (pathLength > 0)
    {
        skMutableString_append(&url, &path[pathStart]);
    }
    
    if (path)
    {
        skRequest_setURL(request, skMutableString_getString(&url));
    }
    
    skMutableString_deinit(&url);
    
    skRequest_send(request, async);
    
    return SK_NO_ERROR;
}


void skRESTClient_poll(skRESTClient* client)
{
    for (int i = 0; i < SK_MAX_NUM_SIMULTANEOUS_REQUESTS; i++)
    {
        skRequest* r = &client->requestPool[i].request;
        if (r->isRunning == 0)
        {
            continue;
        }
        
        mtx_lock(&r->mutex);
        skRequestState state = r->state;
        mtx_unlock(&r->mutex);
        
        if (state != SK_IN_PROGRESS)
        {
            onRequestFinished(client, &client->requestPool[i]);
        }
    }
}

void skRESTClient_cancelAllRequests(skRESTClient* client, int waitUntilCancelled)
{
    for (int i = 0; i < SK_MAX_NUM_SIMULTANEOUS_REQUESTS; i++)
    {
        skRequest* r = &client->requestPool[i].request;
        if (r == 0)
        {
            continue;
        }
        
        skRequest_cancel(r, 0);
    }
    
    if (waitUntilCancelled)
    {
        skRESTClient_waitForAllRequestsToFinish(client);
    }
}

void skRESTClient_waitForAllRequestsToFinish(skRESTClient* client)
{
    /*This do while statement allows requests to be sent in the
     callbacks invoked by skRESTClient_poll*/
    do
    {
        for (int i = 0; i < SK_MAX_NUM_SIMULTANEOUS_REQUESTS; i++)
        {
            skRequest* r = &client->requestPool[i].request;
            if (r == 0)
            {
                continue;
            }
            
            int res = 0;
            thrd_join(r->thread, &res);
        }
        
        //do a poll to fire notifications about finished requests
        skRESTClient_poll(client);
    }
    while (skRESTClient_getNumActiveRequests(client) > 0);
}

int skRESTClient_getNumActiveRequests(skRESTClient* client)
{
    int numActive = 0;
    
    for (int i = 0; i < SK_MAX_NUM_SIMULTANEOUS_REQUESTS; i++)
    {
        skRequest* r = &client->requestPool[i].request;
        if (r == 0)
        {
            continue;
        }
        
        if (r->isRunning)
        {
            numActive++;
        }
    }
    
    return numActive;
}

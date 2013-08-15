#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "restclient.h"
#include "request.h"

static void onRequestFinished(skRESTClient* client, skRequestPoolEntry* rpe)
{
    skRequest* r = &rpe->request;
    if (r->state == SK_SUCCEEDED)
    {
        r->response.body = &r->response.bytes[r->response.bodyStart];
        
        if (client->debugResponseCallback)
        {
            client->debugResponseCallback(r, &r->response);
        }
        
        if (rpe->expectedResponseFormat == SK_FORMAT_JSON &&
            rpe->jsonResponseCallback)
        {
            json_error_t error;
            json_t* root = json_loads(r->response.body, 0, &error);
            rpe->jsonResponseCallback(r, root);
            json_decref(root);
        }
        else if (rpe->rawResponseCallback)
        {
            //raw
            rpe->rawResponseCallback(r, &r->response);
        }
    }
    
    skRequest_deinit(r);
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
        if (!client->requestPool[i].request.isRunning)
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

static skError skRESTClient_sendRequestPrivate(skRESTClient* client,
                                               skRequest* request,
                                               const char* path,
                                               skResponseCallback responseCallback,
                                               skJSONResponseCallback jsonResponseCallback,
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
    const long pathLength = strlen(path);
    
    int pathStart = 0;
    if (pathLength > 0)
    {
        if (path[0] == '/')
        {
            pathStart = 1;
        }
    }
    
    if (responseCallback)
    {
        client->requestPool[poolIndex].expectedResponseFormat = SK_FORMAT_RAW;
        client->requestPool[poolIndex].rawResponseCallback = responseCallback;
    }
    else
    {
        client->requestPool[poolIndex].expectedResponseFormat = SK_FORMAT_JSON;
        client->requestPool[poolIndex].jsonResponseCallback = jsonResponseCallback;
    }
    
    skMutableString url;
    skMutableString_init(&url);
    skMutableString_append(&url, client->baseURL);
    
    if (pathLength > 0)
    {
        skMutableString_append(&url, &path[pathStart]);
    }
    
    skRequest_setURL(request, skMutableString_getString(&url));
    
    skMutableString_deinit(&url);
    
    skRequest_send(request, async);
    
    return SK_NO_ERROR;
}


skError skRESTClient_sendRequestWithJSONResponse(skRESTClient* client,
                                                 skRequest* request,
                                                 const char* path,
                                                 skJSONResponseCallback jsonResponseCallback,
                                                 int async)
{
    return skRESTClient_sendRequestPrivate(client,
                                           request,
                                           path,
                                           NULL,
                                           jsonResponseCallback,
                                           async);
}

skError skRESTClient_sendRequest(skRESTClient* client,
                                 skRequest* request,
                                 const char* path,
                                 skResponseCallback responseCallback,
                                 int async)
{
    return skRESTClient_sendRequestPrivate(client,
                                           request,
                                           path,
                                           responseCallback,
                                           NULL,
                                           async);
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
 
    //do a poll fire notifications about finished requests
    skRESTClient_poll(client);
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
        
        numActive++;
    }
    
    return numActive;
}

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "restclient.h"
#include "request.h"

static size_t readCallback(void *ptr, size_t size, size_t nmemb, void *userp)
{
    skRequest* c = (skRequest*)userp;
    
    mtx_lock(&c->mutex);
    int shouldCancel = c->shouldCancel;
    mtx_unlock(&c->mutex);
    
    if (shouldCancel)
    {
        return CURL_READFUNC_ABORT;
    }
    
    //printf("%s", ptr);
    
    const size_t numBytes = size * nmemb;
    
    c->response.bytes = realloc(c->response.bytes, c->response.size + numBytes);
    memcpy(&c->response.bytes[c->response.size], ptr, numBytes);
    c->response.size += numBytes;
    
    //printf("%s\n", ptr);
    
    const size_t numParsedBytes = http_parser_execute(&c->httpParser,
                                                      &c->httpParserSettings,
                                                      ptr,
                                                      numBytes);
    
    //printf("%s\n", http_errno_description(HTTP_PARSER_ERRNO(&c->httpParser)));
    
    return numBytes;
}

static int on_message_begin(http_parser* p)
{
    skRequest* c = (skRequest*)p->data;
    //printf("-------------ON MESSAGE BEGIN----------\n");
    return 0;
}

static int on_message_complete(http_parser* p)
{
    skRequest* c = (skRequest*)p->data;
    //printf("-----------ON MESSAGE COMPLETE---------\n");
    return 0;
}

static int on_headers_complete(http_parser* p)
{
    skRequest* c = (skRequest*)p->data;
    c->response.httpStatusCode = p->status_code;
    c->response.bodyStart = p->nread;
    //printf("----------ON HEADERS COMPLETE----------\n");
    return 0;
}

static int requestThreadEntryPoint(void *userPtr)
{
    skRequest* request = (skRequest*)userPtr;
    
    CURLcode res = curl_easy_perform(request->curl);
    
    if(res == CURLE_OK)
    {
        http_parser_execute(&request->httpParser,
                            &request->httpParserSettings,
                            NULL,
                            0);
    }
    
    skRequestState state = SK_SUCCEEDED;
    
    
    if (request->shouldCancel)
    {
        state = SK_CANCELLED;
    }
    else if (res != CURLE_OK)
    {
        state = SK_FAILED;
    }
    
    if (state == SK_SUCCEEDED)
    {
        request->response.body = &request->response.bytes[request->response.bodyStart];
    }
    
    mtx_lock(&request->mutex);
    request->state = state;
    mtx_unlock(&request->mutex);
    
    return 0;
}

void skRequest_init(skRequest* request)
{
    memset(request, 0, sizeof(skRequest));
    request->curl = curl_easy_init();
    skResponse_init(&request->response);
    skMutableString_init(&request->requestBody);
    
    //set up http parser + callbacks
    memset(&request->httpParserSettings, 0, sizeof(http_parser_settings));
    request->httpParserSettings.on_headers_complete = on_headers_complete;
    request->httpParserSettings.on_message_begin = on_message_begin;
    request->httpParserSettings.on_message_complete = on_message_complete;
    http_parser_init(&request->httpParser, HTTP_RESPONSE);
    request->httpParser.data = request;
}

void skRequest_deinit(skRequest* request)
{
    if (skRequest_getState(request) == SK_IN_PROGRESS)
    {
        skRequest_cancel(request, 1);
    }
    
    if (request->headerFieldList)
    {
        curl_slist_free_all(request->headerFieldList);
    }
    
    curl_easy_cleanup(request->curl);
    skResponse_deinit(&request->response);
    skMutableString_deinit(&request->requestBody);
    
    memset(request, 0, sizeof(skRequest));
}

void skRequest_setURL(skRequest* request, const char* url)
{
    curl_easy_setopt(request->curl, CURLOPT_URL, url);
}

void skRequest_setMethod(skRequest* request, skHTTPMethod method)
{
    request->method = method;
}

void skRequest_appendToBody(skRequest* request, const char* string, int urlEncode)
{
    if (urlEncode)
    {
        char* escaped = curl_easy_escape(request->curl, string, 0);
        skMutableString_append(&request->requestBody, escaped);
        curl_free(escaped);
    }
    else
    {
        skMutableString_append(&request->requestBody, string);
    }
}

void skRequest_addHeaderField(skRequest* request, const char* name, const char* value)
{
    skMutableString s;
    skMutableString_init(&s);
    skMutableString_append(&s, name);
    skMutableString_append(&s, ": ");
    skMutableString_append(&s, value);
    
    request->headerFieldList = curl_slist_append(request->headerFieldList, skMutableString_getString(&s));
    
    skMutableString_deinit(&s);
}

void skRequest_send(skRequest* request, int async)
{
    if (request->isRunning)
    {
        return;
    }
    
    request->isRunning = 1;
    
    request->state = SK_IN_PROGRESS;
    
    if (request->headerFieldList)
    {
        curl_easy_setopt(request->curl, CURLOPT_HTTPHEADER, request->headerFieldList);
    }
    
    curl_easy_setopt(request->curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(request->curl, CURLOPT_WRITEHEADER, request);
    curl_easy_setopt(request->curl, CURLOPT_WRITEFUNCTION, readCallback);
    curl_easy_setopt(request->curl, CURLOPT_WRITEDATA, request);
    
    const char* requestBody = skMutableString_getString(&request->requestBody);
    
    if (requestBody)
    {
        curl_easy_setopt(request->curl, CURLOPT_POST, 1);
        curl_easy_setopt(request->curl, CURLOPT_POSTFIELDS, requestBody);
    }
    else if (request->method == SK_HTTP_DELETE)
    {
        curl_easy_setopt(request->curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    }
    else if (request->method == SK_HTTP_PATCH)
    {
        curl_easy_setopt(request->curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    }
    else if (request->method == SK_HTTP_PUT)
    {
        curl_easy_setopt(request->curl, CURLOPT_CUSTOMREQUEST, "PUT");
    }
    
    if (async)
    {
        thrd_create(&request->thread, requestThreadEntryPoint, request);
    }
    else
    {
        requestThreadEntryPoint(request);
        request->isRunning = 0;
        if (request->responseCallback)
        {
            request->responseCallback(request, &request->response);
        }
    }
}

void skRequest_poll(skRequest* request)
{
    if (request->isRunning == 0)
    {
        return;
    }
    
    skRequestState state = skRequest_getState(request);
    
    if (state != SK_IN_PROGRESS)
    {
        request->isRunning = 0;
        if (request->responseCallback)
        {
            request->responseCallback(request, &request->response);
        }
    }
}

void skRequest_cancel(skRequest* request, int waitUntilCanceled)
{
    mtx_lock(&request->mutex);
    request->shouldCancel = 1;
    mtx_unlock(&request->mutex);
    
    if (waitUntilCanceled)
    {
        skRequest_waitUntilDone(request);
    }
}

void skRequest_waitUntilDone(skRequest* request)
{
    if (!request->thread)
    {
        return;
    }
    
    int res = 0;
    const int joinResult = thrd_join(request->thread, &res);
    if (joinResult != thrd_success)
    {
        assert(0 && "thread join error"); //TODO: handle this error
    }
}

skRequestState skRequest_getState(skRequest* request)
{
    mtx_lock(&request->mutex);
    skRequestState state = request->state;
    mtx_unlock(&request->mutex);
    
    return state;
}


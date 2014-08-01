#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <curl/curl.h>

#include "restclient.h"
#include "request.h"

static
void dump(const char *text,
          FILE *stream, unsigned char *ptr, size_t size,
          char nohex)
{
    size_t i;
    size_t c;
    
    unsigned int width=0x10;
    
    if(nohex)
    /* without the hex output, we can fit more on screen */
        width = 0x40;
    
    fprintf(stream, "%s, %10.10ld bytes (0x%8.8lx)\n",
            text, (long)size, (long)size);
    
    for(i=0; i<size; i+= width) {
        
        fprintf(stream, "%4.4lx: ", (long)i);
        
        if(!nohex) {
            /* hex not disabled, show it */
            for(c = 0; c < width; c++)
                if(i+c < size)
                    fprintf(stream, "%02x ", ptr[i+c]);
                else
                    fputs("   ", stream);
        }
        
        for(c = 0; (c < width) && (i+c < size); c++) {
            /* check for 0D0A; if found, skip past and start a new line of output */
            if (nohex && (i+c+1 < size) && ptr[i+c]==0x0D && ptr[i+c+1]==0x0A) {
                i+=(c+2-width);
                break;
            }
            fprintf(stream, "%c",
                    (ptr[i+c]>=0x20) && (ptr[i+c]<0x80)?ptr[i+c]:'.');
            /* check again for 0D0A, to avoid an extra \n if it's at width */
            if (nohex && (i+c+2 < size) && ptr[i+c+1]==0x0D && ptr[i+c+2]==0x0A) {
                i+=(c+3-width);
                break;
            }
        }
        fputc('\n', stream); /* newline */ 
    }
    fflush(stream);
}

static int my_trace(CURL *handle, curl_infotype type,
                    char *data, size_t size,
                    void *userp)
{
    const char *text;
    (void)handle; /* prevent compiler warning */
    
    switch (type) {
        case CURLINFO_TEXT:
            fprintf(stderr, "== Info: %s", data);
        default: /* in case a new one is introduced to shock us */
            return 0;
            
        case CURLINFO_HEADER_OUT:
            text = "=> Send header";
            break;
        case CURLINFO_DATA_OUT:
            text = "=> Send data";
            break;
        case CURLINFO_SSL_DATA_OUT:
            text = "=> Send SSL data";
            break;
        case CURLINFO_HEADER_IN:
            text = "<= Recv header";
            break;
        case CURLINFO_DATA_IN:
            text = "<= Recv data";
            break;
        case CURLINFO_SSL_DATA_IN:
            text = "<= Recv SSL data";
            break;
    }
    
     dump(text, stderr, (unsigned char *)data, size, 1);
    
    return 0;
}

static size_t responseBytesReceivedCallback(void *ptr, size_t size, size_t nmemb, void *userp)
{
    skRequest* c = (skRequest*)userp;
    
    mtx_lock(&c->mutex);
    int shouldCancel = c->shouldCancel;
    mtx_unlock(&c->mutex);
    
    if (shouldCancel)
    {
        return CURL_READFUNC_ABORT;
    }
    
    const size_t numBytes = size * nmemb;
    
    c->response.bytes = realloc(c->response.bytes, c->response.size + numBytes);
    memcpy(&c->response.bytes[c->response.size], ptr, numBytes);
    c->response.size += numBytes;
    
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
    c->response.bodySize = c->response.size - c->response.bodyStart;
    
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

static int debugThreadCount = 0;

static int requestThreadEntryPoint(void *userPtr)
{
    debugThreadCount++;
    //printf("requestThreadEntryPoint, thread count %d\n", debugThreadCount);
    
    skRequest* request = (skRequest*)userPtr;
    
    if (request->requestThreadEntryCallback && request->async)
    {
        request->requestThreadEntryCallback(request);
    }
    
    assert(request->isRunning);
    assert(request->curl);
    
    CURLcode res = curl_easy_perform(request->curl);
    
    //printf("curl_easy_perform returned %d (%s)\n", res, curl_easy_strerror(res));
    
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
        request->errorCode = SK_REQUEST_CANCELLED;
        state = SK_CANCELLED;
    }
    else if (res != CURLE_OK)
    {
        request->errorCode = SK_COULD_NOT_CONNECT_TO_SERVER;
        state = SK_FAILED;
    }
    
    if (state == SK_SUCCEEDED)
    {
        //null terminate the received bytes
        request->response.bytes = realloc(request->response.bytes, request->response.size + 1);
        request->response.bytes[request->response.size] = '\0';
        request->response.body = &request->response.bytes[request->response.bodyStart];
        
        request->response.bodySize = request->response.size - request->response.bodyStart;
    }
    
    mtx_lock(&request->mutex);
    request->state = state;
    mtx_unlock(&request->mutex);
    
    assert(request->isRunning);
    debugThreadCount--;
    return 0;
}

static int debugAllocationBalance = 0;

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
    
    debugAllocationBalance++;
    
    //printf("skRequest_init %p, debugAllocationBalance %d\n", request, debugAllocationBalance);
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
    
    debugAllocationBalance--;
    //printf("skRequest_deinit %p, debugAllocationBalance %d\n", request, debugAllocationBalance);
}

void skRequest_setURL(skRequest* request, const char* url)
{
    skMutableString_deinit(&request->url);
    skMutableString_append(&request->url, url);
    
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

void skRequest_setHeaderField(skRequest* request, const char* name, const char* value)
{
    //replace the field, if it exists.
    skMutableString s;
    skMutableString_init(&s);
    skMutableString_append(&s, name);
    skMutableString_append(&s, ": ");
    
    struct curl_slist* existingItem = NULL;
    {
        struct curl_slist* curr = request->headerFieldList;
        while (curr)
        {
            size_t lk = strlen(skMutableString_getString(&s));
            size_t ld = strlen(curr->data);
            
            const int exists = ld < lk ? 0 : strncmp(curr->data, skMutableString_getString(&s), lk) == 0;
            if (exists)
            {
                existingItem = curr;
                break;
            }
            
            curr = curr->next;
        }
    }
    
    skMutableString_append(&s, value);
    
    
    if (existingItem)
    {
        struct curl_slist* newList = NULL;
        struct curl_slist* curr = request->headerFieldList;
        while (curr)
        {
            if (curr == existingItem)
            {
                newList = curl_slist_append(newList,
                                            skMutableString_getString(&s));
            }
            else
            {
                newList = curl_slist_append(newList,
                                            curr->data);
            }
            
            curr = curr->next;
        }
        
        
        curl_slist_free_all(request->headerFieldList);
        request->headerFieldList = newList;
    }
    else
    {
        request->headerFieldList = curl_slist_append(request->headerFieldList,
                                                     skMutableString_getString(&s));
    }
    
    skMutableString_deinit(&s);
}

void skRequest_send(skRequest* request, int async)
{
    if (request->isRunning)
    {
        return;
    }
    
    request->state = SK_IN_PROGRESS;
    
    if (request->headerFieldList)
    {
        curl_easy_setopt(request->curl, CURLOPT_HTTPHEADER, request->headerFieldList);
    }
    
    curl_easy_setopt(request->curl, CURLOPT_CONNECTTIMEOUT_MS, SK_REQUEST_CONNECT_TIMEOUT_MS);
    
    curl_easy_setopt(request->curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(request->curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(request->curl, CURLOPT_WRITEHEADER, request);
    curl_easy_setopt(request->curl, CURLOPT_WRITEFUNCTION, responseBytesReceivedCallback);
    curl_easy_setopt(request->curl, CURLOPT_WRITEDATA, request);
    
    if (request->log)
    {
        curl_easy_setopt(request->curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(request->curl, CURLOPT_DEBUGFUNCTION, my_trace);
    }
    
    const char* requestBody = skMutableString_getString(&request->requestBody);
    
    if (strlen(requestBody))
    {
        curl_easy_setopt(request->curl, CURLOPT_POST, 1);
        curl_easy_setopt(request->curl, CURLOPT_POSTFIELDS, requestBody);
    }
    
    if (request->method == SK_HTTP_DELETE)
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
    
    curl_easy_setopt(request->curl, CURLOPT_URL, skMutableString_getString(&request->url));
    
    skResponse_deinit(&request->response);
    http_parser_init(&request->httpParser, HTTP_RESPONSE);
    
    request->isRunning = 1;
    request->async = async;
    
    if (async)
    {
        const int res = thrd_create(&request->thread, requestThreadEntryPoint, request);
        if (res == thrd_success) {
            thrd_detach(request->thread);
        } else {
            // TODO: handle this case
        }
    }
    else
    {
        requestThreadEntryPoint(request);
        request->isRunning = 0;
        
        if (request->state == SK_SUCCEEDED)
        {
            if (request->responseCallback)
            {
                request->responseCallback(request, &request->response);
            }
        }
        else if (request->errorCode != SK_NO_ERROR)
        {
            if (request->errorCallback)
            {
                request->errorCallback(request, request->errorCode);
            }
        }
    }
}

void skRequest_poll(skRequest* request)
{
    if (request->isRunning == 0)
    {
        /*The request is not active. Nothing further.*/
        return;
    }
    
    skRequestState state = skRequest_getState(request);
    
    if (state == SK_IN_PROGRESS)
    {
        /*Waiting for a state change.*/
    }
    else
    {
        int joinRes = 0;
        thrd_join(request->thread, &joinRes);
        
        /* The state changed from SK_IN_PROGRESS. */
        request->isRunning = 0;
        
        if (request->errorCode == SK_NO_ERROR)
        {
            if (request->responseCallback)
            {
                request->responseCallback(request, &request->response);
            }
        }
        else
        {
            if (request->errorCallback)
            {
                request->errorCallback(request, request->errorCode);
            }
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
    
    skRequest_poll(request);
}

skRequestState skRequest_getState(skRequest* request)
{
    mtx_lock(&request->mutex);
    skRequestState state = request->state;
    mtx_unlock(&request->mutex);
    
    return state;
}

int skRequest_isRunning(skRequest* request)
{
    return request->isRunning;
}

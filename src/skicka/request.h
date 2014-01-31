#ifndef SK_REQUEST_H
#define SK_REQUEST_H


#include "../extern/tinycthread/tinycthread.h"
#include "../extern/http_parser/http_parser.h"
#include "response.h"
#include "mutablestring.h"
#include "errorcodes.h"

/*! \file */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

#define SK_REQUEST_NUM_USER_DATA_ENTRIES 8
    
#define SK_REQUEST_CONNECT_TIMEOUT_MS 5000
    
    typedef void CURL;
    struct curl_slist;
    struct skRequest;
    
    /**
     * Response callback. Arguments are only valid until this function returns.
     * Deep copy any data that should exist past the call to this function.
     * @param request The request.
     * @param respose The response.
     */
    typedef void (*skResponseCallback)(struct skRequest* request, skResponse* response);
    
    /**
     *
     */
    typedef void (*skErrorCallback)(struct skRequest* request, skError errorCode);
    
    /**
     * Called from the request thread as soon as it has been created (only if the request
     * is run in asynchronous mode).
     */
    typedef void (*skRequestThreadEntryCallback)(struct skRequest* request);
    
    /**
     * Valid HTTP request states.
     */
    typedef enum skRequestState
    {
        SK_IN_PROGRESS = 0,
        SK_CANCELLED,
        SK_FAILED,
        SK_SUCCEEDED
    } skRequestState;
    
    /**
     * Valid HTTP request methods.
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
     * An HTTP request.
     */
    typedef struct skRequest
    {
        /** Set to a non-zero value for verbose logging. */
        int log;
        /** */
        skHTTPMethod method;
        /** */
        int isRunning;
        /** */
        int async;
        /** */
        skRequestState state;
        /** */
        mtx_t mutex;
        /** */
        int shouldCancel;
        /** */
        CURL *curl;
        /** */
        struct curl_slist* headerFieldList;
        /** */
        thrd_t thread;
        /** */
        skMutableString requestBody;
        /** */
        skMutableString url;
        /** */
        void* userData[SK_REQUEST_NUM_USER_DATA_ENTRIES];
        /** General purpose custom user flags*/
        int userFlags;
        /** */
        http_parser_settings httpParserSettings;
        /** */
        http_parser httpParser;
        /** */
        skResponse response;
        /** */
        skResponseCallback responseCallback;
        /** */
        skErrorCallback errorCallback;
        /** */
        skRequestThreadEntryCallback requestThreadEntryCallback;
        /** */
        skError errorCode;

    } skRequest;
        
    /**
     * Initializes an HTTP request and allocates resources
     * associated with the request.
     * @param request The request to initialize.
     */
    void skRequest_init(skRequest* request);
    
    /**
     * Frees an HTTP request and frees resources 
     * associated with the request. Must not be called when 
     * the request is in progress.
     * @param request The request to deinitialize.
     */
    void skRequest_deinit(skRequest* request);
    
    /**
     * Sets the URL of an HTTP request.
     * @param request The request.
     * @param url The URL.
     */
    void skRequest_setURL(skRequest* request, const char* url);
    
    /**
     * Sets the HTTP method for an HTTP request.
     * @param request The request.
     * @param method The HTTP method.
     */
    void skRequest_setMethod(skRequest* request, skHTTPMethod method);

    /** 
     * Appends a given string to the body of a request and
     * sets the HTTP method to POST.
     * @param request The request.
     * @param string The string to append.
     * @param urlEncode If non-zero, \c string is URL encoded before it is appended.
     */
    void skRequest_appendToBody(skRequest* request, const char* string, int urlEncode);
    
    /**
     * Adds an HTTP header field to a given request.
     * @param request The request.
     * @param name The name of the header field.
     * @param value The value of the header field.
     */
    void skRequest_setHeaderField(skRequest* request, const char* name, const char* value);
    
    /**
     * Sends an HTTP request.
     * @param request The request to send.
     * @param async If non-zero, the request this function returns immediately
     * and the request is sent on a separate thread. In this case, use \c skRequest_poll 
     * to poll for request completion.
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
     * @param request The request.
     */
    void skRequest_waitUntilDone(skRequest* request);
    
    /**
     *
     */
    skRequestState skRequest_getState(skRequest* request);
    
    /**
     *
     */
    int skRequest_isRunning(skRequest* request);
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //SK_REQUEST_H

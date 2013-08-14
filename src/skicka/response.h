#ifndef SK_RESPONSE_H
#define SK_RESPONSE_H


/*! \file */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /**
     * A response to an HTTP request.
     */
    typedef struct skResponse
    {
        int httpStatusCode;
        int bodyStart;
        int size;
        char* bytes;
        char* body;
    } skResponse;
    
    /**
     *
     */
    void skResponse_init(skResponse* response);
    
    /**
     *
     */
    void skResponse_deinit(skResponse* response);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //SK_RESPONSE_H

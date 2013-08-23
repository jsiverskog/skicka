#ifndef SK_ERROR_CODES_H
#define SK_ERROR_CODES_H

/*! \file */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */
    
    /**
     * 
     */
    typedef enum skError
    {
        SK_NO_ERROR = 0,
        SK_COULD_NOT_CONNECT_TO_SERVER,
        SK_REQUEST_CANCELLED,
        SK_REQUEST_IS_IN_PROGRESS,
        SK_UNEXPECTED_ERROR,
        SK_INVALID_PARAMETER
    } skError;
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /*SK_ERROR_CODES_H*/

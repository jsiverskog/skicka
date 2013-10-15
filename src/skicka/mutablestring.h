#ifndef SK_MUTABLE_STRING_H
#define SK_MUTABLE_STRING_H

/*! \file */

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    /** The maximum number of chars in */
    #define SK_MUTABLE_STRING_STATIC_SIZE 255
    
    /**
     * A mutable string.
     */
    typedef struct skMutableString
    {
        /** */
        int charCount;
        /** 
         * This statically allocated buffer is used for short strings
         * to avoid excessive allocations.
         */
        char staticData[SK_MUTABLE_STRING_STATIC_SIZE + 1];
        /** 
         * This buffer is used for strings exceeding
         * \c SK_MUTABLE_STRING_STATIC_SIZE.
         */
        char* dynamicData;
    } skMutableString;
    
    /**
     *
     */
    void skMutableString_init(skMutableString* ms);
    
    /**
     *
     */
    void skMutableString_deinit(skMutableString* ms);
    
    /**
     *
     */
    void skMutableString_append(skMutableString* ms, const char* toAppend);
    
    /**
     *
     */
    void skMutableString_appendInt(skMutableString* ms, int toAppend);
    
    /**
     *
     */
    void skMutableString_appendBytes(skMutableString* ms, const char* toAppend, int numBytes);
    
    /**
     *
     */
    const char* skMutableString_getString(skMutableString* ms);
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //SK_MUTABLE_STRING_H

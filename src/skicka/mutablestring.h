#ifndef SK_MUTABLE_STRING_H
#define SK_MUTABLE_STRING_H

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

    
    typedef struct skMutableString
    {
        int charCount;
        //char staticData[SK_MUTABLE_STRING_STATIC_SIZE]; TODO: use for small strings
        char* data;
    } skMutableString;
    
    void skMutableString_init(skMutableString* ms);
    
    void skMutableString_deinit(skMutableString* ms);
    
    void skMutableString_append(skMutableString* ms, const char* toAppend);
    
    const char* skMutableString_getString(skMutableString* ms);
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif //SK_MUTABLE_STRING_H

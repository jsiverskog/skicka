#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "testrequest.h"
#include "testmutablestring.h"




int main(int argc, const char * argv[])
{
    
    testGET();
    testGETAsync();
    testPOST();
    
    testMutableString();
    
    return 0;
}


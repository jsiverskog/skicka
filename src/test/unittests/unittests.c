#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "testrequest.h"




int main(int argc, const char * argv[])
{
    
    testGET();
    testGETAsync();
    testPOST();
    
    return 0;
}


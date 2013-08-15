#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sput.h"

#include "testclient.h"
#include "testrequest.h"
#include "testmutablestring.h"

int main(int argc, const char * argv[])
{
    sput_start_testing();
    
    sput_enter_suite("Mutable string tests");
    sput_run_test(testMutableStringSingleAppend);
    sput_run_test(testMutableStringMutipleAppend);
    
    sput_enter_suite("Request tests");
    sput_run_test(testGET);
    sput_run_test(testGETAsync);
    sput_run_test(testPOST);
    
    sput_enter_suite("REST Client tests");
    
    sput_finish_testing();
    
    return 0;
}


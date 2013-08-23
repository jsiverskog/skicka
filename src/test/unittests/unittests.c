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
    
    sput_enter_suite("Synchronous request tests");
    sput_run_test(testSuccessfulGET);
    sput_run_test(testFailedGET);
    
    sput_enter_suite("Asynchronous request tests");
    sput_run_test(testSuccessfulGETAsync);
    sput_run_test(testCancelledGETAsync);
    sput_run_test(testFailedGETAsync);
    
    sput_enter_suite("REST Client tests");
    sput_run_test(testClientRequestCount);
    sput_run_test(testClientRequestPoolSize);
    
    sput_finish_testing();
    
    return 0;
}


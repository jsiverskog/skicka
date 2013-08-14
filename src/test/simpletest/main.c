#include "request.h"

static void responseCallback(skRequest* request, skResponse* response)
{
    printf("%s\n", response->body);
}

int main(int argc, const char * argv[])
{
    const char* testServerURL = "http://localhost:8000/validjson";
    
    //create a request and send it to the local test server
    {
        skRequest r;
        skRequest_init(&r);
        skRequest_setURL(&r, testServerURL);
        r.responseCallback = responseCallback;
        skRequest_send(&r, 0);
        
    }
    return 0;
}


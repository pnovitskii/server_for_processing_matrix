#include <iostream>
#include <time.h>
#include <iomanip>
#include <vector>

#include "Server.h"










int main()
{
    Server server;
    
    server.routine();
    //server.handle_client();
    
    
    

    //future<vector<vector<int>>> result = async(launch::async, [&]() {foo(matrix, 2); return matrix; });
    //std::promise<std::vector<std::vector<int>>> promise;
    //std::future<std::vector<std::vector<int>>> future = promise.get_future();
    
    
    
    //server.send(result.get());

    system("pause");
    return 0;
}

#pragma once
#pragma warning(disable: 4996)
#include <chrono>
#pragma comment(lib, "ws2_32.lib")
#include <future>
#include <winsock2.h>
#include <thread>
#include <functional>
#include "Parallel_matrix_processing.h"
#include <time.h>
#include <cstdlib>

template <typename T>
using Matrix = std::vector<std::vector<T>>;

class Server {
    WSAData wsaData;
    WORD DLLversion = MAKEWORD(2, 1);
    bool work = true;
    // Info about socket address
    SOCKADDR_IN addr;
    SOCKET sListen;
    int sizeofaddr;
public:
    Server();
    ~Server();

    template<typename T>
    void send_matrix(SOCKET socket, Matrix<T> matrix);

    template <typename T>//, typename = enable_if_t<is_fundamental_v<T>>>
    void receive_matrix(SOCKET socket, Matrix<T>& data, size_t size);

    void receive_configuration(SOCKET socket, std::vector<size_t>& configuration);
    bool ping_pong(SOCKET socket);
    void handle_client(SOCKET socket);
    void routine();
    bool receive_command(SOCKET socket, std::string desired_command, std::string& buf);
    bool receive_command(SOCKET socket, std::string desired_command);
    void send_command(SOCKET socket, std::string command);
};

Server::Server() {
    if (WSAStartup(DLLversion, &wsaData) != 0) {
        std::cout << "Error" << std::endl;
        exit(1);
    }
    sizeofaddr = sizeof(addr);

    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(1111);
    addr.sin_family = 2; //AF_INET
    sListen = socket(2, 1, 0); // 2 = AF_INET, 1 = SOCK_STREAM
    bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
    listen(sListen, SOMAXCONN);
    std::cout << "Server started. Waiting for connections..." << std::endl;

}

Server::~Server() {
    WSACleanup();
}

template<typename T>
void Server::send_matrix(SOCKET socket, Matrix<T> matrix) {
    if (socket == 0) {
        std::cout << "Error" << std::endl;
    }
    else {
        std::cout << "Data sent.\n";
        for (int i = 0; i < matrix.size(); i++) {
            send(socket, (char*)matrix[i].data(), sizeof(T) * matrix.size(), 0);
        }

    }
}

template <typename T>//, typename = enable_if_t<is_fundamental_v<T>>>
void Server::receive_matrix(SOCKET socket, Matrix<T>& data, size_t size) {
    if (socket == 0) {
        std::cout << "Error" << std::endl;
    }
    else {
        std::cout << "Received data!\n";
        data.resize(size);
        for (int i = 0; i < size; i++) {
            data[i].resize(size);
            recv(socket, (char*)data[i].data(), sizeof(T) * size, 0);
        }

        //int size = data[0];
        //data.erase(data.begin());
        //data.resize(size);
    }
}

void Server::receive_configuration(SOCKET socket, std::vector<size_t>& configuration) {
    if (socket == 0) {
        std::cout << "Error" << std::endl;
        return;
    }
    configuration.resize(3);
    recv(socket, reinterpret_cast<char*>(configuration.data()), sizeof(size_t) * configuration.size(), 0);
    std::cout << "Received configuration!\n";
}

bool Server::ping_pong(SOCKET socket) {
    std::cout << "Waiting for PING\n";
    if (this->receive_command(socket, "PING")) {
        this->send_command(socket, "PONG");
        std::cout << "Received PING. PONG sent." << std::endl;
        return true;
    }
    else {
        std::cout << "No PING received." << std::endl;
        return false;
    }
}

/*
* My protocol:
* 1. Ping/Pong session
* 2. Configuration -> server receives and waits for data
* 3. Data (matrix) -> server receives and waits for START command
* 4. START command -> server starts computing
* 5. STATUS command -> server receives command and sends current status ->
*    -> client receives status "1" (done) and sends GET command
* 6. GET command -> server sends data (matrix)
*/

void Server::handle_client(SOCKET socket) {
    std::thread worker; // Объявление внешней области видимости

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (!ping_pong(socket)) {
            break;
        }

        std::vector<std::vector<int>> matrix;
        std::vector<size_t> configuration;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        this->receive_configuration(socket, configuration);

        std::this_thread::sleep_for(std::chrono::seconds(1));
        this->receive_matrix(socket, matrix, configuration[0]);

        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::promise<decltype(matrix)> p;
        std::future<decltype(matrix)> f = p.get_future();

        if (this->receive_command(socket, "START")) {
            worker = std::thread([&]() { 
                foo(matrix, configuration[2]);
                srand(time(0));
                std::this_thread::sleep_for(std::chrono::seconds(rand() % 7 + 4));
                //done = true;
                p.set_value(matrix);
                });
        }

        

        while (true) {
            
            if (this->receive_command(socket, "STATUS")) {
                std::future_status status = f.wait_for(std::chrono::seconds(1));
                if (status == std::future_status::ready) {
                    while (true) {
                        this->send_command(socket, "1");
                        if (this->receive_command(socket, "GET")) {
                            break;
                        }
                    }
                    break;
                }
                else {
                    this->send_command(socket, "0");
                }
            }
        }

        worker.join(); 
        this->send_matrix(socket, matrix);   
    }
    closesocket(socket);
}



void Server::routine() {
    while (work) {
        SOCKET newConnection;
        newConnection = accept(sListen, (SOCKADDR*)&addr, &sizeofaddr);
        if (newConnection == ~0) {
            std::cerr << "Failed to accept client socket: " << WSAGetLastError() << std::endl;
            continue;
        }
        //bool done = false;
        std::thread client_thread(&Server::handle_client, this, newConnection);
        client_thread.detach();
    }
}

bool Server::receive_command(SOCKET socket, std::string desired_command) {
    std::string dummy_buffer;
    return this->receive_command(socket, desired_command, dummy_buffer);
}

bool Server::receive_command(SOCKET socket, std::string desired_command, std::string& buf) {
    std::vector<char> buffer(1024);
    recv(socket, (char*)buffer.data(), sizeof(char) * buffer.size(), 0);
    std::string received_command(std::move(buffer.data()));
    std::cout << "Command is " << received_command << std::endl;
    if (received_command.find(desired_command) != std::string::npos) {
        buf = received_command;
        return true;
    }
    return false;
}

void Server::send_command(SOCKET socket, std::string command) {
    send(socket, (char*)command.data(), sizeof(command), 0);
}
#pragma once
#pragma warning(disable: 4996)
#include <chrono>
#pragma comment(lib, "ws2_32.lib")
#include <future>
#include <winsock2.h>
#include <thread>
#include <functional>
#include "Parallel_matrix_processing.h"

template <typename T>
using Matrix = std::vector<std::vector<T>>;

class Server {
    // WSAStartup
    WSAData wsaData;
    WORD DLLversion = MAKEWORD(2, 1);
    bool work = true;
public:
    Server() {
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
    ~Server() { WSACleanup(); }
private:
    // Info about socket address
    SOCKADDR_IN addr;
    SOCKET sListen;
    int sizeofaddr;

public:
    template<typename T>
    void send_matrix(SOCKET socket, Matrix<T> matrix) {
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
    void receive_matrix(SOCKET socket, Matrix<T>& data, size_t size) {
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

    void receive_configuration(SOCKET socket, std::vector<size_t>& configuration) {
        if (socket == 0) {
            std::cout << "Error" << std::endl;
            return;
        }
        configuration.resize(3);
        recv(socket, reinterpret_cast<char*>(configuration.data()), sizeof(size_t) * configuration.size(), 0);
        std::cout << "Received configuration!\n";
    }

    bool ping_pong(SOCKET socket) {
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

    void handle_client(SOCKET socket, bool& done) {
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
            if (this->receive_command(socket, "START")) {
                foo(matrix, configuration[2]);
                std::this_thread::sleep_for(std::chrono::seconds(5));
                done = true;
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
            this->send_matrix(socket, matrix);

            //f = [&status]() {status = false; };
            //this->receive_command(socket, "SHUTDOWN", f);
        }
        closesocket(socket);
    }

    void routine() {
        while (work) {
            SOCKET newConnection;
            newConnection = accept(sListen, (SOCKADDR*)&addr, &sizeofaddr);
            if (newConnection == ~0) {
                std::cerr << "Failed to accept client socket: " << WSAGetLastError() << std::endl;
                continue;
            }
            bool done = false;
            std::thread client_thread(&Server::handle_client, this, newConnection, std::ref(done));
            client_thread.detach();
        }
    }

    bool receive_command(SOCKET socket, std::string desired_command, std::string& buf) {
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

    bool receive_command(SOCKET socket, std::string desired_command) {
        std::string dummy_buffer;
        return this->receive_command(socket, desired_command, dummy_buffer);
    }

    void send_command(SOCKET socket, std::string command) {
        send(socket, (char*)command.data(), sizeof(command), 0);
    }
};
#include "TcpConnection.h"
#include "Vulkan.h"
#include <exception>
#include <iostream>
#include <memory>

#include "EventLoop.h"
#include "Log.h"
#include "TcpClient.h"

void onConnection(const std::shared_ptr<TcpConnection>& conn) {
    try {
        Vulkan vulkan("Game", 800, 600);
        vulkan.tcpConnection_ = conn;
        vulkan.run();
    } catch (std::exception e) {
        std::cerr << e.what() << std::endl;
    }
}

void onMessage(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer) {

}

int main() {
    Log::Instance()->close();
    EventLoop loop;
    TcpClient client(&loop, "127.0.0.1", 9999);
    client.setReadCallback(onMessage);
    client.setOnConnection(onConnection);
    client.connect();
    loop.loop();
}
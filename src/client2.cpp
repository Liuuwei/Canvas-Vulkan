#include "TcpConnection.h"
#include "Vulkan.h"
#include <GLFW/glfw3.h>
#include <exception>
#include <iostream>
#include <memory>

#include "EventLoop.h"
#include "Log.h"
#include "TcpClient.h"

Vulkan vulkan("Game", 800, 600);

void onConnection(const std::shared_ptr<TcpConnection>& conn) {
    vulkan.tcpConnection_ = conn;
    while (!glfwWindowShouldClose(vulkan.windows_)) {
        vulkan.pollEvents();
    }
}

void onMessage(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer) {
    int n = -1;
    while ( (n = buffer->FindEnd()) != -1) {
        std::string msg = buffer->retriveSome(n - buffer->readIndex() + 1);
        vulkan.processNetWork(msg);
        vulkan.draw();
    }
    vulkan.draw();
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
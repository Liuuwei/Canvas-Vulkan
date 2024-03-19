#include "InetAddr.h"
#include "EventLoop.h"
#include "TcpServer.h"
#include <thread>
#include "Log.h"

void onMessage(const std::shared_ptr<TcpConnection>& conn, Buffer* buffer) {
    int n = -1;
    while ( (n = buffer->FindEnd()) != -1) {
        std::string msg = buffer->retriveSome(n - buffer->readIndex() + 1);
        for (auto other : conn->loop()->tcpConnections()) {
            auto player = other.second.lock();
            if (player) {
                player->send(msg);
            }
        } 
    }
}

void onConnection(const std::shared_ptr<TcpConnection>& conn) {
    
}

int main() {
    Log::Instance()->close();
    EventLoop loop;
    InetAddr addr("127.0.0.1", 9999);
    TcpServer server(&loop, addr);
    server.setOnMessage(onMessage);
    server.setOnConnection(onConnection);
    server.setThreadNums(1);
    server.start();
    loop.loop();
}
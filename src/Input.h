//
//  rtmp_relay
//

#pragma once

#include <random>
#include <vector>
#include "Socket.h"

class Input
{
public:
    enum class State
    {
        UNINITIALIZED = 0,
        VERSION_SENT = 1,
        ACK_SENT = 2,
        HANDSHAKE_DONE = 3
    };
    
    struct Init
    {
        uint32_t time;
        uint32_t zero;
        uint8_t randomBytes[1528];
    };
    
    struct Ack
    {
        uint32_t time;
        uint32_t time2;
        uint8_t randomBytes[1528];
    };
    
    Input() = default;
    Input(Network& network, Socket socket);
    ~Input();
    
    Input(const Input&) = delete;
    Input& operator=(const Input&) = delete;
    
    Input(Input&& other);
    Input& operator=(Input&& other);
    
    void update();
    
    bool getPacket(std::vector<uint8_t>& packet);
    
    bool isConnected() const { return _socket.isReady(); }
    
protected:
    void handleRead(const std::vector<uint8_t>& data);
    void handleClose();
    
    Network& _network;
    Socket _socket;
    
    std::vector<uint8_t> _data;
    
    State _state = State::UNINITIALIZED;
    
    std::random_device _rd;
    std::mt19937 _generator;
};

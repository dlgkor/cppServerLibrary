#pragma once
#include<sys/socket.h>
#include<arpa/inet.h>
#include"Packet.h"
#include"SafeQueue.h"

namespace core{
    class TCPInfo{
        public:
        int sockfd;
        sockaddr_in clientaddr;
        core::SafeQueue<core::Packet*> *taskque; //received packet is pushed
        core::SafeQueue<core::Packet*> sendque;
        bool wouldblock;
        public:
        TCPInfo(); //allocate sendque
        ~TCPInfo(); //free sendque

        //sendque의 경우에는 깊은 복사를 해야 한다
    };

    class UDPInfo{
        public:
        sockaddr_in clientaddr;
        core::SafeQueue<core::Packet*> *taskque;
        core::SafeQueue<core::Packet*> sendque;
        UDPInfo(); //allocate sendque
        ~UDPInfo(); //free sendque
    };

    //ClientInfo
    class ClientInfo{
        public:
        TCPInfo tcpinfo;
        UDPInfo udpinfo;

        core::PacketComposer packetComposer;
    };
}
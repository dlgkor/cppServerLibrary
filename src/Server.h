#pragma once
#include<vector>
#include<string>
#include<cstring>
#include<iostream>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/epoll.h>
#include<pthread.h>
#include<fcntl.h>
#include<map>

#include"Packet.h"
#include"ClientInfo.h"
#include"Debug.h"

#define TCP_BUF_SIZE 1024


namespace core{
    class Server{
        private:
        sockaddr_in TCPserverAdress;
        sockaddr_in UDPserverAdress;
        int tcpSocket; //socket for listening
        int udpSocket; 
        
        std::map<int, core::ClientInfo*> clientDictionary; 
        //pthread_mutex_t server_mtx;
        //key: tcpfd, value:clientinfo
        //udp info is set by udpinfoset packet

        int epollfd;

        core::SafeQueue<core::Packet*> *defaultTCPTaskQ;
        core::SafeQueue<core::Packet*> *defaultUDPTaskQ;

        pthread_t tcpLoopThread;
        pthread_t tcpSendThread;
        pthread_t udpLoopThread;
        pthread_t udpSendThread;
        
        public:
        Server(core::SafeQueue<core::Packet*>*, core::SafeQueue<core::Packet*>*); //make socket

        int BindTCP(std::string, int);
        int BindUDP(std::string, int);

        int Listen(); //start listening(TCP)
        void TCPLoop(); //recv and push to queue(thread)
        void TCPEventHandler(epoll_event&);
        void TCPSend(); //pop queue and send(thread)
        void SendTCPPacket(int, core::Packet*);

        void UDPLoop(); //recv and push to queue(thread)
        void UDPEventHandler(epoll_event&);
        void UDPSend(); //pop queue and send(thread)
        void SendUDPPacket(int, core::Packet*);

        static void* StartTCPLoop(void*); //func for making thread
        static void* StartTCPSend(void*);
        static void* StartUDPLoop(void*);
        static void* StartUDPSend(void*);

        void StartTCPLoopThread();
        void StartTCPSendThread();
        void StartUDPLoopThread();
        void StartUDPSendThread();

        ~Server(); //close socket
        private:
        void TCPListenEvent(epoll_event&);
        void TCPCloseEvent(epoll_event&);
        void TCPInEvent(epoll_event&); 
        void TCPLetEvent(epoll_event&); //deprecated
        void TCPOutEvent(epoll_event&);
    };
}
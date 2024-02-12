#include"ClientInfo.h"

core::TCPInfo::TCPInfo():taskque(nullptr), wouldblock(false){}
core::TCPInfo::~TCPInfo(){
    while(!sendque.IsEmpty()){
        core::Packet* _packet;
        sendque.Pop(_packet);
        delete _packet;
    }
}

core::UDPInfo::UDPInfo():taskque(nullptr){}
core::UDPInfo::~UDPInfo(){
    while(!sendque.IsEmpty()){
        core::Packet* _packet;
        sendque.Pop(_packet);
        delete _packet;
    }
}
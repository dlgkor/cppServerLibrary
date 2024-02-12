#include"Packet.h"

core::Packet::Packet():packetID(0),dataSize(0),recv_head(false),readCur(0){
    memset(data,0,MAX_DATA_SIZE);
}

char* core::Packet::Serialize(){
    char* _packet = new char[sizeof(packetID)+sizeof(dataSize)+dataSize];
    memcpy(_packet, &packetID, sizeof(packetID));
    memcpy(_packet+sizeof(packetID),&dataSize,sizeof(dataSize));
    memcpy(_packet+sizeof(packetID)+sizeof(dataSize),data,dataSize);
    return _packet;
}

uint16_t core::Packet::Length(){
    return sizeof(packetID)+sizeof(dataSize)+dataSize;
}

//function that parse char,int from data(readCur is used)

core::Packet::~Packet(){}
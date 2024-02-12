#pragma once
#include<arpa/inet.h>
#include<memory.h>
#include<iostream>
#include<stdint.h>
#include<cstring>

#include"Debug.h"

namespace core{
    const int MAX_DATA_SIZE = 4096;
    const int MAX_CLIP_SIZE = 8192;

    class Packet{
    public:
        //header
        //uint32_t start_code; //discard byte until it matches start code
        uint16_t packetID; //correspond to functionname
        uint16_t dataSize;
        bool recv_head;

        //body
        char data[MAX_DATA_SIZE]; 
        //uint32_t end_code; //if end code doesn't match discard packet
        
        uint32_t readCur;

        int clientid; //same as tcp socket fd
    public:
        Packet();
        //function that parse char,int from data(readCur is used)
        char* Serialize();
        uint16_t Length();
        ~Packet();
    };

    class PacketComposer{
        //TCP Packet composer
    private:
        Packet* tempPacket;
        char clipPacket[MAX_CLIP_SIZE];
        uint32_t clipPacketSize;
        uint32_t clipReadCur;
    public:
        PacketComposer();
        int addClip(char*, int);
        Packet* Compose();
        void ClearHandledData();
        ~PacketComposer();
    };
}
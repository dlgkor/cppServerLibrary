#include"Packet.h"

core::PacketComposer::PacketComposer():tempPacket(nullptr),clipPacketSize(0),clipReadCur(0){}

int core::PacketComposer::addClip(char* src, int srcSize){
    //Add to clip packet
    LOG_VAR(srcSize);
    LOG_VAR(clipPacketSize);
    if(MAX_CLIP_SIZE < clipPacketSize + srcSize){
        std::cerr<<"ClipPacket size overflow error\n";
        return 0;
    }
    memcpy(clipPacket+clipPacketSize, src, srcSize);
    clipPacketSize += srcSize;

    //clipPacket[clipPacketSize] = '\0';

    return 1;
}

core::Packet* core::PacketComposer::Compose(){
    if(clipPacketSize==0)
            return nullptr;
    //Only Compose with clip packet
    //return nullptr if packet compose is incompleted
    //tcp recv func loop until this fuction returns nullptr
    if(!tempPacket)
        tempPacket = new Packet();
    
    if(!tempPacket->recv_head){
        if(clipPacketSize - clipReadCur < 4)
            return nullptr;

        memcpy(&tempPacket->packetID, clipPacket, 2);
        memcpy(&tempPacket->dataSize, clipPacket + 2, 2);
        LOG_VAR(tempPacket->packetID);
        LOG_VAR(tempPacket->dataSize);

        clipReadCur += 4;

        tempPacket->recv_head = true;
    }

    LOG_VAR(tempPacket->dataSize);

    if(clipPacketSize-clipReadCur < tempPacket->dataSize)
        return nullptr;

    memcpy(&tempPacket->data, clipPacket+clipReadCur, tempPacket->dataSize);
    clipReadCur += tempPacket->dataSize;

    //ClearHandledData()

    core::Packet* _tempPacket = tempPacket;
    tempPacket = nullptr;
    return _tempPacket;
}

void core::PacketComposer::ClearHandledData(){
    //버퍼 용량 넘어가지 않도록 버퍼 데이터를 앞으로 땡겨줌
    if(clipReadCur == 0)
        return;

    if(clipPacketSize-clipReadCur != 0)
        memmove(clipPacket, clipPacket+clipReadCur, clipPacketSize-clipReadCur); //copy using buffer
    
    clipPacketSize -= clipReadCur;
    clipReadCur = 0;
}

core::PacketComposer::~PacketComposer(){
    if(tempPacket)
        delete tempPacket;
}
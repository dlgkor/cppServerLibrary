#include<pthread.h>
#include<string>

#include"Server.h"

//WSL2 IP
//(cmd) wsl hostname -I

#define MYIP "127.0.0.1"
#define MYPORT 8888
//localhost에 포트번호 8888로 통일하니까 잘 된다
//vscode가 자동으로 localhost 8888포트를 호스트와 통일해 준다
//ufw 방화벽 내릴 필요 없음

int main(int argc, char* argv[]){
    core::SafeQueue<core::Packet*> *defaultQ = new core::SafeQueue<core::Packet*>();
    core::Server myServer(defaultQ, nullptr);

    if(argc==1){
        myServer.BindTCP(MYIP, MYPORT);
        myServer.BindUDP(MYIP, MYPORT);
    }
    else{
        myServer.BindTCP(std::string(argv[1]), std::atoi(argv[2]));
        myServer.BindUDP(std::string(argv[1]), std::atoi(argv[2]));
    }
    
    
    myServer.Listen();
    myServer.StartTCPLoopThread();
    myServer.StartTCPSendThread();

    const char* hc = "hello client";
    
    while(1){
        if(defaultQ->IsEmpty())
            continue;
        core::Packet* mypacket;
        defaultQ->Pop(mypacket);
        std::cout<<"packet: " << mypacket->data << std::endl;
        int id = mypacket->clientid;
        delete mypacket;
        
        mypacket = new core::Packet();
        mypacket->packetID = 1;
        strcpy(mypacket->data, hc);
        mypacket->dataSize = strlen(mypacket->data);
        myServer.SendTCPPacket(id, mypacket);
    }

    delete defaultQ;
    
    return 0;
}
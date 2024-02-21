#include<pthread.h>
#include<string>

#include"Server.h"

//WSL2 IP
//(cmd) wsl hostname -I

//#define MYIP "127.0.0.1"
#define MYIP "172.20.35.196"
#define MYPORT 8888
//localhost에 포트번호 8888로 통일하니까 잘 된다
//vscode가 자동으로 localhost 8888포트를 호스트와 통일해 준다
//ufw 방화벽 내릴 필요 없음

int main(int argc, char* argv[]){
    core::SafeQueue<core::Packet*> *userQ = new core::SafeQueue<core::Packet*>();
    core::Server myServer(userQ, userQ);
    //동일한 taskQ를 설정해줄 수 있다

    if(argc==1)
        myServer.Bind(MYIP, MYPORT); //Bind TCP and UDP
    else
        myServer.Bind(std::string(argv[1]), std::atoi(argv[2]));
    
    
    myServer.Listen();
    myServer.StartTCPLoopThread();
    myServer.StartTCPSendThread();
    myServer.StartUDPLoopThread();
    myServer.StartUDPSendThread();

    const char* hc = "hello client";
    int id = 0;
    
    while(1){
        id=0;
        if(userQ->IsEmpty())
            continue;
        core::Packet* mypacket;
        userQ->Pop(mypacket);
        if(mypacket->packetID == 0x1001){
            std::cout<<"packet: " << mypacket->data << std::endl;
            id = mypacket->clientid;
        }
        delete mypacket;
        
        mypacket = new core::Packet();
        mypacket->packetID = 0x1001;
        strcpy(mypacket->data, hc);
        mypacket->dataSize = strlen(mypacket->data);
        myServer.SendUDPPacket(id, mypacket);
        usleep(1);
    }

    delete userQ;
    
    return 0;
}
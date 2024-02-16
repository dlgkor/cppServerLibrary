#include"Server.h"

core::Server::Server(core::SafeQueue<core::Packet*>* TCPTaskQ, core::SafeQueue<core::Packet*>* UDPTaskQ){
    tcpSocket = socket(AF_INET, SOCK_STREAM, 0);
    udpSocket = socket(AF_INET, SOCK_DGRAM, 0);

    epollfd = epoll_create(100);

    userTCPTaskQ = TCPTaskQ;
    userUDPTaskQ = UDPTaskQ;

    tcpLoopThread = 0;
    tcpSendThread = 0;
    udpLoopThread = 0;
    udpSendThread = 0;

    //pthread_mutex_init(&server_mtx, NULL);
}

int core::Server::BindTCP(std::string serverIP, int serverPORT){
    std::memset(&TCPserverAdress, 0, sizeof(TCPserverAdress));
    TCPserverAdress.sin_family = AF_INET;

    //TCPserverAdress.sin_addr.s_addr = htonl(INADDR_ANY);
    
    if(inet_pton(AF_INET, serverIP.c_str(), &(TCPserverAdress.sin_addr)) < 1){
        std::cerr<<"[TCP] Invalid IP adress "<<serverIP<<std::endl;
        return 0;
    }

    TCPserverAdress.sin_port = htons(serverPORT);

    if(bind(tcpSocket, reinterpret_cast<sockaddr*>(&TCPserverAdress), sizeof(TCPserverAdress)) < 0){
        std::cerr<<"[TCP] Error binding socket\n";
        return 0;
    }

    std::cout<<"[TCP] socket is bined to "<<serverIP<<":"<<serverPORT<<std::endl;

    return 1;
}

int core::Server::BindUDP(std::string serverIP, int serverPORT){
    //udp is not binded
    
    std::memset(&UDPserverAdress, 0, sizeof(UDPserverAdress));
    UDPserverAdress.sin_family = AF_INET;

    if(inet_pton(AF_INET, serverIP.c_str(), &(UDPserverAdress.sin_addr)) < 1){
        std::cerr<<"[UDP] Invalid IP adress\n";
        return 0;
    }

    UDPserverAdress.sin_port = htons(serverPORT);

    if(bind(udpSocket, reinterpret_cast<sockaddr*>(&UDPserverAdress), sizeof(UDPserverAdress)) < 0){
        std::cerr<<"[UDP] Error binding socket\n";
        return 0;
    }

    std::cout<<"[UDP] socket is bined to "<<serverIP<<":"<<serverPORT<<std::endl;

    return 1;
}

int core::Server::Bind(std::string serverIP, int serverPORT){
    //Bind TCP and UDP Socket
    BindTCP(serverIP,serverPORT);
    BindUDP(serverIP,serverPORT);
    //BindUDP(serverIP,9000);
}

int core::Server::Listen(){
    if(listen(tcpSocket, 10) == -1){
        std::cerr<<"[TCP] Error listening on socket\n";
        return 0;
    }

    std::cout<<"[TCP] socket is listening\n";

    return 1;
}

void core::Server::TCPLoop(){
    const int epoll_size = 100;
    epoll_event ev, events[epoll_size];
    int event_count = 0;

    ev.events = EPOLLIN;
    ev.data.fd = tcpSocket;
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, tcpSocket, &ev)==-1){
        std::cerr<<"[TCP] Socket epoll_ctl error\n";
    }

    LOG("[TCP] loop is enabled\n");

    while (1)
    {
        LOG("[TCP] epoll wait start\n");
        event_count = epoll_wait(epollfd, events, epoll_size, -1);
        LOG("[TCP] epoll wait over\n");
        if(event_count == -1){
            std::cerr<<"[TCP] epoll wait error\n";
            continue;
        }
        for(int i=0;i<event_count;i++){
            LOG("[TCP] Handling Event\n");
            TCPEventHandler(events[i]);
        }
    }    
}

void core::Server::TCPEventHandler(epoll_event& event){
    if(event.data.fd == tcpSocket && event.events & EPOLLIN){
        LOG("[TCP] Socket event occured\n");
        TCPListenEvent(event);
    }
    else{
        if(event.events & EPOLLRDHUP){
            //recv가 0을 반환하는 경우도 상대방이 연결을 끊었다는 의미이므로 처리해야 한다
            //출처: https://stackoverflow.com/questions/6437879/how-do-i-use-epollhup
            LOG("[TCP] EPOLLRDHUP event occured\n");
            TCPCloseEvent(event);
            return; 
        }
        if(event.events & EPOLLIN || event.events & EPOLLET){
            if(event.events & EPOLLIN)
                LOG("[TCP] EPOLLIN event occured\n");
            if(event.events & EPOLLET)
                LOG("[TCP] EPOLLET event occured\n");

            TCPLetEvent(event);
        }

        if(event.events & EPOLLOUT){
            LOG("[TCP] EPOLLOUT event occured\n");
            //exception 처리 추가하기
            TCPOutEvent(event); //pop sendque and write
        }
    }
}

void core::Server::TCPSend(){
    //데이터를 보내다가 wouldblock 된다면 epoll_ctl_mod로 epollout 플래그를 이벤트에 추가
    //epollout 이벤트 발생 시에 데이터를 마저 보낸다
    //이때 wouldblock이 뜨면 그냥 나온다
    //epollout 플래그가 설정되있기 때문에 버퍼가 비면 다시 이벤트가 발생하기 때문이다
    //만약 데이터를 전부 보내게 된다면 epollout 플래그를 삭제한다(epoll_ctl_mod)
    epoll_event ev;
    ev.events = EPOLLIN | EPOLLET | EPOLLOUT;

    while(1){
        if(clientDictionary.empty())
            continue;
        
        for(auto it=clientDictionary.begin();it!=clientDictionary.end();it++){
            //만약 이 루프가 도는 중에 client가 삭제되면 무슨 일이 일어날까?
            //clientDictonary mutex를 만들어서 clientDictionary 순회 루프를 포장해야 한다

            //clientDictionary에서 client 삭제 요청 패킷 처리?

            if(it->second->tcpinfo.sendque.IsEmpty())
                continue;

            core::Packet* _packet;
            it->second->tcpinfo.sendque.Seek(_packet);

            if(it->second->tcpinfo.wouldblock)
                continue;            

            char* packetBytes = _packet->Serialize();
            uint16_t packetSize = _packet->Length();

            int sendN = send(it->second->tcpinfo.sockfd, packetBytes, packetSize, 0);
            LOG_VAR(it->second->tcpinfo.sockfd);
            LOG_VAR(packetSize);

            if(sendN < 0){
                if(errno == EWOULDBLOCK){
                    it->second->tcpinfo.wouldblock = true;
                    ev.data.fd = it->second->tcpinfo.sockfd;
                    epoll_ctl(epollfd, EPOLL_CTL_MOD, it->second->tcpinfo.sockfd, &ev);
                }
            }
            else{
                if(sendN != packetSize)
                    std::cerr<<"[TCP] send size and packet size dismatch Error\n";
                it->second->tcpinfo.sendque.Pop(_packet);
                delete _packet;
                LOG_ENDL("[TCP] successfully send data to client");
            }

            delete packetBytes;
        }

        usleep(1);
    }
}

void core::Server::SendTCPPacket(int clientid, core::Packet* _packet){
    //pthread_mutex_lock(&server_mtx);
    std::map<int, core::ClientInfo*>::iterator it = clientDictionary.find(clientid);
    if(it==clientDictionary.end()){
        std::cerr<<"[TCP] packet receiver connection is already closed\n";
        delete _packet;
        return;
    }
    
    //try-catch
    //if catch delete packet
    it->second->tcpinfo.sendque.Push(_packet);
    //pthread_mutex_unlock(&server_mtx);
}

void core::Server::TCPListenEvent(epoll_event& event){
    //Accept new client
    sockaddr_in clientaddr;
    socklen_t socksize = sizeof(sockaddr_in);
    int sockfd = accept4(tcpSocket, reinterpret_cast<sockaddr*>(&clientaddr), &socksize, SOCK_NONBLOCK);
    if(sockfd == -1){
        std::cerr<<"[TCP] Error Accepting socket\n";
        return;
    }
    LOG("[TCP] accepted fd ");
    LOG_ENDL(sockfd);

    ClientInfo* clientinfo = new ClientInfo(); //deleted at TCPhup event 

    clientinfo->tcpinfo.sockfd = sockfd;
    clientinfo->tcpinfo.clientaddr = clientaddr;
    clientinfo->tcpinfo.taskque = userTCPTaskQ;
    clientinfo->udpinfo.taskque = userUDPTaskQ;
    clientDictionary.insert({sockfd, clientinfo});

    epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    
    ev.data.fd = sockfd;

    //https://reakwon.tistory.com/236
    //clientinfo 정보가 담긴 첫 패킷을 받아야 clientlist에 등록되도록 만들까?
    
    if(epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev)==-1){
        std::cerr<<"[TCP] new client epoll_ctl error\n";
        return;
    }

    OnFirstConnect(sockfd);
}

void core::Server::TCPCloseEvent(epoll_event& event){
    //connection is closed
    //erase socket event
    //pthread_mutex_lock(&server_mtx);
    LOG("[TCP] CloseEvent occured\n");
    
    //sendque에 TCPClose 패킷을 넣고 처리해볼까?

    int sockfd = event.data.fd;
    //int sockfd = static_cast<core::ClientInfo*>(event.data.ptr)->tcpinfo.sockfd;

    epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, NULL); //https://reakwon.tistory.com/236

    delete clientDictionary[sockfd];
    clientDictionary.erase(sockfd);
    close(sockfd); //close tcp socket connected to client
    //pthread_mutex_unlock(&server_mtx);
}

void core::Server::TCPInEvent(epoll_event& event){
    //deprecated
    TCPLetEvent(event);
}

void core::Server::TCPLetEvent(epoll_event& event){
    //한번 이벤트가 발생하면 recv가 eagain 오류를 반환할 때까지 전부 읽어들여야 한다
    //그렇게 읽어들인 데이터 스트림으로 패킷을 조립하자
    //만약 패킷이 완성되고 남는 데이터가 있다면 그 데이터는 다음 패킷 조립 앞부분에 사용한다
    //패킷 조립 클래스 만들 예정

    char buffer[TCP_BUF_SIZE];
    int readN = 0;

    //core::ClientInfo* clientInfo = static_cast<core::ClientInfo*>(event.data.ptr);
    core::ClientInfo* clientInfo = clientDictionary[event.data.fd];

    while(1){
        //readN = recv(clientInfo->tcpinfo.sockfd, buffer, TCP_BUF_SIZE, 0);
        readN = recv(event.data.fd, buffer, TCP_BUF_SIZE, 0);

        if(readN < 0){
            if(errno == EAGAIN)
                break;
        }
        if(readN == 0){
            TCPCloseEvent(event);
            break;
        }

        LOG("[TCP] received data\n");
        clientInfo->packetComposer.addClip(buffer, readN);
        while(1){
            LOG("[TCP] looping\n");
            core::Packet* tempPacket = clientInfo->packetComposer.Compose();
            if(!tempPacket)
                break;

            if(!clientInfo->tcpinfo.taskque){
                std::cerr<<"[TCP] None taskque error\n";
                delete tempPacket;
                continue;
            }
        
            tempPacket->clientid = event.data.fd;
            clientInfo->tcpinfo.taskque->Push(tempPacket);
        }
        clientInfo->packetComposer.ClearHandledData();
    }
}

void core::Server::TCPOutEvent(epoll_event& event){
    //클라이언트들의 queue에 쌓여있는 패킷이 있는지 확인하고 보내는 쓰레드(TcpSend func)가 따로 있다
    //이 함수에서 epollout 이벤트 발생을 확인하면 해당 쓰레드에 그 사실을 통지하기만 한다
    //만약 bool 자료형을 사용한다면 쓰레드는 해당사실을 확인 하자마자 bool을 false로 바꾸고 작업을 수행해야 한다
    std::map<int, core::ClientInfo*>::iterator it = clientDictionary.find(event.data.fd);
    if(it==clientDictionary.end()){
        std::cerr<<"[TCP] packet receiver connection is already closed\n";
        return;
    }

    epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = event.data.fd;
    epoll_ctl(epollfd, EPOLL_CTL_MOD, event.data.fd, &ev);

    it->second->tcpinfo.wouldblock = false;
}

void core::Server::OnFirstConnect(int _clientid){
    //Send ClientID to client by TCP
    //tcp 첫 연결 시에 호출
    core::Packet* _packet = new core::Packet();
    _packet->packetID = 0x1000;
    _packet->dataSize = sizeof(int32_t);
    memcpy(_packet->data, &_clientid, sizeof(int32_t));
    SendTCPPacket(_clientid, _packet);
}

void core::Server::UDPLoop(){
    char buffer[UDP_BUF_SIZE];
    sockaddr_in client_addr;
    int client_addr_size = sizeof(client_addr);

    while(1){
        LOG("[UDP] loop start\n");
        int packet_len = recvfrom(udpSocket, buffer, UDP_BUF_SIZE, 0, reinterpret_cast<sockaddr*>(&client_addr), (socklen_t*)&client_addr_size);
        LOG("[UDP] received udp packet\n");

        core::Packet* tempPacket = new core::Packet();
        memcpy(&tempPacket->clientid, buffer, sizeof(int32_t)); //client library is required to add clientID at packet header
        memcpy(&tempPacket->packetID, buffer+sizeof(int32_t), sizeof(uint16_t));
        memcpy(&tempPacket->dataSize, buffer+sizeof(int32_t)+sizeof(uint16_t), sizeof(uint16_t));
        if(packet_len < sizeof(int32_t)+sizeof(uint16_t)+sizeof(uint16_t)+tempPacket->dataSize){
            std::cerr<<"[UDP] packet loss error\n";
            delete tempPacket;
            continue;
        }

        memcpy(&tempPacket->data[0],buffer+sizeof(int32_t)+sizeof(uint16_t)+sizeof(uint16_t), tempPacket->dataSize);

        std::map<int, core::ClientInfo*>::iterator it = clientDictionary.find(tempPacket->clientid);
        if(it==clientDictionary.end() || !it->second->udpinfo.taskque){
            if(it==clientDictionary.end())
                std::cerr<<"[UDP] cannot find clientinfo that obtains clientid\n";
            else if(!it->second->udpinfo.taskque)
                std::cerr<<"[UDP] None taskque error\n";

            delete tempPacket;
            continue;
        }

        //need to register udpinfo by tcp communication    
        
        //check if it matches with registered ip and port
        if(it->second->tcpinfo.clientaddr.sin_addr.s_addr==client_addr.sin_addr.s_addr && it->second->tcpinfo.clientaddr.sin_port == client_addr.sin_port){
            it->second->udpinfo.taskque->Push(tempPacket);
        }
        else{
            delete tempPacket;
            std::cerr<<"[UDP] client udp info(IP,PORT) mismatch with registered tcp info(IP,PORT)\n";
        }
    }   
}

void core::Server::UDPSend(){
    while(1){
        if(clientDictionary.empty())
            continue;

        for(auto it=clientDictionary.begin();it!=clientDictionary.end();it++){
            if(it->second->udpinfo.sendque.IsEmpty())
                continue;
            
            core::Packet* _packet;
            it->second->udpinfo.sendque.Pop(_packet);

            char* packetBytes = _packet->Serialize();
            uint16_t packetSize = _packet->Length();
            
            int send_len = sendto(udpSocket, packetBytes,packetSize,0,reinterpret_cast<sockaddr*>(&it->second->tcpinfo.clientaddr),
                sizeof(it->second->tcpinfo.clientaddr));
            
            LOG_ENDL("[UDP] send packet");
            LOG_VAR(send_len);

            delete packetBytes;
        }
        usleep(1);
    }
}

void core::Server::SendUDPPacket(int clientid, core::Packet* _packet){
    //pthread_mutex_lock(&server_mtx);
    std::map<int, core::ClientInfo*>::iterator it = clientDictionary.find(clientid);
    if(it==clientDictionary.end()){
        std::cerr<<"[UDP] packet receiver connection is already closed\n";
        delete _packet;
        return;
    }
    
    //try-catch
    //if catch delete packet
    it->second->udpinfo.sendque.Push(_packet);
}

core::Server::~Server(){
    close(tcpSocket);
    close(udpSocket);

    for(auto it=clientDictionary.begin();it!=clientDictionary.end();it++)
        delete it->second;
    clientDictionary.clear();
    //pthread_mutex_destroy(&server_mtx);
}

void* core::Server::StartTCPLoop(void* server){
    static_cast<core::Server*>(server)->TCPLoop();
    return nullptr;
}
void* core::Server::StartTCPSend(void* server){
    static_cast<core::Server*>(server)->TCPSend();
    return nullptr;
}
void* core::Server::StartUDPLoop(void* server){
    static_cast<core::Server*>(server)->UDPLoop();
    return nullptr;
}
void* core::Server::StartUDPSend(void* server){
    static_cast<core::Server*>(server)->UDPSend();
    return nullptr;
}

void core::Server::StartTCPLoopThread(){
    pthread_create(&tcpLoopThread, NULL, core::Server::StartTCPLoop, (void*)this);
    std::cout<<"create tcpLoopthread: "<<tcpLoopThread<<std::endl;
}
void core::Server::StartTCPSendThread(){
    pthread_create(&tcpSendThread, NULL, core::Server::StartTCPSend, (void*)this);
    std::cout<<"create tcpSendthread: "<<tcpSendThread<<std::endl;
}
void core::Server::StartUDPLoopThread(){
    pthread_create(&udpLoopThread, NULL, core::Server::StartUDPLoop, (void*)this);
    std::cout<<"create udpLoopthread: "<<udpLoopThread<<std::endl;
}
void core::Server::StartUDPSendThread(){
    pthread_create(&udpSendThread, NULL, core::Server::StartUDPSend, (void*)this);
    std::cout<<"create udpSendthread: "<<udpSendThread<<std::endl;
}
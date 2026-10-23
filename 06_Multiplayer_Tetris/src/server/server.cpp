#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <utility>
#include <string>
#include <chrono>
#include <algorithm>
#include <thread> // 멀티 스레드로 클라이언트 관리
#include <mutex>
#pragma comment(lib, "Ws2_32.lib")
using namespace std;

#pragma pack(push, 1) // tcp 스트림 특성 때문에 데이터를 1바이트 단위로 정렬
struct playerstate { // 플레이어 정보 수신 구조체
    int playerid = 0; // 플레이어 id
    bool gm_play = true; // 게임 플레이 플래그
    int score = 0; // 점수
    bool gm_map[20][10] = {}; // 게임 맵
    int x = 0, y = 0; // 현재 블록 xy좌표
    bool cpd_tp[4][4] = {}; // 현재 블록 상태
};
#pragma pack(pop)

#pragma pack(push, 1) // tcp 스트림 특성 때문에 데이터를 1바이트 단위로 정렬
struct header { // 송수신을 위해 먼저 처리할 헤더
    uint8_t type; // 0 : playerstate, 1 : 채팅, 2 : discon, 3 : 자신 id 수신
    uint32_t length; // 데이터 길이
};
#pragma pack(pop)

vector<bool> id(4, false); // 사용중인 아이디 확인 (최대 4명)
mutex smtx; // send를 처리하기 위한 mtx
mutex clientmtx; // 각 클라이언트 소켓을 담아놓은 백터를 관리하기 위한 lock

vector<SOCKET> clients; // 클라이언트 소켓 백터
vector<thread> clienthandle;

// 반복 recv (TCP 스트림 특성 때문에 사용)
int recv_all(SOCKET sock, char* buf, int len, int timeout_ms) {
    int total = 0;
    while (total < len) {
        fd_set readfds; // 소켓(또는 파일) 목록을 담는 구조체. select 함수에 넘기면 어떤 소켓이 읽기/쓰기/에러 상태인지 알려줌 (지금은 읽기)
        FD_ZERO(&readfds); // readfds 안을 0으로 초기화(아직 감지할 소켓 없음)
        FD_SET(sock, &readfds); // sock을 readfds 집합에 추가(sock 소켓이 읽기 가능한지 감시)

        timeval tv{}; // 최대 얼마 동안 기다릴지를 설정
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        int ret = select(0, &readfds, nullptr, nullptr, &tv);
        if (ret < 0) {
            return -1; // select error
        }
        if (ret == 0) {
            // timeout: 타임아웃으로 알려줌
            return -2;
        }
        //select()가 리턴된 뒤에는, readfds 안에 읽기 가능한 소켓만 남음
        if (FD_ISSET(sock, &readfds)) { // 그 소켓이 정말로 준비되었다면 recv 호출
            int bytes = recv(sock, buf + total, len - total, 0);
            if (bytes > 0) {
                total += bytes; // 이번에 받은 값 추가
                continue;
            }
            else if (bytes == 0) {
                return 0; // 원격 닫힘
            }
            else {
                return -1; // recv 에러
            }
        }
    }
    return total;
}
// 반복 send (TCP 스트림 특성 때문에 사용)
int send_all(SOCKET s, const char* buf, int len, int timeout_ms) {
    int total = 0;
    while (total < len) {
        fd_set writefds; // 소켓(또는 파일) 목록을 담는 구조체. select 함수에 넘기면 어떤 소켓이 읽기/쓰기/에러 상태인지 알려줌 (지금은 쓰기)
        FD_ZERO(&writefds); // writefds 안을 0으로 초기화(아직 감지할 소켓 없음)
        FD_SET(s, &writefds); // s를 writefds 집합에 추가(sock 소켓이 쓰기 가능한지 감시)

        timeval tv{}; // 최대 얼마 동안 기다릴지를 설정
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;

        int ret = select(0, nullptr, &writefds, nullptr, &tv);
        if (ret < 0) return -1; // select error
        if (ret == 0) {
            // timeout: 아직 준비 안됨
            if (total == 0) return -2;
            else continue;

        }
        //select()가 리턴된 뒤에는, writefds 안에 쓰기 가능한 소켓만 남음
        if (FD_ISSET(s, &writefds)) { // 그 소켓이 정말로 준비되었다면 send 호출
            int n = send(s, buf + total, len - total, 0);
            if (n > 0) {
                total += n; // 이번에 보낸 값 추가
                continue;
            }
            else if (n == 0) {
                return 0;
            }
            else {
                return -1;
            }
        }
    }
    return total;
}

void clientthread(SOCKET clientsoc, int id_num, vector<SOCKET>& clients, mutex& smtx, mutex& clientmtx) {
    playerstate ps{};
    ps.playerid = id_num;
    OutputDebugStringA("클라이언트 스레드 연결\n");
    {
        lock_guard<mutex> lock(clientmtx);
        cout << "broadcast from client " << id_num << " to " << clients.size() << "\n" << flush;
    }
    while (true) {
        header hd;
        int bytes = recv_all(clientsoc, (char*)&hd, sizeof(hd), 1000); // 타입과 tcp 길이 수신
        if (bytes > 0) {
            cerr << "[CLIENT]   " << id_num << "   got header : type = " << (int)hd.type << " length = " << hd.length << "\n";
        }
        else {
            cerr << "[CLIENT] "<< id_num << "   recv header returned " << bytes << " err=" << WSAGetLastError() << "\n";
            if (bytes == -2) {
                this_thread::sleep_for(chrono::milliseconds(1));
                continue;
            }
            cerr << "[CLIENT] " << id_num << "   connection closed (ret=" << bytes << ")\n";
            break;
        }

        vector<SOCKET> tempclients; // 클라이언트 백터 복사

        {
            lock_guard<mutex> lock(clientmtx);
            tempclients = clients;
        }


        if (hd.type == 1) { // 채팅
            string msg(hd.length, 0);
            int bytes = recv_all(clientsoc, &msg[0], hd.length,500);
            if (bytes > 0) {

            }
            else {
                cerr << "[CLIENT] recv header returned " << bytes << " err=" << WSAGetLastError() << "\n";
                break;
            }

            int i = 0;
            for (auto& sd : tempclients) {
                ++i;
                if (sd == clientsoc) continue;


                // 헤더 전송
                int retry = 3;
                while (retry--) {
                    lock_guard<mutex> lock(smtx);
                    int bytes = send_all(sd, (char*)&hd, sizeof(hd), 1000);
                    if (bytes >= 0) break; // 성공
                    if (bytes == -2) {
                        cerr << "클라이언트 " << i << "에게 채팅 헤더 송신 타임아웃, 재시도 " << 3 - retry << "\n";
                        this_thread::sleep_for(chrono::milliseconds(1));
                        continue;
                    }
                    cerr << "[SERVER] send header to socket " << clientsoc << " 실패: ret=" << bytes
                        << " err=" << WSAGetLastError() << "\n";
                    break;
                }

                // 메시지 데이터 전송
                retry = 3;
                while (retry--) {
                    lock_guard<mutex> lock(smtx);
                    int bytes = send_all(sd, msg.data(), hd.length, 500);
                    if (bytes >= 0) break; // 성공
                    if (bytes == -2) {
                        cerr << "클라이언트 " << i << "에게 채팅 데이터 송신 타임아웃, 재시도 " << 3 - retry << "\n";
                        this_thread::sleep_for(chrono::milliseconds(1));
                        continue;
                    }
                    cerr << "[SERVER] send message to socket " << clientsoc << " 실패: ret=" << bytes
                        << " err=" << WSAGetLastError() << "\n";
                    break;
                }
            }
        }
        else if(hd.type == 0) { // playerstate
            int bytes = recv_all(clientsoc, (char*)&ps, hd.length,500);
            if (bytes > 0) {
                cerr << "[CLIENT]   " << id_num <<"   got playerstate" << bytes << "\n";
            }
            else {
                cerr << "[CLIENT] recv header returned " << bytes << " err=" << WSAGetLastError() << "\n";
            }

            int i = 0;
            for (auto& sd : tempclients) {
                ++i;
                if (sd == clientsoc) continue;


                // 헤더 전송
                int retry = 3;
                while (retry--) {
                    lock_guard<mutex> lock(smtx);
                    int bytes = send_all(sd, (char*)&hd, sizeof(hd), 1000);
                    if (bytes >= 0) break;  // 성공
                    if (bytes == -2) {       // timeout
                        cerr << "클라이언트 " << i << "에게 ps 헤더 송신 타임아웃, 재시도 " << 3 - retry << "\n";
                        this_thread::sleep_for(chrono::milliseconds(1));
                        continue;
                    }
                    cerr << "[SERVER] send header to socket " << clientsoc << " 실패: ret=" << bytes
                        << " err=" << WSAGetLastError() << "\n";
                    break;
                }

                // 데이터 전송
                retry = 3;
                while (retry--) {
                    lock_guard<mutex> lock(smtx);
                    int bytes = send_all(sd, (char*)&ps, hd.length, 500);
                    if (bytes >= 0) break;  // 성공
                    if (bytes == -2) {
                        cerr << "클라이언트 " << i << "에게 ps 데이터 송신 타임아웃, 재시도 " << 3 - retry << "\n";
                        this_thread::sleep_for(chrono::milliseconds(1));
                        continue;
                    }
                    cerr << "[SERVER] send data to socket " << clientsoc << " 실패: ret=" << bytes
                        << " err=" << WSAGetLastError() << "\n";
                    break;
                }
            }
        }
    }


    // 연결 종료
    closesocket(clientsoc);

    id[id_num - 1] = false;

    vector<SOCKET> tempclients; // 클라이언트 백터 복사

    {
        lock_guard<mutex> lock(clientmtx);
        clients.erase(remove(clients.begin(), clients.end(), clientsoc), clients.end());
        tempclients = clients;
    }

    header hd;
    // 다른 클라이언트에게 종료 알림
    for (auto& sd : tempclients) {
        lock_guard<mutex> lock(smtx);
        hd.type = 2;
        hd.length = sizeof(id_num);
        if(send_all(sd, (char*)&hd, sizeof(hd),200) <= 0) {
            cout << "sendType2 실패\n" << flush;
        }
        if (send_all(sd, (char*)&id_num, hd.length,500) <= 0) {
            cout << "send id_num 실패\n" << flush;
        }
    }

    cout << "클라이언트 " << id_num << " 연결 종료\n";

}



int main() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup 실패\n";
        return 1;
    }
    else {
        cout << "WSAStartup ok\n";
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "socket() failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return 1;
    }
    else {
        cout << "socket() ok\n";

    }
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(54000);
    if (bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "bind() failed: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    else {
        cout << "bind() ok\n";
    }
    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "listen() failed: " << WSAGetLastError() << "\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    else {
        cout << "listening on port 54000\n";
    }


    while (true) {
        SOCKET clientSocket = accept(serverSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) {
            int err = WSAGetLastError();
            cerr << "accept() failed: " << err << "\n";
            this_thread::sleep_for(chrono::milliseconds(100));
            continue;
        }
        cout << "accept() returned socket: " << clientSocket << "\n";

        int id_num = -1;
        for (int i = 0; i < id.size(); i++) {
            if (!id[i]) {
                id[i] = true;
                id_num = i + 1;
                break;
            }
        }

        if (id_num != -1) {

            {
                lock_guard<mutex> lock(clientmtx);
                clients.push_back(clientSocket);
            }
            cout << "클라이언트 " << id_num << " 연결 성공\n";

            header hd;
            hd.type = 3;
            hd.length = sizeof(id_num);

            {
                lock_guard<mutex> lock(smtx);
                if (send_all(clientSocket, (char*)&hd, sizeof(hd), 200) <= 0) {
                    cout << "send 초기 id_num 실패\n" << flush;
                }
                if (send_all(clientSocket, (char*)&id_num, hd.length, 200) <= 0) {
                    cout << "send 초기 id_num 실패\n" << flush;
                }
            }

            SOCKET newSock = clientSocket;
            clienthandle.emplace_back(clientthread, newSock, id_num, ref(clients), ref(smtx), ref(clientmtx)); // ref(x) x를 reference 참조 (원본 전달). 그냥 쓰면 스레드에 복사본 전달됨
            clienthandle.back().detach();
        }
        else {
            cout << "접속 불가\n";
            closesocket(clientSocket);
        }

    }
    for (auto& th : clienthandle) {
        if (th.joinable()) th.join();
    }

    closesocket(serverSocket);
    WSACleanup();

    return 0;
}

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <windows.h>
#include <conio.h>
#include <thread>
#include <mutex>
#include <string>
#include <vector>
#include <chrono>
#include <random>
#include "tetris.h"
#pragma comment(lib, "Ws2_32.lib") // 링크 필요
using namespace std;

playerstate ps;
bef_playerstate bps[5];

int ill_x = ps.x;
int ill_y = ps.y;

// 7개 블록, 각각 4x4 크기
bool tp[7][4][4] = {
    {   // 정사각형 블록
        {0,0,0,0},
        {0,1,1,0},
        {0,1,1,0},
        {0,0,0,0}
    },
    {   // 역기역 블록
        {0,0,0,0},
        {1,1,1,0},
        {1,0,0,0},
        {0,0,0,0}
    },
    {   // 기역 블록
        {0,0,0,0},
        {1,1,1,0},
        {0,0,1,0},
        {0,0,0,0}
    },
    {   // S 블록
        {0,0,0,0},
        {1,1,0,0},
        {0,1,1,0},
        {0,0,0,0}
    },
    {   // Z 블록
        {0,0,0,0},
        {0,1,1,0},
        {1,1,0,0},
        {0,0,0,0}
    },
    {   // T 블록
        {0,0,0,0},
        {1,1,1,0},
        {0,1,0,0},
        {0,0,0,0}
    },
    {   // I 블록 (일자)
        {0,0,0,0},
        {1,1,1,1},
        {0,0,0,0},
        {0,0,0,0}
    }
};

bool rset = false; // 다음 블록 생성 변수

chrono::steady_clock::time_point start = chrono::steady_clock::now(); // 블록 하강 대기 시작 시간
chrono::steady_clock::time_point end_t = chrono::steady_clock::now();   // 블록 하강 대기 끝 시간
chrono::steady_clock::duration diff = end_t - start;
long long ms = 0; // 블록 하강 대기 시간 저장 변수

chrono::steady_clock::time_point bot_start = chrono::steady_clock::now(); // 바닥에 닿은 시작 시간
chrono::steady_clock::time_point bot_end = chrono::steady_clock::now();   // 바닥에 닿은 끝 시간
chrono::steady_clock::duration bot_diff = bot_end - bot_start;
bool blk_cnt = false; // 바닥에 닿았는지 판별 변수
long long bot_ms = 0; // 바닥에 닿은 시간 저장 변수

chrono::steady_clock::time_point rise_start = chrono::steady_clock::now(); // 바닥 상승 대기 시작 시간
chrono::steady_clock::time_point rise_end = chrono::steady_clock::now();   // 바닥 상승 대기 끝 시간
chrono::steady_clock::duration rise_diff = rise_end - rise_start;
long long rise_ms = 0; // 바닥 상승 대기 시간 저장 변수

int falling_t = 2000; // 블록 자동으로 떨어지는 기준 시간
int rising_t = 10000; // 바닥 상승하는 기준 시간


random_device rd;
mt19937 gen(rd());
uniform_int_distribution<int> dis(0, 6);
uniform_int_distribution<int> bot_block(0, 9);

string et_blk = "□";
string fd_blk = "■";
string hf_blk = "▨";


WSADATA wsaData;
SOCKET sock;
sockaddr_in serverAddr{};
fd_set readfds; // 읽기 감시용

bool chat_mode = false; // 채팅 모드 확인
string msg; // 채팅 입력 버퍼
vector<string> chatvec; // 채팅 로그 백터

mutex send_mtx;
mutex client_mtx;
mutex chat_mtx;
mutex sock_mtx;

queue<OutMsg> out_queue;
mutex out_mtx;
condition_variable out_cv;

atomic<bool> thread_play(true);
atomic<bool> player_out(false);
atomic<bool> recv_chat(false);
queue<playerstate> recv_queue;
atomic<int> discon(-1);


int main() {
    ios::sync_with_stdio(0);
    cin.tie(0);
    thread sender; // 송신 스레드 생성
    thread receiver; // 수신 스레드 생성

    if (sv_con()) { // 서버와 연결
        sender = thread(sendthread); // 송신 스레드 연결
        receiver = thread(recvthread); // 수신 스레드 연결
    }
rgame:
    init(); // 초기화
    gameloop(); // 게임 동작
    gotoxy(3, 35, ps.playerid); // 재시작 여부 판별
    cout << "replay? (y, n) : " << flush;
    char rpl;
    while (1) {
        gotoxy(21, 35, ps.playerid);
        cin >> rpl;
        if (rpl == 'y') {// y면 초기화 후 재시작
            system("cls");
            goto rgame;
        }
        else if (rpl == 'n') break; // n면 종료
    }
    thread_play.store(false); // 스레드 동작 플래그 false
    out_cv.notify_all(); // wait으로 잠들어있는 모든 스레드 깨우기
    if (sender.joinable()) sender.join(); // 송신 스레드 join
    if (receiver.joinable()) receiver.join(); // 수신 스레드 join
    closesocket(sock); // 소캣 닫기
    WSACleanup(); // 종료 시 정리


    return 0;
}

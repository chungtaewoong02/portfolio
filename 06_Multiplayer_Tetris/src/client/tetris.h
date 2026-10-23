#ifndef __TT_H__
#define __TT_H__
#include <winsock2.h> // winsock 2.0 표준 api 함수 선언하는 헤더(#pragma comment(lib, "Ws2_32.lib") 여기서 함수 내용 있는 라이브러리 연결)
#include <ws2tcpip.h> // winsock 2용 확장 기능 헤더 inet_pton와 같은 함수 확장 헤더
#include <thread> // 스레드 헤더
#include <mutex> // 스레드 사용 시 보호를 위해 코드 블록 단위로 lock을 걸 수 있게 해주는 헤더
#include <condition_variable> // 송신 스레드 wait / notify 헤더
#include <vector> // 백터 사용 헤더
#include <string> // 문자열 관련 함수 헤더
#include <chrono> // 시간 관련 헤더
#include <random> // 랜덤값 관련 헤더
#include <atomic> // 스레드 사용 시 보호를 위해 원자 단위로(하나의 값만) 보호해주는 헤더
#include <queue> // 큐 사용 헤더
using namespace std;

extern random_device rd; // gen 초기화 변수
extern mt19937 gen; // 랜덤 선택 변수
extern uniform_int_distribution<int> dis; // 블록 모양 선택 리스트
extern uniform_int_distribution<int> bot_block; // 바닥 상승 중 뺄 블록 선택 리스트

extern string et_blk; // "□"
extern string fd_blk; // "■"
extern string hf_blk; // "▨"

extern int ill_x, ill_y; // 생성된 블록의 환영 블록의 좌표
extern int falling_t; // 블록 하강 기준 시간
extern int rising_t; // 바닥 상승 기준 시간

extern bool tp[7][4][4]; // 생성할 블록 타입 배열

extern bool rset; // 블록 재생성 flag
extern bool blk_cnt; // 바닥 고정 flag

extern chrono::steady_clock::time_point start, end_t; // 블록 하강 시간 계산
extern chrono::steady_clock::duration diff;
extern long long ms;

extern chrono::steady_clock::time_point bot_start, bot_end; // 블록이 바닥에 닿고있는 시간 계산
extern chrono::steady_clock::duration bot_diff;
extern long long bot_ms;

extern chrono::steady_clock::time_point rise_start, rise_end; // 바닥 상승 시간 계산
extern chrono::steady_clock::duration rise_diff;
extern long long rise_ms;

#pragma pack(push, 1)// tcp 스트림 특성 때문에 데이터가 바이트 단위로 일치해야해서 패딩 제거를 위해 1바이트 단위로 정렬
struct playerstate { // 플레이어 상태 송수신 구조체
	int playerid = 0; // 플레이어 아이디
	bool gm_play = true; // 게임 실행 여부
	int score = 0; // 점수
	bool gm_map[20][10] = {}; // 게임 맵
	int x = 0, y = 0; // 블록 좌표
	bool cpd_tp[4][4] = {}; // 7개 블록 중 선택한 블록 카피
};
#pragma pack(pop)

struct bef_playerstate { // 다른 플레이어들의 이전 정보 저장 구조체
	int bef_x = 0, bef_y = 0; // 이전 블록 xy 좌표
	bool bef_cpd_tp[4][4] = {}; // 이전 블록 모양
	bool bef_gm_map[20][10] = {}; // 이전 맵 상태
};


extern bef_playerstate bps[5]; // 다른 플레이어 playerstate 저장 배열

extern playerstate ps;

#pragma pack(push, 1) // tcp 스트림 특성 때문에 데이터를 1바이트 단위로 직렬화
struct header { // 송수신을 위해 먼저 처리할 헤더
	uint8_t type = 0; // 0 : playerstate, 1 : 채팅, 2 : discon, 3 : 자신 id 수신
	uint32_t length = 0; // 데이터 길이
};
#pragma pack(pop)

// 송신 큐 구조체 및 관련 변수
struct OutMsg {
	header hd; // 헤더 정보
	vector<char> data; // 실제 메세지 데이터
};

extern queue<OutMsg> out_queue; // 보낼 채팅 메세지 저장해두는 큐
extern mutex out_mtx; // out_queue 접근을 위한 mtx
extern condition_variable out_cv; // 스레드가 “기다리기(wait)” 또는 “깨우기(notify)” 하는 신호 객체

extern WSADATA wsaData; // winsock 초기화 정보 구조체
extern SOCKET sock; // 통신용 소켓
extern sockaddr_in serverAddr; // 서버 주소 구조체(주소 체계, ip 주소, 포트 정보 등)
extern fd_set readfds; // 읽기 감시용

extern bool chat_mode; // 채팅 모드 확인
extern vector<string> chatvec; // 채팅 로그 저장
extern string msg; // 채팅 입력 버퍼

// 타입 0 : playerstate / 1 : 채팅 / 2 : 연결 끊김 / 3 : 자신 id 수신


extern mutex send_mtx; // 보낼 때
extern mutex client_mtx; // ps
extern mutex chat_mtx; // 채팅
extern mutex sock_mtx; // sock
extern atomic<bool> thread_play; // 스레드 동작 플래그
extern atomic<bool> player_out; // 플레이어 연결 끊김 플래그
extern atomic<bool> recv_chat; // 채팅 수신 플래그
extern queue<playerstate> recv_queue; // 수신받은 다른 플레이어들 상태를 임시로 저장해놓을 큐
extern atomic<int> discon; // 연결 끊긴 플레이어 id 표시 변수

void gotoxy(int x, int y, int playerid); // 콘솔창 커서 이동 함수
void init(); // 초기화 함수
void game_over(); // 게임 오버 함수
void draw_blk(bool erase = false); // 블록 그리기 함수
void draw_ill_blk(bool erase = false); // 환영 블록 그리기 함수
bool can_move(int nx, int ny, bool shape[4][4]); // 이동 가능 판별 함수
void add_blk(int i); // 새 블록 생성 함수
void move_blk(int comd); // 블록 이동 함수
void move_ill_blk(); // 환영 블록 이동 함수
void turn_blk(int comd); // 블록 회전 함수
bool tch_bot(); // 블록 바닥에 닿고 있는지 판별 함수
void blk_stop(); // 블록 맵에 고정하는 함수
int filled_line(); // 맵에서 가득 찬 줄 반환하는 함수
void erase_line(int line); // 줄 제거 함수
void rise_blk(int i); // 바닥 상승 함수
void player_st(playerstate& temp); // 다른 플레이어 게임 상태 그리기 함수
bool sv_con(); // 서버와의 연결하는 함수
int recv_all(SOCKET sock, char* buf, int len, int timeout_ms = 1); // 반복 recv 함수(TCP 스트림 특성 때문에 사용)
int send_all(SOCKET s, const char* buf, int len, int timeout_ms = 1); // 반복 send 함수(TCP 스트림 특성 때문에 사용)
void enqueue_chat_send(const string& msg); // 송신할 채팅 메세지 큐에 저장하는 함수
void recvthread(); // 수신 스레드 함수
void sendthread(); // 송신 스레드 함수
void gameloop(); // 실제 게임 동작 함수

#endif // !__TT_H__

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
using namespace std;

void gotoxy(int x, int y, int playerid) { // 커서 위치 지정
    int offsetx = 0, offsety = 0;
    switch (playerid) {
    case 1: offsetx = 0;  offsety = 0;  break; // 1사분면
    case 2: offsetx = 40; offsety = 0;  break; // 2사분면
    case 3: offsetx = 0;  offsety = 40; break; // 3사분면
    case 4: offsetx = 40; offsety = 40; break; // 4사분면
    }
    COORD Pos = { static_cast<SHORT>(x + offsetx - 1), static_cast<SHORT>(y + offsety - 1) };
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Pos);
}



void init() { // 초기화
    system("mode con cols=160 lines=160");
    system("cls");
    while (!chatvec.empty()) chatvec.pop_back(); // 채팅 로그 초기화

    ps.gm_play = true; // 게임 플레이 플래그
    ps.score = 0; // 점수
    rset = false; // 블록 생성 플래그
    blk_cnt = false; // 바닥 고정 플래그
    falling_t = 2000; // 블록 자동으로 떨어지는 기준 시간
    rising_t = 10000; // 바닥 상승하는 기준 시간
    chat_mode = false; // 채팅 모드 진입 플래그

    // 2. 블록 위치 초기화
    ps.x = 0; // 블록 x,y 좌표
    ps.y = 0;
    ill_x = ill_y = 0; // 수직 아래와 닿는 블록의 환영의 x,y 좌표
    for (int i = 0; i < 4; i++) // 현재 블록 상태 초기화
        for (int j = 0; j < 4; j++)
            ps.cpd_tp[i][j] = false;

    // 3. 게임 맵 초기화
    for (int i = 0; i < 20; i++)
        for (int j = 0; j < 10; j++)
            ps.gm_map[i][j] = false;

    // 4. 시간 초기화
    start = chrono::steady_clock::now(); // 블록 하강 시간
    end_t = chrono::steady_clock::now();
    bot_start = chrono::steady_clock::now(); // 블록이 바닥과 닿고 있는 시간
    bot_end = chrono::steady_clock::now();
    rise_start = chrono::steady_clock::now(); // 바닥이 올라오는 시간
    rise_end = chrono::steady_clock::now();

    // 경계선 출력
    for (int i = 0; i <= 20; ++i) {
        gotoxy(1, i + 5, ps.playerid);
        cout << et_blk << flush;
        gotoxy(12, i + 5, ps.playerid);
        cout << et_blk << flush;
    }
    for (int i = 1; i <= 12; ++i) {
        gotoxy(i, 25, ps.playerid);
        cout << et_blk << flush;
    }

    gotoxy(1, 90, 1);
    cout << ">> " << flush; // 채팅 모드시 타이핑 문자 위치 표시
}

void game_over() {
    gotoxy(20, 5, ps.playerid);
    cout << "my_score : " << ps.score << flush;
    gotoxy(3, 30, ps.playerid);
    cout << "gameover" << flush;
    ps.gm_play = false; // 게임 플레이 플래그 false
}

bool can_move(int nx, int ny, bool shape[4][4]) { // 이동 가능 판별
    for (int j = 0; j < 4; ++j) {
        for (int k = 0; k < 4; ++k) {
            if (shape[j][k]) { // 현재 다루고 있는 블록
                int gx = nx + k;  // 0 ~ 9
                int gy = ny + j;  // 0 ~ 19

                // 경계 체크 (0-based)
                if (gx < 0 || gx >= 10 || gy >= 20) return false;

                // 바닥보다 위(gy < 0)는 허용
                if (gy >= 0 && ps.gm_map[gy][gx]) return false;
            }
        }
    }
    return true;
}


void draw_blk(bool erase) { // 블록 그리기
    for (int j = 0; j < 4; ++j) {
        for (int k = 0; k < 4; ++k) {
            if (ps.cpd_tp[j][k]) {
                int gx = ps.x + k;
                int gy = ps.y + j;
                if (gx >= 0 && gx < 10 && gy < 20) { // 맵 범위 내에서만 but 맵 위는 허용
                    gotoxy(gx + 2, gy + 5, ps.playerid);
                    if (erase) {
                        cout << " " << flush; // 블록 지우기
                    }
                    else {
                         cout << fd_blk << flush; //블록 그리기
                    }
                }
            }
        }
    }
}

void move_ill_blk() { // 환영 블록 이동
    int nx = ps.x, ny = ps.y;
    while (can_move(nx, ny, ps.cpd_tp)) {
        ++ny;
    }
    --ny;

    draw_ill_blk(true);  // 기존 위치 블록 지우기
    ill_x = nx; ill_y = ny;
    draw_ill_blk(); // 새 위치 블록 그리기
}

void draw_ill_blk(bool erase) { // 환영 블록 그리기

    for (int j = 0; j < 4; ++j) {
        for (int k = 0; k < 4; ++k) {
            if (ps.cpd_tp[j][k]) {
                int gx = ill_x + k;
                int gy = ill_y + j;
                if (gx >= 0 && gx < 10 && gy < 20) { // 맵 범위 내에서만 but 맵 위는 허용
                    gotoxy(gx + 2, gy + 5, ps.playerid);
                    if (erase) {
                        cout << " " << flush; // 블록 지우기
                    }
                    else {
                        cout << hf_blk << flush; // 블록 그리기
                    }
                }
            }
        }
    }
}

void add_blk(int i) { // 블록 생성
    for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k) ps.cpd_tp[j][k] = tp[i][j][k];
    ps.x = 4; ps.y = -3; // 시작 위치
    if (!can_move(ps.x, ps.y, ps.cpd_tp)) { // 이미 겹치는 상황이라 게임오버
        game_over();
        return;
    }

    draw_blk();
    int nx = ps.x, ny = ps.y;
    while (can_move(nx, ny, ps.cpd_tp)) {
        ++ny;
    }
    --ny;
    ill_x = nx; ill_y = ny;
    draw_ill_blk(); // 새 위치 블록 그리기
}


void move_blk(int comd) { // 블록 이동
    int nx = ps.x, ny = ps.y;
    if (comd == 80) ++ny;       // down
    else if (comd == 75) --nx;  // left
    else if (comd == 77) ++nx;  // right

    if (can_move(nx, ny, ps.cpd_tp)) {
        draw_blk(true);  // 기존 위치 블록 지우기
        ps.x = nx; ps.y = ny;
        if (comd != 80)
        {
            move_ill_blk();
        }
        draw_blk(); // 새 위치 블록 그리기
    }
}

bool tch_bot() {
    bool chk[4] = {}; // 바닥과 닿는 면 확인
    bool flag = false;
    int nx = ps.x;
    int ny = ps.y + 1;

    for (int j = 3; j >= 0; --j) {
        for (int k = 0; k < 4; ++k) { // 아래줄부터 확인
            if (chk[k]) continue; // 바닥과 닿는지 확인했으면 스킵
            if (ps.cpd_tp[j][k]) {//
                if ((ny + j) >= 20 || ((ny + j) >= 0 && ps.gm_map[ny + j][nx + k])) { //바로 아래 칸에 블록이 있다면 break
                    flag = true;
                    break;
                }
                chk[k] = true; // 바닥과 닿지 않음
            }
        }
        if (flag) break;
    }

    return flag;
}


void blk_stop() { // 블록 정지
    for (int j = 0; j < 4; ++j) {
        for (int k = 0; k < 4; ++k) {
            if (ps.cpd_tp[j][k]) { //4*4 중 블록이 있다면
                int gy = ps.y + j;
                int gx = ps.x + k;
                if (gy >= 0 && gy < 20 && gx >= 0 && gx < 10) {
                    ps.gm_map[gy][gx] = true; // 채워넣기
                }
            }
        }
    }
    if (ps.y < 0) game_over();
}

void turn_blk(int comd) { // 블록 회전
    bool tmp[4][4]; // 기존 블록 복사
    bool rot[4][4]; // 회전한 블록 저장
    for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k) {
        tmp[j][k] = ps.cpd_tp[j][k];
        rot[j][k] = ps.cpd_tp[j][k];
    }

    if (comd == 'a') { // 좌회전 (반시계)
        for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k) rot[j][k] = tmp[k][3 - j];
    }
    else if (comd == 'd') { // 우회전 (시계)
        for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k) rot[j][k] = tmp[3 - k][j];
    }
    else if (comd == 32) { // 스페이스바(바닥으로 강하)
        while (!tch_bot()) {
            move_blk(80);
        }
        blk_stop();
        rset = true;
        blk_cnt = false;
    }


    if (can_move(ps.x, ps.y, rot)) { // 이동 가능 판별
        draw_blk(true); // 기존 위치 삭제
        draw_ill_blk(true);
        for (int j = 0; j < 4; ++j) for (int k = 0; k < 4; ++k) ps.cpd_tp[j][k] = rot[j][k];
        int nx = ps.x, ny = ps.y;
        while (can_move(nx, ny, ps.cpd_tp)) {
            ++ny;
        }
        --ny;
        ill_x = nx; ill_y = ny;
        draw_ill_blk(); // 새 위치 블록 그리기
        draw_blk(false); // 회전한 블록 생성
    }
}


int filled_line() { // 줄 채워졌는지 확인
    bool flag = true;
    for (int j = 0; j < 20; ++j) {
        for (int k = 0; k < 10; ++k) if (!ps.gm_map[j][k]) flag = false; // 한줄 확인 중 빈 칸 있으면 flase
        if (flag) return j; // 한 줄 가득 차 있으면 그 줄 반환
        flag = true;
    }
    return -1;
}

void erase_line(int line) { // 줄 지우기
    for (int k = 0; k < 10; ++k) {
        ps.gm_map[line][k] = false; // 가득 찬 줄 삭제
        gotoxy(k + 2, line + 5, ps.playerid);
        cout << " " << flush;
    }
    for (int j = line; j > 0; --j) { // 삭제한 줄 위의 블록들 아래로 내리기
        for (int k = 0; k < 10; ++k) {
            ps.gm_map[j][k] = ps.gm_map[j - 1][k];
            gotoxy(k + 2, j + 5, ps.playerid);
            cout << " " << flush;
            if (ps.gm_map[j][k]) {
                gotoxy(k + 2, j + 5, ps.playerid);
                cout << fd_blk << flush;
            }
        }
    }
    ps.score += 100;
    gotoxy(20, 5, ps.playerid);
    cout << "my_score : " << ps.score << flush;
}

void rise_blk(int i) {
    draw_blk(true);  // 기존 위치 블록 지우기
    draw_ill_blk(true);
    for (int k = 0; k < 10; ++k) { // 위로 올려야하는데 맨 윗줄에 블록이 있다면 gameover
        if (ps.gm_map[0][k]) {
            game_over();
            return;
        }
    }
    for (int j = 0; j < 20; ++j) { // 한칸씩 위로 올리기
        for (int k = 0; k < 10; ++k) {
            ps.gm_map[j][k] = ps.gm_map[j + 1][k];
            gotoxy(k + 2, j + 5, ps.playerid);
            cout << " " << flush;
            if (ps.gm_map[j][k]) {
                gotoxy(k + 2, j + 5, ps.playerid);
                cout << fd_blk << flush;
            }
        }
    }
    for (int k = 0; k < 10; ++k) { // 밑에 한 줄 생성(랜덤으로 한 칸 제외)
        if (k != i) ps.gm_map[19][k] = true;
        else ps.gm_map[19][k] = false;
        gotoxy(k + 2, 24, ps.playerid);
        cout << " " << flush;
        if (ps.gm_map[19][k]) {
            gotoxy(k + 2, 24, ps.playerid);
            cout << fd_blk << flush;
        }
    }
    for (int j = 0; j < 4; ++j) { // 기존 위치의 블록과 줄이 한 칸 올라왔을 때 블록이 충돌한다면 --y;
        for (int k = 0; k < 4; ++k) {
            if (ps.cpd_tp[j][k]) {
                int gx = ps.x + k;  // 0 ~ 9
                int gy = ps.y + j;  // 0 ~ 19

                if (ps.cpd_tp[j][k] && ps.gm_map[gy][gx]) --ps.y;

            }
        }
    }
    --ill_y;
    draw_ill_blk();
    draw_blk(); // 새 위치 블록 그리기
}

void player_st(playerstate& temp) { // 다른 플레이어 상태 그리기
    if (temp.playerid <= 0 || temp.playerid > 4) return;
    for (int i = 0; i <= 20; ++i) {
        gotoxy(1, i + 5, temp.playerid);
        cout << et_blk << flush;
        gotoxy(12, i + 5, temp.playerid);
        cout << et_blk << flush;
    }
    for (int i = 1; i <= 12; ++i) {
        gotoxy(i, 25, temp.playerid);
        cout << et_blk << flush;
    }
    // 맵 출력
    for (int j = 0; j < 20; j++) {
        for (int k = 0; k < 10; k++) {
            if (temp.gm_map[j][k]) {
                gotoxy(k + 2, j + 5, temp.playerid);
                cout << fd_blk << flush;
            } // 이전 상태와 새로 들어온 상태가 다를 때
            if (bps[temp.playerid].bef_gm_map[j][k] != temp.gm_map[j][k]) {
                if (!temp.gm_map[j][k]) {
                    gotoxy(k + 2, j + 5, temp.playerid);
                    cout << " " << flush;
                } // 새로 들어온 맵 상태 이전 맵 상태에 저장
                bps[temp.playerid].bef_gm_map[j][k] = temp.gm_map[j][k];
            }
        }
    }
    // 이전 블록 제거
    for (int j = 0; j < 4; j++) {
        for (int k = 0; k < 4; k++) {
            if (bps[temp.playerid].bef_cpd_tp[j][k]) {
                int gx = bps[temp.playerid].bef_x + k;
                int gy = bps[temp.playerid].bef_y + j;
                if (gx >= 0 && gx < 10 && gy >= 0 && gy < 20) { // 맵 범위 내에서만 but 맵 위는 허용
                    gotoxy(gx + 2, gy + 5, temp.playerid); // 콘솔 좌표 변환
                    cout << " " << flush;
                }
            }
        }
    }

    bps[temp.playerid].bef_x = temp.x; // 현재 좌표를 이전 좌표 저장 위치에 저장
    bps[temp.playerid].bef_y = temp.y;

    // 현재 블록 출력
    for (int j = 0; j < 4; j++) {
        for (int k = 0; k < 4; k++) {
            bps[temp.playerid].bef_cpd_tp[j][k] = temp.cpd_tp[j][k];
            if (temp.cpd_tp[j][k]) {
                int gx = temp.x + k;
                int gy = temp.y + j;
                if (gx >= 0 && gx < 10 && gy >= 0 && gy < 20) { // 맵 범위 내에서만 but 맵 위는 허용
                    gotoxy(gx + 2, gy + 5, temp.playerid); // 콘솔 좌표 변환
                    cout << fd_blk << flush;

                }
            }
        }
    }

    gotoxy(20, 5, temp.playerid); // 점수 표시
    cout << "player" << temp.playerid << " score : " << temp.score << flush;

    if (!temp.gm_play) {
        gotoxy(3, 30, temp.playerid);
        cout << "gameover" << flush;
    }
    else {
        gotoxy(3, 30, temp.playerid);
        cout << "          " << flush;
    }
}

bool sv_con() { // 서버 접속
    // 버전 2.2 사용, 사실상 최종 완성된 안정 버전이 2.2이기 때문
    // 소켓 라이브러리 사용 전 초기화 함수(사용할 winsock 버전, 초기화된 정보를 받을 구조체 포인터)
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup 실패\n";
        return false;
    }
    // IPv4 주소 체계를 사용하는 TCP 스트림 방식으로 통신할 소켓 생성
    sock = socket(AF_INET, SOCK_STREAM, 0);
    // 어떤 방식으로 쓰인 주소인가(IPv4)
    serverAddr.sin_family = AF_INET;
    // 들어갈 서버에서 들어갈 포트
    serverAddr.sin_port = htons(54000);
    // 문자열을 주소 체계 (IPv4) 서버 주소로 변환해서 삽입 (서버를 실행하는 PC의 IP로 변경, 같은 PC면 127.0.0.1)
    inet_pton(AF_INET, "192.168.0.12", &serverAddr.sin_addr);

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) { // 서버와 연결
        sock = INVALID_SOCKET; // 나중에 select나 send/recv에서 체크용
        return false;
    }
    else {
        // 연결 성공 시 ID 수신
        int id = 0;
        header hd;
        recv_all(sock, (char*)&hd, sizeof(hd), 500);
        if (hd.type == 3) {// id 부여받았다면
            recv_all(sock, (char*)&id, hd.length, 500); // id 수신
        }
        ps.playerid = id; // 자신 id 갱신
    }
    u_long mode = 0;
    ioctlsocket(sock, FIONBIO, &mode); // 블로킹 모드
    return true;
}


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
            // timeout: 만약 아직 0바이트 받았다면 타임아웃으로 알려주고,
            // 일부 바이트를 이미 받았다면 기다릴 필요가 있으므로 루프 계속.
            if (total == 0) return -2;
            else continue;
             // 일부 읽은 상태이면 계속 루프(다시 select)
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

void enqueue_chat_send(const string& msg) {
    OutMsg om;// 메세지 구조체
    om.hd.type = 1; // 송신 타입 1 (채팅)
    om.hd.length = static_cast<uint32_t>(msg.size()); // 메세지 size 만큼의 길이
    om.data.assign(msg.begin(), msg.end()); // msg 채팅 내용 om의 data로 복사

    {
        lock_guard<mutex> lk(out_mtx);
        // 큐 크기 제한 : 네트워크가 느려질 때 무한정 쌓이지 않게
        if (out_queue.size() > 100) out_queue.pop();
        // 단순 복사 비용이 크기 때문에 om을 복사해서 집어넣는 것이 아닌 통채로 큐 안에 넣기. om은 비어있는 상태가 됨
        out_queue.push(move(om));
    }
    out_cv.notify_one(); // 스레드를 깨우라는 신호. 여기서는 sendthread를 wait으로 멈췄다가 다시 실행
}

void sendthread() {
    using namespace std::chrono; // 크로노 생략
    auto next_tick = steady_clock::now(); // 송신 큐가 비어있을 때 기다리는 최대 시간

    // 1) 이벤트(채팅 등) 우선 처리: 큐가 비어있으면 최대 next_tick까지 기다림
    while (thread_play.load()) { // thread_play가 true라면
        {
            unique_lock<mutex> qlk(out_mtx); // 조건부 lock qlk 생성시 바로 lock
            if (out_queue.empty()) { // 송신 큐가 비어있다면
                // next_tick까지 기다렸다가 깨어나거나, notify_one()으로 즉시 깨어남
                out_cv.wait_until(qlk, next_tick); // qlk는 기다리는 동안 mutex 해제 후 깨어나면 다시 lock
            }
            while (!out_queue.empty()) { // 송신 큐가 비어있지 않을 동안
                OutMsg om = move(out_queue.front()); // 메세지 구조체에 out_queue 첫번째 값 이동
                out_queue.pop(); // 빈 front 칸 pop
                qlk.unlock(); // lock해제

                // 보내기 (헤더 + 데이터)
                if (sock == INVALID_SOCKET) return; // sock가 유효하지 않다면 return
                int bytes = send_all(sock, (char*)&om.hd, sizeof(om.hd), 1000); // 헤더 먼저 송신 timeout 1000ms
                if (bytes <= 0) { // 송신 실패면
                    sock = INVALID_SOCKET; // sock 유효하지 않음
                    thread_play.store(false); // thread_play false로 갱신
                    return;
                }
                if (!om.data.empty()) { // om의 데이터가 비어있지 않으면
                    bytes = send_all(sock, om.data.data(), (int)om.data.size(), 1000); // 데이터 송신 timeout 1000ms
                    if (bytes <= 0) { // 송신 실패면
                        sock = INVALID_SOCKET; // sock 유효하지 않음
                        thread_play.store(false); // thread_play false로 갱신
                        return;
                    }
                }

                qlk.lock(); // lock 걸기
            }
        }

        // 2) 주기 전송: playerstate (150ms)
        auto now = steady_clock::now();
        if (now >= next_tick) {
            next_tick = now + milliseconds(150); // next_tick 갱신

            // ps 복사본 생성 (짧게 lock)
            playerstate send_ps;
            {
                lock_guard<mutex> lk(send_mtx);
                send_ps = ps; // 이 안에서 사용할 ps 복사본 send_ps
            }

            header hd; // 헤더 구조체 type과 length
            hd.type = 0; // 송신 타입  0 (ps)
            hd.length = sizeof(playerstate); // 송신 길이 ps만큼

            if (sock == INVALID_SOCKET) { // sock 유효하지 않으면
                thread_play.store(false); // thread_play false로 갱신
                return;
            }

            int bytes = send_all(sock, (char*)&hd, sizeof(hd), 1000); // 헤더 먼저 송신 timeout 1000ms
            if (bytes <= 0) { // 송신 실패면

                sock = INVALID_SOCKET; // sock 유효하지 않음
                thread_play.store(false); // thread_play false로 갱신
                return;
            }

            bytes = send_all(sock, (char*)&send_ps, sizeof(send_ps), 1000); // 데이터 송신 timeout 1000ms
            if (bytes <= 0) { // 송신 실패면
                sock = INVALID_SOCKET; // sock 유효하지 않음
                thread_play.store(false); // thread_play false로 갱신
                return;
            }
        }

        this_thread::sleep_for(milliseconds(1)); // 짧은 sleep으로 busy wait 완화 (대기중 wakeup이 더 빠름)
    }
}

void recvthread() { // 수신 스레드
    while (thread_play.load()) { // thread_play 현 상태 load
        if (sock == INVALID_SOCKET) { // sock 유효하지 않으면
            this_thread::sleep_for(chrono::milliseconds(5));
            continue;
        }

        header hd; // 헤더 구조체
        int bytes = recv_all(sock, (char*)&hd, sizeof(hd), 1000); // 헤더 수신 timeout 1000ms
        if (bytes <= 0) { // 수신 실패면
            if (bytes == -2) { // timeout 시간 내에 데이터 도착 안함 -> 받을 데이터 없음
                this_thread::sleep_for(chrono::milliseconds(1));
                OutputDebugStringA("recv 실패 timeout\n");
                continue;
            }// 에러 또는 접속 종료
            else {
                sock = INVALID_SOCKET; // sock 유효하지 않음
                thread_play.store(false); // thread_play false 갱신
                return;
            }
        }
        else {
            char buf[128];

            sprintf_s(buf, "헤더 수신 type, length : %d, %d\n", (int)hd.type, (int)hd.length);
            OutputDebugStringA(buf);
        }

        if (hd.type == 1) { // 채팅
            string pl_msg(hd.length, 0);//hd.length 길이의 string '/0'으로 초기화 생성
            bytes = recv_all(sock, &pl_msg[0], hd.length, 1000); // 메세지 데이터 수신 timeout 1000ms
            if (bytes <= 0) { // 수신 실패면
                sock = INVALID_SOCKET; // sock 유효하지 않음
                thread_play.store(false); // thread_play false 갱신
                return;
            }
            {
                lock_guard<mutex> lock(chat_mtx); // chatvec에 접근하기 위한 lock
                chatvec.push_back(pl_msg); // 채팅 내역을 저장하는 chatvec에 수신한 메세지 갱신
                if (chatvec.size() > 5) chatvec.erase(chatvec.begin()); // 최대 5 문자열까지 저장하고 그 이상은 제거
                recv_chat.store(true); // 채팅 수신 플래그 true 갱신
            }
        }
        else if (hd.type == 0) { // playerstate
            playerstate temp; // ps 구조체
            bytes = recv_all(sock, (char*)&temp, hd.length, 1000); // ps 데이터 수신 timeout 1000ms
            if (bytes <= 0) { // 수신 실패면
                OutputDebugStringA("recv ps 실패\n");
                sock = INVALID_SOCKET; // sock 유효하지 않음
                thread_play.store(false); // thread_play false 갱신
                return;
            }
            else {
                char buf[128];

                sprintf_s(buf, "헤더 수신 playerstate : %d\n", bytes);
                OutputDebugStringA(buf);
            }
            {
                lock_guard<mutex> lock(client_mtx); // 다른 플레이어들의 상태를 저장하는 recv_queue에 접근하기 위한 lock
                recv_queue.push(temp);// 수신받은 다른 플레이어 ps recv_queue에 넣기
            }
        }
        else if (hd.type == 2) { // disconnect
            int pid = -1; // 연결이 끊긴 다른 플레이어 id를 저장할 변수
            bytes = recv_all(sock, (char*)&pid, hd.length, 1000); // 플레이어 id 수신 timeout 1000ms
            if (bytes <= 0) { // 수신 실패면
                sock = INVALID_SOCKET; // sock 유효하지 않음
                thread_play.store(false); // thread_play false 갱신
                return;
            }
            discon.store(pid); // 연결 끊긴 플레이어 위치를 나타내기 위한 discon에 id 갱신
            player_out.store(true); // 플레이어 연결 끊김 플래그 true로 갱신
        }
    }
}

void gameloop() { // 게임 루프
    add_blk(dis(gen)); // 초기 블록 생성
    gotoxy(20, 5, ps.playerid);// 점수 표시
    cout << "my_score : " << ps.score<< flush; // 내 초기 점수 출력
    while (ps.gm_play) { // 게임 오버가 아닌 상태라면
        if (rset) { // 블록이 바닥에 고정되었다면 다음 블록 생성
            add_blk(dis(gen));
            start = chrono::steady_clock::now();
            rset = false;
        }

        int ln = 0; // 채워진 줄의 위치를 나타낼 변수
        while (ln >= 0) {
            ln = filled_line(); // 채워진 줄 확인
            if (ln >= 0) erase_line(ln); // 채워진 줄 확인 시 제거
        }

        end_t = chrono::steady_clock::now(); // 생성된 블록 하강 시간
        diff = end_t - start;
        ms = chrono::duration_cast<chrono::milliseconds>(diff).count();
        if (ms >= falling_t) { // 시간이 falling_t 이상이라면
            move_blk(80); //down
            if (blk_cnt) { // 바닥에 닿은 상태로 down키 한번 더 누르면 고정
                blk_stop(); // 블록 멈추기
                rset = true; // 블록 재생성 플래그 true
                blk_cnt = false; // 바닥 고정 플래그 false
            }
            start = chrono::steady_clock::now(); // 새 블록 생성되니 블록 떨어지는 시간의 start 갱신
        }

        rise_end = chrono::steady_clock::now(); // 바닥 상승시간
        rise_diff = rise_end - rise_start;
        rise_ms = chrono::duration_cast<chrono::milliseconds>(rise_diff).count();
        if (rise_ms >= rising_t) { // 시간이 rising_t 이상이라면
            rise_blk(bot_block(gen)); // 한 줄에서 랜덤한 한 블록 제외하고 줄 생성해서 바닥에서 올리기
            rise_start = chrono::steady_clock::now(); // rising time 갱신
            if (rising_t > 1000) rising_t -= 1000; // 1000ms 제한 rising time 감소
            if (falling_t > 500) falling_t -= 100; // 500ms 제한 falling time 감소
        }


        if (_kbhit()) { // 키가 눌렸는지 확인
            int comd = _getch(); // 한 문자 입력

            if (chat_mode) { // 채팅 모드라면

                if (comd != 0 && comd != 224) {//확장키 번호가 아니라면
                    char ch = comd;
                    if (ch == '\r') {// 엔터 입력 시 게임 모드
                        gotoxy(1, 90, 1);
                        cout << ">>                                                     " << flush; // 내가 타이핑 중이던 문자 터미널창에서 제거
                        chat_mode = false; // 채팅 모드 종료
                    }
                    else if (ch == '\b') { // 백스페이스 지우기
                        if (!msg.empty()) { // 현재 내가 작성하고 있는 문자열이 비어있지 않다면
                            msg.pop_back(); // msg 맨 마지막 글자 제거
                            gotoxy(1, 90, 1); // 글씨 지웠으니 다시 입력했던 문자들 갱신
                            cout << ">>                                                     " << flush;
                            gotoxy(1, 90, 1);
                            cout << ">> " << msg << flush;
                        }
                    }
                    else {
                        msg.push_back(ch); // 일반 아스키 코드값이라면 입력 문자열에 추가

                        if (!msg.empty()) { // 현재까지 입력한 문자들 보이기
                            gotoxy(1, 90, 1);
                            cout << ">> " << msg << flush;
                        }
                    }

                    if (!chat_mode) { // 문자열 입력 끝나고 엔터 누르면 문자열 전송
                        if (!msg.empty()) { // msg가 비어있지 않다면
                            enqueue_chat_send(msg); // 메세지 송신
                            {
                                lock_guard<mutex> lock(chat_mtx); // chatvec에 접근하기 위한 lock
                                chatvec.push_back(msg); // chatlog에 문자열 추가
                                if (chatvec.size() > 5) chatvec.erase(chatvec.begin()); // 6줄 이상 안보이게
                            }
                            msg.clear(); // 메세지 갱신 끝났으니 clear
                            recv_chat.store(true); // 채팅 수신 플래그 true로 갱신

                        }
                    }
                }
            }
            else { // 게임 모드라면
                if (comd == 0 || comd == 224) { // 확장키라면 이동
                    comd = _getch(); // 확장키라 다시 한번 입력
                    move_blk(comd); // 확장키에 해당하는 블록 동작

                    if (comd == 80 && blk_cnt) { // 바닥에 닿은 상태로 down키 한번 더 누르면 고정
                        blk_stop(); // 블록 정지
                        rset = true; // 새 블록 생성 플래그 true
                        blk_cnt = false; // 바닥 고정 플래그 false
                    }
                }
                else {
                    if (comd == '\r') chat_mode = true; // 엔터 누르면 채팅 모드 진입
                    else turn_blk(comd); // 아스키라면 회전
                }

            }
        }



        if (tch_bot()) { // 블록이 바닥에 닿았다면
            if (!blk_cnt) bot_start = chrono::steady_clock::now(); // 처음 닿았을 때 시작
            else bot_end = chrono::steady_clock::now(); // 이후 닿은게 유지되었을 때 끝
            blk_cnt = true; // 바닥에 닿았는지
            bot_diff = bot_end - bot_start; // 바닥에 닿고 있는 시간
            bot_ms = chrono::duration_cast<chrono::milliseconds>(bot_diff).count();
            if (bot_ms >= 2000) { // 시간이 2000ms 이상이라면
                blk_stop(); // 블록 정지
                rset = true; // 새 블록 생성 플래그 true
                blk_cnt = false; // 바닥 고정 플래그 false
            }
        }
        else blk_cnt = false; // 바닥에 닿았던거 초기화

        if (recv_chat.load()) { // 채팅 수신 플래그가 true라면
            int i = 0; // 채팅 로그 쓰기

            for (auto& c : chatvec) {
                lock_guard<mutex> lock(chat_mtx); // chatvec에 접근하기 위해 lock
                gotoxy(1, 80 + i, 1);
                i += 2;
                cout << c << "          " << flush;
            }

            recv_chat.store(false); // 채팅 수신 플래그 false 갱신
        }
        if (player_out.load()) { // 플레이어 연결 종료 플래그가 true라면
            for (int i = 1; i <= 35; i++) { // 해당하는 플래이어 맵 상태 지우기
                gotoxy(1, i, discon.load());
                cout << "                                     " << flush;
            }
            gotoxy(1, 25, discon.load());
            cout << "p" << discon.load() << " discon" << flush;
            discon.store(-1); // 연결 끊긴 플레이어 아이디 초기화
            player_out.store(false); // 플레이어 연결 종료 플래그 false 갱신
        }
        playerstate tempp; // ps 구조체
        {
            lock_guard<mutex> lock(client_mtx); // 다른 플레이어 ps가 저장된 recv_queue에 접근하기 위한 lock
            if (!recv_queue.empty()) {// recv_queue가 비어있지 않다면
                tempp = recv_queue.front(); // 맨 앞 ps 정보 복사
                recv_queue.pop();
            }
            else continue; // 비어있다면 continue
        }
        player_st(tempp); // 플레이어 상태 출력
    }
}

# 정태웅 | ASIC Design Portfolio

> RTL 합성부터 P&R, Sign-off check까지 직접 수행하며 **문제의 원인을 리포트로 확인하고 수치로 검증하는** 신입 ASIC 엔지니어입니다.

- 청주대학교 전자공학과 (2027.02 졸업 예정, 4.1 / 4.5)
- 청주대학교 산학협력단 KDT AI 시스템 반도체 설계 과정 (2026.04 ~ 2026.11 수료 예정)
- 청주대학교 SNSLAB 학부연구생 (2025.01 ~ 현재)
- Email: wjdxodnd02@naver.com

<br>

## Skills

| 분야 | 내용 |
|---|---|
| Front-End | Design Compiler, Formality |
| Back-End | IC Compiler II (Floorplan, PNS, Place, CTS, Route, Filler, Sign-off check), StarRC, PrimeTime, IC Validator |
| Simulation | VCS, Verdi, PSpice |
| Script / Language | **TCL (Tool별 run script, timing ECO script, DRC 위반 자동 수정 proc 작성)**, Verilog, Python, C, C++ |
| 기타 | Linux, FPGA, MCU (ATmega128), RTOS |

<br>

## Projects

| # | 프로젝트 | 핵심 내용 | Tool | 키워드 |
|---|---|---|---|---|
| 1 | [**Error Detection Code Block Design: Synthesis → P&R**](01_TOP_Synthesis_PnR/) | 같은 설계를 합성부터 Route, Filler, Sign-off check까지 완주. DRC 0 · LVS short/open 0 · PG floating 0 | DC, ICC2, Formality | **RTL-to-Layout<br>완주** |
| 2 | [**CISC: Synthesis → Sign-off**](03_CISC_Synthesis_Formality/) | 반전 clock(clk′)으로 생긴 half-cycle(0.7 ns) critical path를 합성에서 분석, CCD useful skew와 PT 자동 ECO 실패 후 수동 ECO(clone · Vt swap)로 sign-off. 4 scenario timing · DRC · LVS 위반 0 | DC, Formality, ICC2, StarRC, PrimeTime | **Half-cycle 경로<br>Timing Closure** |
| 3 | [**RISC16 + SRAM 합성 & TCL 자동화**](02_RISC16_Synthesis_TCL/) | fanout · transition 위반 자동 수정 TCL proc 작성, 3개 공정별 최종 결과 비교를 통한 PPA 비교 분석 | DC, Formality, VCS | **반복 작업<br>자동화/<br>PPA 분석** |
| 4 | [YOLOv5 객체 추종 운반 로봇 (졸업작품)](04_YOLOv5_Robot/) | Jetson Nano(상위) + ATmega128(하위) 제어 구조, YOLOv5 + DeepSORT 추종, 초음파 장애물 회피 | Python, C, Android | HW · SW<br>시스템<br>통합 |
| 5 | [임베디드 로봇 프로젝트](05_Embedded_Robot/) | ATmega128 기반 라인트레이서 → 장애물 회피 → 블루투스 RC카, 오실로스코프 디버깅 | C, Android | 단계별<br>디버깅 |
| 6 | [멀티플레이 테트리스](06_Multiplayer_Tetris/) | 최대 4인 접속, 자체 패킷 프로토콜, 송수신 스레드 분리 (소스 코드 포함) | C++ | 네트워크 ·<br>멀티스레드 |

<br>

## Highlights

**1. Error Detection Code Block: 합성부터 Sign-off check까지 한 설계로 완주** ([Project 1](01_TOP_Synthesis_PnR/))

| 단계 | Setup WNS (ns, slow corner) | Hold WNS (ns, fast corner) | Utilization |
|---|---|---|---|
| Place | +0.01828 | −0.00037 (1 path) | 0.3077 |
| CTS | +0.00972 | +0.01039 | 0.3285 |
| Route + Filler | −0.00031 (1 path, PT ECO 예정) | +0.00802 | 0.3297 |

DRC 0 · LVS short/open/floating 0 · PG floating 0 · Logic LVT 1.46 % (spec < 20 %)

**2. Half-cycle 경로를 합성부터 Sign-off까지 추적** ([Project 2](03_CISC_Synthesis_Formality/))

반전 clock(clk′)으로 동작하는 메모리로 인해 생긴 half-cycle(0.7 ns) 경로를 합성에서 critical path로 찾아 714 MHz spec을 만족하고, back-end에서는 classic CTS 대신 CCD(useful skew)를 선택.
Route 후 남은 setup 위반을 PrimeTime `fix_eco_timing`이 0개 수정하자, load별 slack scan으로 위반 load만 골라 MUX · OR clone과 Vt swap을 적용해 직접 해결.

| 단계 | Setup WNS (ns, tt0p78v125c) |
|---|---|
| Route | −0.00706 (1 path) |
| PT 자동 ECO | 0 fixed |
| 수동 ECO (sizing → MUX clone → OR clone → Vt swap) | +0.00032 |
| Final (Filler 후) | **+0.00027** · hold +0.03854 · DRC / LVS 0 |

합성 단계에서는 retiming으로 레지스터가 삭제된 LEC FAIL을 `only_map` → `dont_touch` 순서로 해결 (Formality 20 failing → 0).

**3. 반복 작업을 TCL로 자동화** ([Project 3](02_RISC16_Synthesis_TCL/))

주기 단축과 공정 이식 시 수십 개씩 생기는 max_fanout · max_transition 위반을 `split_fanout`, `report_transition_drivers` proc로 자동 수정. 공정에 종속되지 않도록 max_fanout 값과 buffer cell을 인자로 받게 설계해 3개 공정의 PPA 비교에 그대로 사용.

<br>

## Repository 구조

```
├── 01_TOP_Synthesis_PnR/          # Project 1: Error Detection Code Block
├── 02_RISC16_Synthesis_TCL/       # Project 3: RISC16
├── 03_CISC_Synthesis_Formality/   # Project 2: CISC (Synthesis → Sign-off)
├── 04_YOLOv5_Robot/
├── 05_Embedded_Robot/
└── 06_Multiplayer_Tetris/
    ├── README.md   # 프로젝트 요약
    ├── images/     # 핵심 슬라이드
    └── docs/       # 발표 자료 전체 (PDF)
```

각 폴더의 README에서 요약을 먼저 보고, 세부 내용은 `docs/`의 PDF에서 확인할 수 있습니다.

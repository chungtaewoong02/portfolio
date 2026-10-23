# 정태웅 | ASIC Physical Design Portfolio

> RTL 합성부터 P&R, Sign-off check까지 직접 수행하며 **문제의 원인을 리포트로 확인하고 수치로 검증하는** 신입 PD 엔지니어입니다.

- 청주대학교 전자공학과 (2027.02 졸업 예정, 4.1 / 4.5)
- 청주대학교 산학협력단 KDT AI 시스템 반도체 설계 과정 (2026.04 ~ 2026.11 수료 예정)
- 청주대학교 SNSLAB 학부연구생 (2024.11 ~ 현재)
- Email: wjdxodnd02@naver.com

<br>

## Skills

| 분야 | 내용 |
|---|---|
| Synthesis / STA | Design Compiler (Topographical, WLM), PrimeTime, Formality |
| Physical Design | IC Compiler II (Floorplan, PNS, Place, CTS, Route, Filler, Sign-off check), StarRC, IC Validator |
| Simulation | VCS, Verdi, PSpice |
| Script / Language | **TCL (DC 위반 수정 자동화 proc 직접 작성)**, Verilog, Python, C, C++ |
| 기타 | Linux, FPGA, MCU (ATmega128), RTOS |

<br>

## Projects

| # | 프로젝트 | 핵심 내용 | Tool | 분야 |
|---|---|---|---|---|
| 1 | [**TOP: Synthesis → P&R**](01_TOP_Synthesis_PnR/) | 같은 설계를 합성부터 Route, Filler, Sign-off check까지 완주. DRC 0 · LVS short/open 0 · PG floating 0 | DC, ICC2, Formality | **PD** |
| 2 | [**RISC16 + SRAM 합성 & TCL 자동화**](02_RISC16_Synthesis_TCL/) | fanout · transition 위반 자동 수정 TCL proc 작성, 3개 공정에서 수렴 주기 하한 탐색 | DC, Formality, VCS | **Synthesis / STA** |
| 3 | [**CISC 합성: Retiming과 Formality**](03_CISC_Synthesis_Formality/) | retiming으로 레지스터가 삭제되어 Formality FAIL → `only_map` + `dont_touch`로 해결 | DC, Formality, VCS | **Synthesis / Verification** |
| 4 | [YOLOv5 객체 추종 운반 로봇 (졸업작품)](04_YOLOv5_Robot/) | Jetson Nano + ATmega128 상하위 제어, YOLOv5 + DeepSORT 추종, 초음파 장애물 회피 | Python, C, Android | Embedded / AI |
| 5 | [임베디드 로봇 프로젝트](05_Embedded_Robot/) | ATmega128 기반 라인트레이서 → 장애물 회피 → 블루투스 RC카, 오실로스코프 디버깅 | C, Android | Embedded |
| 6 | [멀티플레이 테트리스](06_Multiplayer_Tetris/) | 최대 4인 접속, 자체 패킷 프로토콜, 송수신 스레드 분리 (소스 코드 포함) | C++ | Software |

<br>

## Highlights

**1. 합성부터 Sign-off check까지 한 설계로 완주** ([Project 1](01_TOP_Synthesis_PnR/))

| 단계 | Setup WNS (ns, slow corner) | Hold WNS (ns, fast corner) | Utilization |
|---|---|---|---|
| Place | +0.01828 | −0.00037 (1 path) | 0.3077 |
| CTS | +0.00972 | +0.01039 | 0.3285 |
| Route + Filler | −0.00031 (1 path, PT ECO 예정) | +0.00802 | 0.3297 |

DRC 0 · LVS short/open/floating 0 · PG floating 0 · Logic LVT 1.46 % (spec < 20 %)

**2. 반복 작업을 TCL로 자동화** ([Project 2](02_RISC16_Synthesis_TCL/))

주기 단축과 공정 이식 시 수십 개씩 생기는 max_fanout · max_transition 위반을 `split_fanout`, `report_transition_drivers` proc로 자동 수정. 공정에 종속되지 않도록 max_fanout 값과 buffer cell을 인자로 받게 설계.

**3. Tool 동작 원리까지 추적한 문제 해결** ([Project 3](03_CISC_Synthesis_Formality/))

`set_dont_retime`은 레지스터의 *이동*만 막고 *삭제*는 막지 못한다는 점을 확인하고, mapping 후 `dont_touch`를 거는 순서로 바꿔 Formality 20 failing → 0 failing.

<br>

## Repository 구조

```
├── 01_TOP_Synthesis_PnR/          # 메인 PD 프로젝트
├── 02_RISC16_Synthesis_TCL/
├── 03_CISC_Synthesis_Formality/
├── 04_YOLOv5_Robot/
├── 05_Embedded_Robot/
└── 06_Multiplayer_Tetris/
    ├── README.md   # 프로젝트 요약
    ├── images/     # 핵심 슬라이드
    └── docs/       # 발표 자료 전체 (PDF)
```

각 폴더의 README에서 요약을 먼저 보고, 세부 내용은 `docs/`의 PDF에서 확인할 수 있습니다.

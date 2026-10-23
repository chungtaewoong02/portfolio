# CISC 합성: Retiming과 Formality

[← 메인으로](../)

8-bit CISC 마이크로프로세서를 3개 공정에서 합성하면서, **retiming이 레지스터를 삭제해 Formality가 실패한 문제**를 원인부터 추적해 해결한 프로젝트입니다.

| 항목 | 내용 |
|---|---|
| 기간 | 2026 (KDT AI 시스템 반도체 설계 과정) |
| 설계 | 8-bit CISC (Controller, Memory, Bidirectional bus, Dual-phase clock) |
| Spec | 714 MHz (1.4 ns), LVT < 25 % (timing 목적 28 %까지 허용) |
| 라이브러리 | SAED32 32nm, 180nm, 130nm |
| Tool | Design Compiler, Formality, VCS |
| 상세 자료 | [발표 자료 PDF](docs/CISC_Synthesis.pdf) |

![Architecture](images/architecture.png)

<br>

## 결과 요약 (SAED32 32nm, Hierarchical)

| 항목 | 1.4 ns (spec) | 1.29 ns (하한) |
|---|---|---|
| Timing | MET | Reg-to-reg MET, Output WNS −0.048 ns (허용 −0.05 이내) |
| Cell area | 2,231 µm² | 2,355 µm² |
| LVT 비율 | 24.8 % | 26.9 % (28 % 허용 범위) |
| Total power | 1.12 mW | 1.38 mW |
| Formality | PASS | PASS |

<br>

## 핵심 1 — Retiming이 레지스터를 삭제한 문제

### 현상
`set_optimize_registers true`로 global retiming을 켜자 `mem1/temp_reg`가 redundant로 판단되어 삭제됨
→ Formality **20 failing + 41 unverified** compare points, RTL ≢ netlist

### 원인 분석
- `set_dont_retime`은 레지스터의 **이동**만 막고 **삭제**는 막지 못함
- mapping 전에 `dont_touch`를 걸면 mapping 자체가 막힘

### 해결
```
compile_ultra -only_map           # mapping만 먼저 수행 → temp_reg가 실제 cell로 존재
set_dont_touch mem1/temp_reg      # 이후 최적화에서 보호
compile_ultra -retime -incremental
```
→ Formality **122 passing, 0 failing**

| 실패 | 해결 |
|---|---|
| ![fail](images/formality_fail.png) | ![fix](images/formality_fix.png) |

<br>

## 핵심 2 — Half-cycle 경로가 정하는 주기 하한

- Memory 레지스터가 반전 clock(clk′)에서 latch → clk → clk′ 경로는 주기의 **절반(0.7 ns)** 안에 도착해야 함
- 이 경로가 32nm 전체의 critical path (1.4 ns에서 slack +0.000147 ns)
- 1.29 ns는 output WNS −0.048 ns로 허용치(−0.05) 이내, 1.28 ns는 −0.051 ns로 초과 → **하한 1.29 ns**
- 대가: LVT 24.8 % → 26.9 % (속도를 얻는 대신 leakage 증가)

| Half-cycle 경로 | 1.29 ns vs 1.28 ns |
|---|---|
| ![half cycle](images/half_cycle.png) | ![period floor](images/period_floor.png) |

<br>

## 공정별 비교

![cross process](images/cross_process.png)

- 다른 공정으로 옮길 때 spec의 출력 부하가 SAED32 셀 기준으로 정의되어 있어, 대상 라이브러리의 pin capacitance와 단위(fF ↔ pF)를 확인해 값을 다시 설정
- 180nm · 130nm는 1.4 ns에서 WNS −1.9 / −1.2 ns로 공정 속도 자체의 한계 확인

<br>

## 배운 점
- Retiming과 equivalence check는 따로 생각할 수 없고, 보존 전략을 함께 설계해야 한다는 것을 배웠습니다.
- Hierarchy를 유지하면 내부 신호를 VCS에서 관찰할 수 있어 gate-level 검증이 훨씬 쉬웠습니다.

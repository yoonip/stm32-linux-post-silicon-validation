# STM32F767ZI & Linux Kernel Post-Silicon Validation Testbench

ARM Cortex-M7(STM32F767ZI) 베어메탈 레지스터 제어, OpenOCD/GDB 인서킷 디버깅, Linux 커널 디바이스 드라이버, Python/Streamlit 자동화 계측을 통합한 4계층 하드웨어 검증 실습 기록입니다.

## 1. 실험 환경 및 조건
- **DUT:** NUCLEO-F767ZI (STM32F767ZI, ARM Cortex-M7, 16MHz HSI 기본 클록)
- **Host:** Raspberry Pi 4 (BCM2711, Debian 13 Trixie, Kernel `6.12.47+rpt-rpi-v8`)
- **Sensor & Interface:** OV5647 (MIPI-CSI-2), ST-Link/V2-1 (SWD + Virtual COM Port `/dev/ttyACM0` 115200bps)

## 2. 디렉터리 구성
- `01_baremetal_polling_slt/`: MMIO 베어메탈 펌웨어, Python UART SLT 스크립트, Streamlit 3-Tab 통합 대시보드
- `02_baremetal_nvic_dma_exti/`: ARM Cortex-M7 벡터 테이블, NVIC 수신 인터럽트(`IRQ 39`), DMA1 송신 오프로딩(`Stream 3, Ch 4`), 외부 핀 인터럽트(`EXTI3`, `IRQ 9`) 펌웨어
- `03_linux_kernel_driver/`: Linux 캐릭터 디바이스 드라이버(`slt_kdrv.ko`) 및 BCM2711 물리 `GPIO17`(커널 번호 `529`) 제어 드라이버(`slt_gpio_drv.ko`)

## 3. 실측 결과 및 트러블슈팅 사례
1. **UART 왕복 지연시간 및 수율:** 200회 연속 테스트 Yield 100%, 평균 응답 지연시간 `2.251 ~ 2.410 ms`.
2. **MIPI-CSI-2 프레임 간격 분석:** 3,500ms 캡처 기준 98 frames, 실측 `31.85 FPS`, 평균 간격 `31.399 ms`, 최대 지터 `0.004 ms`.
3. **하드웨어 핀맵 및 배선 결함 분석:**
   - NUCLEO-F767ZI `A0` 소켓이 `PA0`가 아닌 `PA3`에 연결된 회로 특성을 확인하고 `EXTI3_IRQHandler`(벡터 25번)로 매핑 수정.
   - `openocd`의 `mdw 0x40020010` (`GPIOA_IDR`) 메모리 조회로 핀 입력을 검증하고 RPi 4 40핀 헤더 배선(Pin 11 `GPIO17`, Pin 9 `GND`) 교정.

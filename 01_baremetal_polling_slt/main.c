#include <stdint.h>

// STM32F767ZI 하드웨어 레지스터 물리 주소 직접 정의 (MMIO)
#define RCC_AHB1ENR   (*(volatile uint32_t *)0x40023830)
#define RCC_APB1ENR   (*(volatile uint32_t *)0x40023840)

#define GPIOB_MODER   (*(volatile uint32_t *)0x40020400)
#define GPIOB_ODR     (*(volatile uint32_t *)0x40020414)
#define GPIOB_BSRR    (*(volatile uint32_t *)0x40020418)

#define GPIOD_MODER   (*(volatile uint32_t *)0x40020C00)
#define GPIOD_AFRH    (*(volatile uint32_t *)0x40020C24)

#define USART3_CR1    (*(volatile uint32_t *)0x40004800)
#define USART3_BRR    (*(volatile uint32_t *)0x4000480C)
#define USART3_ISR    (*(volatile uint32_t *)0x4000481C)
#define USART3_RDR    (*(volatile uint32_t *)0x40004824)
#define USART3_TDR    (*(volatile uint32_t *)0x40004828)

int main(void);
void Reset_Handler(void) { main(); while(1); }

// 부팅 직후 CPU가 가장 먼저 읽는 벡터 테이블 (초기 스택 포인터 + 리셋 핸들러 주소)
__attribute__((section(".isr_vector")))
const uint32_t vector_table[] = {
    0x20080000,               // 512KB RAM 최상단 스택 포인터(SP)
    (uint32_t)Reset_Handler   // 첫 실행 함수 주소(PC)
};

void uart_puts(const char *s) {
    while (*s) {
        while (!(USART3_ISR & (1U << 7))); // TXE 대기
        USART3_TDR = (uint8_t)(*s++);
    }
}

int main(void) {
    // 1. GPIOB(LED), GPIOD(UART3 핀), USART3 클록(전원) 켜기
    RCC_AHB1ENR |= (1U << 1) | (1U << 3);
    RCC_APB1ENR |= (1U << 18);

    // 2. PB0(녹색), PB7(파란색), PB14(빨간색) 출력 모드 설정
    GPIOB_MODER &= ~((3U << 0) | (3U << 14) | (3U << 28));
    GPIOB_MODER |=  ((1U << 0) | (1U << 14) | (1U << 28));

    // 3. PD8(TX), PD9(RX) 핀을 USART3(AF7) 모드로 설정
    GPIOD_MODER &= ~((3U << 16) | (3U << 18));
    GPIOD_MODER |=  ((2U << 16) | (2U << 18));
    GPIOD_AFRH  &= ~((0xFU << 0) | (0xFU << 4));
    GPIOD_AFRH  |=  ((7U << 0) | (7U << 4));

    // 4. USART3 통신 속도(115200 bps @ 기본 16MHz HSI 클록) 설정 및 활성화
    USART3_BRR = 16000000U / 115200U;
    USART3_CR1 = (1U << 3) | (1U << 2) | (1U << 0); // TE, RE, UE 활성화

    // 5. PC(Python SLT 자동화) 명령 수신 및 LED 제어 루프
    while (1) {
        if (USART3_ISR & (1U << 5)) { // RXNE 수신 확인
            uint8_t cmd = (uint8_t)(USART3_RDR & 0xFFU);
            if (cmd == '1') {
                GPIOB_ODR ^= (1U << 0);
                uart_puts("Green LED Toggled!\r\n");
            } else if (cmd == '2') {
                GPIOB_ODR ^= (1U << 7);
                uart_puts("Blue LED Toggled!\r\n");
            } else if (cmd == '3') {
                GPIOB_ODR ^= (1U << 14);
                uart_puts("Red LED Toggled!\r\n");
            } else if (cmd == '0') {
                GPIOB_BSRR = (1U << 16) | (1U << 23) | (1U << 30);
                uart_puts("All LEDs OFF!\r\n");
            }
        }
    }
}

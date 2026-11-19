#include <stdint.h>
#define RCC_AHB1ENR   (*(volatile uint32_t *)0x40023830)
#define RCC_APB1ENR   (*(volatile uint32_t *)0x40023840)
#define RCC_APB2ENR   (*(volatile uint32_t *)0x40023844)
#define GPIOA_MODER   (*(volatile uint32_t *)0x40020000)
#define GPIOA_PUPDR   (*(volatile uint32_t *)0x4002000C)
#define GPIOB_MODER   (*(volatile uint32_t *)0x40020400)
#define GPIOB_ODR     (*(volatile uint32_t *)0x40020414)
#define GPIOB_BSRR    (*(volatile uint32_t *)0x40020418)
#define GPIOD_MODER   (*(volatile uint32_t *)0x40020C00)
#define GPIOD_AFRH    (*(volatile uint32_t *)0x40020C24)
#define SYSCFG_EXTICR1 (*(volatile uint32_t *)0x40013808)
#define EXTI_IMR       (*(volatile uint32_t *)0x40013C00)
#define EXTI_RTSR      (*(volatile uint32_t *)0x40013C08)
#define EXTI_PR        (*(volatile uint32_t *)0x40013C14)
#define USART3_CR1    (*(volatile uint32_t *)0x40004800)
#define USART3_CR3    (*(volatile uint32_t *)0x40004808)
#define USART3_BRR    (*(volatile uint32_t *)0x4000480C)
#define USART3_ISR    (*(volatile uint32_t *)0x4000481C)
#define USART3_ICR    (*(volatile uint32_t *)0x40004820)
#define USART3_RDR    (*(volatile uint32_t *)0x40004824)
#define USART3_TDR    (*(volatile uint32_t *)0x40004828)
#define NVIC_ISER0    (*(volatile uint32_t *)0xE000E100)
#define NVIC_ISER1    (*(volatile uint32_t *)0xE000E104)
#define DMA1_LIFCR    (*(volatile uint32_t *)0x40026008)
#define DMA1_S3CR     (*(volatile uint32_t *)0x40026058)
#define DMA1_S3NDTR   (*(volatile uint32_t *)0x4002605C)
#define DMA1_S3PAR    (*(volatile uint32_t *)0x40026060)
#define DMA1_S3M0AR   (*(volatile uint32_t *)0x40026064)

int main(void);
void Reset_Handler(void) { main(); while(1); }
void Default_Handler(void) { while(1); }
void uart_dma_send(const char *str, uint32_t len) {
    while (DMA1_S3CR & 1U);
    DMA1_LIFCR = (0x3DU << 22);
    DMA1_S3PAR = (uint32_t)&USART3_TDR; DMA1_S3M0AR = (uint32_t)str; DMA1_S3NDTR = len;
    USART3_ICR = (1U << 6);
    DMA1_S3CR = (4U << 25) | (1U << 10) | (1U << 6) | (1U << 0);
}
void EXTI3_IRQHandler(void) {
    if (EXTI_PR & (1U << 3)) {
        EXTI_PR = (1U << 3); GPIOB_ODR ^= (1U << 7);
        uart_dma_send("HW_EXTI3_TRIGGERED: Blue LED Toggled!\r\n", 39);
    }
}
void USART3_IRQHandler(void) {
    if (USART3_ISR & (1U << 5)) {
        uint8_t cmd = (uint8_t)(USART3_RDR & 0xFFU);
        if (cmd == '1') { GPIOB_ODR ^= (1U << 0); uart_dma_send("DMA+IRQ: Green Toggled!\r\n", 25); }
        else if (cmd == '2') { GPIOB_ODR ^= (1U << 7); uart_dma_send("DMA+IRQ: Blue Toggled!\r\n", 24); }
        else if (cmd == '3') { GPIOB_ODR ^= (1U << 14); uart_dma_send("DMA+IRQ: Red Toggled!\r\n", 23); }
        else if (cmd == '0') { GPIOB_BSRR = (1U << 16) | (1U << 23) | (1U << 30); uart_dma_send("DMA+IRQ: All OFF!\r\n", 19); }
    }
}
__attribute__((section(".isr_vector")))
const uint32_t vector_table[] = {
    0x20080000, (uint32_t)Reset_Handler,
    (uint32_t)Default_Handler, (uint32_t)Default_Handler, (uint32_t)Default_Handler, (uint32_t)Default_Handler,
    (uint32_t)Default_Handler, (uint32_t)Default_Handler, (uint32_t)Default_Handler, (uint32_t)Default_Handler,
    (uint32_t)Default_Handler, (uint32_t)Default_Handler, (uint32_t)Default_Handler, (uint32_t)Default_Handler,
    (uint32_t)Default_Handler, (uint32_t)Default_Handler,
    (uint32_t)Default_Handler, (uint32_t)Default_Handler, (uint32_t)Default_Handler, (uint32_t)Default_Handler,
    (uint32_t)Default_Handler, (uint32_t)Default_Handler, (uint32_t)Default_Handler, (uint32_t)Default_Handler,
    (uint32_t)Default_Handler, (uint32_t)EXTI3_IRQHandler, (uint32_t)Default_Handler, (uint32_t)Default_Handler,
    (uint32_t)Default_Handler, (uint32_t)Default_Handler, (uint32_t)Default_Handler, (uint32_t)Default_Handler,
    (uint32_t)Default_Handler, (uint32_t)Default_Handler, (uint32_t)Default_Handler, (uint32_t)Default_Handler,
    (uint32_t)Default_Handler, (uint32_t)Default_Handler, (uint32_t)Default_Handler, (uint32_t)Default_Handler,
    (uint32_t)Default_Handler, (uint32_t)Default_Handler, (uint32_t)Default_Handler, (uint32_t)Default_Handler,
    (uint32_t)Default_Handler, (uint32_t)Default_Handler, (uint32_t)Default_Handler, (uint32_t)Default_Handler,
    (uint32_t)Default_Handler, (uint32_t)Default_Handler, (uint32_t)Default_Handler, (uint32_t)Default_Handler,
    (uint32_t)Default_Handler, (uint32_t)Default_Handler, (uint32_t)Default_Handler, (uint32_t)USART3_IRQHandler
};
volatile uint32_t cpu_work_counter = 0;
int main(void) {
    RCC_AHB1ENR |= (1U << 0) | (1U << 1) | (1U << 3) | (1U << 21);
    RCC_APB1ENR |= (1U << 18); RCC_APB2ENR |= (1U << 14);
    GPIOA_MODER &= ~(3U << 6); GPIOA_PUPDR &= ~(3U << 6); GPIOA_PUPDR |= (2U << 6);
    GPIOB_MODER &= ~((3U << 0) | (3U << 14) | (3U << 28));
    GPIOB_MODER |= ((1U << 0) | (1U << 14) | (1U << 28));
    GPIOD_MODER &= ~((3U << 16) | (3U << 18)); GPIOD_MODER |= ((2U << 16) | (2U << 18));
    GPIOD_AFRH &= ~((0xFU << 0) | (0xFU << 4)); GPIOD_AFRH |= ((7U << 0) | (7U << 4));
    SYSCFG_EXTICR1 &= ~(0xFU << 12); EXTI_RTSR |= (1U << 3); EXTI_IMR |= (1U << 3);
    USART3_BRR = 16000000U / 115200U; USART3_CR3 = (1U << 7);
    USART3_CR1 = (1U << 5) | (1U << 3) | (1U << 2) | (1U << 0);
    NVIC_ISER0 = (1U << 9); NVIC_ISER1 = (1U << (39 - 32));
    while (1) { cpu_work_counter++; }
}

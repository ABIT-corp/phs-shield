/*
 * STM32F4xx_Framework V1.0
 * @file	stm32f4xx_Startup.c
 * @brief	Set the Stack
 * 			Set the Vector Table
 * 			Configure FPU if used
 * 			Set to maximum Speed if used
 * 			Call main()
 */

/*----------Include-----------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/*----------Define------------------------------------------------------------*/


/*----------Stack-------------------------------------------------------------*/
__attribute__ ((section(".co_stack")))
unsigned long pulStack[STACK_SIZE];

/*----------Macro definition--------------------------------------------------*/
#define WEAK __attribute__ ((weak))

/*----------Declaration of the default interrupt handlers---------------------*/
void WEAK Reset_Handler(void);
void WEAK NMI_Handler(void);
void WEAK HardFault_Handler(void);
void WEAK MemManage_Handler(void);
void WEAK BusFault_Handler(void);
void WEAK UsageFault_Handler(void);
void WEAK SVC_Handler(void);
void WEAK DebugMon_Handler(void);
void WEAK PendSV_Handler(void);
void WEAK SysTick_Handler(void);
void WEAK WWDG_IRQHandler(void);
void WEAK PVD_IRQHandler(void);
void WEAK TAMP_STAMP_IRQHandler(void);
void WEAK RTC_WKUP_IRQHandler(void);
void WEAK FLASH_IRQHandler(void);
void WEAK RCC_IRQHandler(void);
void WEAK EXTI0_IRQHandler(void);
void WEAK EXTI1_IRQHandler(void);
void WEAK EXTI2_IRQHandler(void);
void WEAK EXTI3_IRQHandler(void);
void WEAK EXTI4_IRQHandler(void);
void WEAK DMA1_Stream0_IRQHandler(void);
void WEAK DMA1_Stream1_IRQHandler(void);
void WEAK DMA1_Stream2_IRQHandler(void);
void WEAK DMA1_Stream3_IRQHandler(void);
void WEAK DMA1_Stream4_IRQHandler(void);
void WEAK DMA1_Stream5_IRQHandler(void);
void WEAK DMA1_Stream6_IRQHandler(void);
void WEAK ADC_IRQHandler(void);
void WEAK CAN1_TX_IRQHandler(void);
void WEAK CAN1_RX0_IRQHandler(void);
void WEAK CAN1_RX1_IRQHandler(void);
void WEAK CAN1_SCE_IRQHandler(void);
void WEAK EXTI9_5_IRQHandler(void);
void WEAK TIM1_BRK_TIM9_IRQHandler(void);
void WEAK TIM1_UP_TIM10_IRQHandler(void);
void WEAK TIM1_TRG_COM_TIM11_IRQHandler(void);
void WEAK TIM1_CC_IRQHandler(void);
void WEAK TIM2_IRQHandler(void);
void WEAK TIM3_IRQHandler(void);
void WEAK TIM4_IRQHandler(void);
void WEAK I2C1_EV_IRQHandler(void);
void WEAK I2C1_ER_IRQHandler(void);
void WEAK I2C2_EV_IRQHandler(void);
void WEAK I2C2_ER_IRQHandler(void);
void WEAK SPI1_IRQHandler(void);
void WEAK SPI2_IRQHandler(void);
void WEAK USART1_IRQHandler(void);
void WEAK USART2_IRQHandler(void);
void WEAK USART3_IRQHandler(void);
void WEAK EXTI15_10_IRQHandler(void);
void WEAK RTC_Alarm_IRQHandler(void);
void WEAK OTG_FS_WKUP_IRQHandler(void);
void WEAK TIM8_BRK_TIM12_IRQHandler(void);
void WEAK TIM8_UP_TIM13_IRQHandler(void);
void WEAK TIM8_TRG_COM_TIM14_IRQHandler(void);
void WEAK TIM8_CC_IRQHandler(void);
void WEAK DMA1_Stream7_IRQHandler(void);
void WEAK FSMC_IRQHandler(void);
void WEAK SDIO_IRQHandler(void);
void WEAK TIM5_IRQHandler(void);
void WEAK SPI3_IRQHandler(void);
void WEAK UART4_IRQHandler(void);
void WEAK UART5_IRQHandler(void);
void WEAK TIM6_DAC_IRQHandler(void);
void WEAK TIM7_IRQHandler(void);
void WEAK DMA2_Stream0_IRQHandler(void);
void WEAK DMA2_Stream1_IRQHandler(void);
void WEAK DMA2_Stream2_IRQHandler(void);
void WEAK DMA2_Stream3_IRQHandler(void);
void WEAK DMA2_Stream4_IRQHandler(void);
void WEAK ETH_IRQHandler(void);
void WEAK ETH_WKUP_IRQHandler(void);
void WEAK CAN2_TX_IRQHandler(void);
void WEAK CAN2_RX0_IRQHandler(void);
void WEAK CAN2_RX1_IRQHandler(void);
void WEAK CAN2_SCE_IRQHandler(void);
void WEAK OTG_FS_IRQHandler(void);
void WEAK DMA2_Stream5_IRQHandler(void);
void WEAK DMA2_Stream6_IRQHandler(void);
void WEAK DMA2_Stream7_IRQHandler(void);
void WEAK USART6_IRQHandler(void);
void WEAK I2C3_EV_IRQHandler(void);
void WEAK I2C3_ER_IRQHandler(void);
void WEAK OTG_HS_EP1_OUT_IRQHandler(void);
void WEAK OTG_HS_EP1_IN_IRQHandler(void);
void WEAK OTG_HS_WKUP_IRQHandler(void);
void WEAK OTG_HS_IRQHandler(void);
void WEAK DCMI_IRQHandler(void);
void WEAK CRYP_IRQHandler(void);
void WEAK HASH_RNG_IRQHandler(void);
void WEAK FPU_IRQHandler(void);

/*----------Symbols defined in linker script----------------------------------*/
extern unsigned long _sidata; /*!< Start address for the initialization
 values of the .data section.            */
extern unsigned long _sdata; /*!< Start address for the .data section     */
extern unsigned long _edata; /*!< End address for the .data section       */
extern unsigned long _sbss; /*!< Start address for the .bss section      */
extern unsigned long _ebss; /*!< End address for the .bss section        */
extern void _eram; /*!< End address for ram                     */

/*----------Function prototypes-----------------------------------------------*/
extern int main(void); /*!< The entry point for the application. */
extern void SystemInit(void); /*!< Setup the microcontroller system(CMSIS) */
void Default_Reset_Handler(void); /*!< Default reset handler                */
void Default_Handler(void); /*!< Default interrupt handler            */

/*----------Vector Table------------------------------------------------------*/
/*
 * @brief 	Note that the proper constructs must be placed on this to ensure
 * 			that it ends up at physical address 0x00000000
 */
__attribute__ ((section(".isr_vector")))
void (* const g_pfnVectors[])(void) =
{
	(void *)&pulStack[(STACK_SIZE)-1], /*!< The initial stack pointer         */
	Reset_Handler, /*!< Reset Handler                               */
	NMI_Handler, /*!< NMI Handler                                 */
	HardFault_Handler, /*!< Hard Fault Handler                          */
	MemManage_Handler, /*!< MPU Fault Handler                           */
	BusFault_Handler, /*!< Bus Fault Handler                           */
	UsageFault_Handler, /*!< Usage Fault Handler                         */
	0,0,0,0, /*!< Reserved                                    */
	SVC_Handler, /*!< SVCall Handler                              */
	DebugMon_Handler, /*!< Debug Monitor Handler                       */
	0, /*!< Reserved                                    */
	PendSV_Handler, /*!< PendSV Handler                              */
	SysTick_Handler, /*!< SysTick Handler                             */
	WWDG_IRQHandler, /*!<  0: Window WatchDog                         */
	PVD_IRQHandler, /*!<  1: PVD through EXTI Line detection         */
	TAMP_STAMP_IRQHandler, /*!<  2: Tamper and TimeStamps through the EXTI line*/
	RTC_WKUP_IRQHandler, /*!<  3: RTC Wakeup through the EXTI line        */
	FLASH_IRQHandler, /*!<  4: FLASH                                   */
	RCC_IRQHandler , /*!<  5: RCC                                     */
	EXTI0_IRQHandler, /*!<  6: EXTI Line0                              */
	EXTI1_IRQHandler, /*!<  7: EXTI Line1                              */
	EXTI2_IRQHandler, /*!<  8: EXTI Line2                              */
	EXTI3_IRQHandler, /*!<  9: EXTI Line3                              */
	EXTI4_IRQHandler, /*!< 10: EXTI Line4                              */
	DMA1_Stream0_IRQHandler, /*!< 11: DMA1 Stream 0                           */
	DMA1_Stream1_IRQHandler, /*!< 12: DMA1 Stream 1                           */
	DMA1_Stream2_IRQHandler, /*!< 13: DMA1 Stream 2                           */
	DMA1_Stream3_IRQHandler, /*!< 14: DMA1 Stream 3                           */
	DMA1_Stream4_IRQHandler, /*!< 15: DMA1 Stream 4                           */
	DMA1_Stream5_IRQHandler, /*!< 16: DMA1 Stream 5                           */
	DMA1_Stream6_IRQHandler, /*!< 17: DMA1 Stream 6                           */
	ADC_IRQHandler, /*!< 18: ADC1, ADC2 and ADC3s                    */
	CAN1_TX_IRQHandler, /*!< 19: CAN1 TX                                 */
	CAN1_RX0_IRQHandler, /*!< 20: CAN1 RX0                                */
	CAN1_RX1_IRQHandler, /*!< 21: CAN1 RX1                                */
	CAN1_SCE_IRQHandler, /*!< 22: CAN1 SCE                                */
	EXTI9_5_IRQHandler, /*!< 23: External Line[9:5]s                     */
	TIM1_BRK_TIM9_IRQHandler, /*!< 24: TIM1 Break and TIM9                     */
	TIM1_UP_TIM10_IRQHandler, /*!< 25: TIM1 Update and TIM10                   */
	TIM1_TRG_COM_TIM11_IRQHandler,/*!< 26: TIM1 Trigger and Commutation and TIM11*/
	TIM1_CC_IRQHandler, /*!< 27: TIM1 Capture Compare                    */
	TIM2_IRQHandler, /*!< 28: TIM2                                    */
	TIM3_IRQHandler, /*!< 29: TIM3                                    */
	TIM4_IRQHandler, /*!< 30: TIM4                                    */
	I2C1_EV_IRQHandler, /*!< 31: I2C1 Event                              */
	I2C1_ER_IRQHandler, /*!< 32: I2C1 Error                              */
	I2C2_EV_IRQHandler, /*!< 33: I2C2 Event                              */
	I2C2_ER_IRQHandler, /*!< 34: I2C2 Error                              */
	SPI1_IRQHandler, /*!< 35: SPI1                                    */
	SPI2_IRQHandler, /*!< 36: SPI2                                    */
	USART1_IRQHandler, /*!< 37: USART1                                  */
	USART2_IRQHandler, /*!< 38: USART2                                  */
	USART3_IRQHandler, /*!< 39: USART3                                  */
	EXTI15_10_IRQHandler, /*!< 40: External Line[15:10]s                   */
	RTC_Alarm_IRQHandler, /*!< 41: RTC Alarm (A and B) through EXTI Line   */
	OTG_FS_WKUP_IRQHandler, /*!< 42: USB OTG FS Wakeup through EXTI line     */
	TIM8_BRK_TIM12_IRQHandler, /*!< 43: TIM8 Break and TIM12                    */
	TIM8_UP_TIM13_IRQHandler, /*!< 44: TIM8 Update and TIM13                   */
	TIM8_TRG_COM_TIM14_IRQHandler,/*!< 45:TIM8 Trigger and Commutation and TIM14*/
	TIM8_CC_IRQHandler, /*!< 46: TIM8 Capture Compare                    */
	DMA1_Stream7_IRQHandler, /*!< 47: DMA1 Stream7                            */
	FSMC_IRQHandler, /*!< 48: FSMC                                    */
	SDIO_IRQHandler, /*!< 49: SDIO                                    */
	TIM5_IRQHandler, /*!< 50: TIM5                                    */
	SPI3_IRQHandler, /*!< 51: SPI3                                    */
	UART4_IRQHandler, /*!< 52: UART4                                   */
	UART5_IRQHandler, /*!< 53: UART5                                   */
	TIM6_DAC_IRQHandler, /*!< 54: TIM6 and DAC1&2 underrun errors         */
	TIM7_IRQHandler, /*!< 55: TIM7                                    */
	DMA2_Stream0_IRQHandler, /*!< 56: DMA2 Stream 0                           */
	DMA2_Stream1_IRQHandler, /*!< 57: DMA2 Stream 1                           */
	DMA2_Stream2_IRQHandler, /*!< 58: DMA2 Stream 2                           */
	DMA2_Stream3_IRQHandler, /*!< 59: DMA2 Stream 3                           */
	DMA2_Stream4_IRQHandler, /*!< 60: DMA2 Stream 4                           */
	ETH_IRQHandler, /*!< 61: Ethernet                                */
	ETH_WKUP_IRQHandler, /*!< 62: Ethernet Wakeup through EXTI line       */
	CAN2_TX_IRQHandler, /*!< 63: CAN2 TX                                 */
	CAN2_RX0_IRQHandler, /*!< 64: CAN2 RX0                                */
	CAN2_RX1_IRQHandler, /*!< 65: CAN2 RX1                                */
	CAN2_SCE_IRQHandler, /*!< 66: CAN2 SCE                                */
	OTG_FS_IRQHandler, /*!< 67: USB OTG FS                              */
	DMA2_Stream5_IRQHandler, /*!< 68: DMA2 Stream 5                           */
	DMA2_Stream6_IRQHandler, /*!< 69: DMA2 Stream 6                           */
	DMA2_Stream7_IRQHandler, /*!< 70: DMA2 Stream 7                           */
	USART6_IRQHandler, /*!< 71: USART6                                  */
	I2C3_EV_IRQHandler, /*!< 72: I2C3 event                              */
	I2C3_ER_IRQHandler, /*!< 73: I2C3 error                              */
	OTG_HS_EP1_OUT_IRQHandler, /*!< 74: USB OTG HS End Point 1 Out              */
	OTG_HS_EP1_IN_IRQHandler, /*!< 75: USB OTG HS End Point 1 In               */
	OTG_HS_WKUP_IRQHandler, /*!< 76: USB OTG HS Wakeup through EXTI          */
	OTG_HS_IRQHandler, /*!< 77: USB OTG HS                              */
	DCMI_IRQHandler, /*!< 53: DCMI                                    */
	CRYP_IRQHandler, /*!< 53: CRYP crypto                             */
	HASH_RNG_IRQHandler, /*!< 53: Hash and Rng                            */
	FPU_IRQHandler /*!< 53: FPU                                     */
};

/*----------Default_Reset_Handler---------------------------------------------*/
void Default_Reset_Handler(void) {
	/* Initialize data and bss */
	unsigned long *pulSrc, *pulDest;

	/* Copy the data segment initializers from FLASH to SRAM */
	pulSrc = &_sidata;
	pulDest = &_sdata;

	//for(pulDest = &_sdata; pulDest < &_edata;)
	while (pulDest < &_edata) {
		*(pulDest++) = *(pulSrc++);
	}

	/* Zero fill the bss segment. This is done with inline assembly since this
	 will clear the value of pulDest if it is not kept in a register. */
	__asm("ldr     r0, =_sbss");
	__asm("ldr     r1, =_ebss");
	__asm("mov     r2, #0");
	__asm(".thumb_func");
	__asm("zero_loop:");
	__asm("cmp     r0, r1");
	__asm("it      lt");
	__asm("strlt   r2, [r0], #4");
	__asm("blt     zero_loop");

	/* Relocate Vector Table */
#if(VECT_TAB_SRAM == 1)
	SCB->VTOR = SRAM_BASE | VECT_TAB_OFFSET;
#else
	SCB->VTOR = FLASH_BASE | VECT_TAB_OFFSET;
#endif

	/* Setup the microcontroller system(CMSIS) */
	SystemInit();

	/* Set to maximum Speed from PLL driven by HSI Oscillator */
#if(SET_SYS_MAX_SPEED == 1)
	RCC_PLLConfig(RCC_PLLSource_HSI, 8, 168, 2, 7);
	RCC_PLLCmd(ENABLE);
	while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) != SET);

	FLASH_SetLatency(FLASH_Latency_5);

	RCC_HCLKConfig(RCC_SYSCLK_Div1);
	RCC_PCLK1Config(RCC_HCLK_Div4);
	RCC_PCLK2Config(RCC_HCLK_Div2);

	RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
#endif

	/* Call the application's entry point. */
	main();
}

/*----------Weak aliases------------------------------------------------------*/
/*
 * @brief	Any function with the same name will override this definition
 */
#pragma weak Reset_Handler = Default_Reset_Handler
#pragma weak NMI_Handler = Default_Handler1
#pragma weak HardFault_Handler = Default_Handler2
#pragma weak MemManage_Handler = Default_Handler3
#pragma weak BusFault_Handler = Default_Handler4
#pragma weak UsageFault_Handler = Default_Handler5
#pragma weak SVC_Handler = Default_Handler6
#pragma weak DebugMon_Handler = Default_Handler7
#pragma weak PendSV_Handler = Default_Handler8
#pragma weak SysTick_Handler = Default_Handler9
#pragma weak WWDG_IRQHandler = Default_Handler10
#pragma weak PVD_IRQHandler = Default_Handler11
#pragma weak TAMP_STAMP_IRQHandler = Default_Handler12
#pragma weak RTC_WKUP_IRQHandler = Default_Handler13
#pragma weak FLASH_IRQHandler = Default_Handler14
#pragma weak RCC_IRQHandler = Default_Handler15
#pragma weak EXTI0_IRQHandler = Default_Handler16
#pragma weak EXTI1_IRQHandler = Default_Handler17
#pragma weak EXTI2_IRQHandler = Default_Handler18
#pragma weak EXTI3_IRQHandler = Default_Handler19
#pragma weak EXTI4_IRQHandler = Default_Handler20
#pragma weak DMA1_Stream0_IRQHandler = Default_Handler21
#pragma weak DMA1_Stream1_IRQHandler = Default_Handler22
#pragma weak DMA1_Stream2_IRQHandler = Default_Handler23
#pragma weak DMA1_Stream3_IRQHandler = Default_Handler24
#pragma weak DMA1_Stream4_IRQHandler = Default_Handler25
#pragma weak DMA1_Stream5_IRQHandler = Default_Handler26
#pragma weak DMA1_Stream6_IRQHandler = Default_Handler27
#pragma weak ADC_IRQHandler = Default_Handler28
#pragma weak CAN1_TX_IRQHandler = Default_Handler29
#pragma weak CAN1_RX0_IRQHandler = Default_Handler30
#pragma weak CAN1_RX1_IRQHandler = Default_Handler31
#pragma weak CAN1_SCE_IRQHandler = Default_Handler32
#pragma weak EXTI9_5_IRQHandler = Default_Handler33
#pragma weak TIM1_BRK_TIM9_IRQHandler = Default_Handler34
#pragma weak TIM1_UP_TIM10_IRQHandler = Default_Handler35
#pragma weak TIM1_TRG_COM_TIM11_IRQHandler = Default_Handler36
#pragma weak TIM1_CC_IRQHandler = Default_Handler37
#pragma weak TIM2_IRQHandler = Default_Handler38
#pragma weak TIM3_IRQHandler = Default_Handler39
#pragma weak TIM4_IRQHandler = Default_Handler40
#pragma weak I2C1_EV_IRQHandler = Default_Handler41
#pragma weak I2C1_ER_IRQHandler = Default_Handler42
#pragma weak I2C2_EV_IRQHandler = Default_Handler43
#pragma weak I2C2_ER_IRQHandler = Default_Handler44
#pragma weak SPI1_IRQHandler = Default_Handler45
#pragma weak SPI2_IRQHandler = Default_Handler46
#pragma weak USART1_IRQHandler = Default_Handler47
#pragma weak USART2_IRQHandler = Default_Handler48
#pragma weak USART3_IRQHandler = Default_Handler49
#pragma weak EXTI15_10_IRQHandler = Default_Handler50
#pragma weak RTC_Alarm_IRQHandler = Default_Handler51
#pragma weak OTG_FS_WKUP_IRQHandler = Default_Handler52
#pragma weak TIM8_BRK_TIM12_IRQHandler = Default_Handler53
#pragma weak TIM8_UP_TIM13_IRQHandler = Default_Handler54
#pragma weak TIM8_TRG_COM_TIM14_IRQHandler = Default_Handler55
#pragma weak TIM8_CC_IRQHandler = Default_Handler56
#pragma weak DMA1_Stream7_IRQHandler = Default_Handler57
#pragma weak FSMC_IRQHandler = Default_Handler58
#pragma weak SDIO_IRQHandler = Default_Handler59
#pragma weak TIM5_IRQHandler = Default_Handler60
#pragma weak SPI3_IRQHandler = Default_Handler61
#pragma weak UART4_IRQHandler = Default_Handler62
#pragma weak UART5_IRQHandler = Default_Handler63
#pragma weak TIM6_DAC_IRQHandler = Default_Handler64
#pragma weak TIM7_IRQHandler = Default_Handler65
#pragma weak DMA2_Stream0_IRQHandler = Default_Handler66
#pragma weak DMA2_Stream1_IRQHandler = Default_Handler67
#pragma weak DMA2_Stream2_IRQHandler = Default_Handler68
#pragma weak DMA2_Stream3_IRQHandler = Default_Handler69
#pragma weak DMA2_Stream4_IRQHandler = Default_Handler70
#pragma weak ETH_IRQHandler = Default_Handler71
#pragma weak ETH_WKUP_IRQHandler = Default_Handler72
#pragma weak CAN2_TX_IRQHandler = Default_Handler73
#pragma weak CAN2_RX0_IRQHandler = Default_Handler74
#pragma weak CAN2_RX1_IRQHandler = Default_Handler75
#pragma weak CAN2_SCE_IRQHandler = Default_Handler76
#pragma weak OTG_FS_IRQHandler = Default_Handler77
#pragma weak DMA2_Stream5_IRQHandler = Default_Handler78
#pragma weak DMA2_Stream6_IRQHandler = Default_Handler79
#pragma weak DMA2_Stream7_IRQHandler = Default_Handler80
#pragma weak USART6_IRQHandler = Default_Handler81
#pragma weak I2C3_EV_IRQHandler = Default_Handler82
#pragma weak I2C3_ER_IRQHandler = Default_Handler83
#pragma weak OTG_HS_EP1_OUT_IRQHandler = Default_Handler84
#pragma weak OTG_HS_EP1_IN_IRQHandler = Default_Handler85
#pragma weak OTG_HS_WKUP_IRQHandler = Default_Handler86
#pragma weak OTG_HS_IRQHandler = Default_Handler87
#pragma weak DCMI_IRQHandler = Default_Handler88
#pragma weak CRYP_IRQHandler = Default_Handler89
#pragma weak HASH_RNG_IRQHandler = Default_Handler90
#pragma weak FPU_IRQHandler = Default_Handler91

/*----------Default_Handler---------------------------------------------------*/
/*
 * @brief	This is the code that gets called when the
 * 			processor receives an unexpected interrupt
 */

void GetRegistersFromStack( uint32_t *StackAddress,  uint32_t vector)
{
	uint32_t r0, r1, r2, r3, r12, lr, pc, psr, vector_register;

	r0  = StackAddress[ 0 ];
	r1  = StackAddress[ 1 ];
	r2  = StackAddress[ 2 ];
	r3  = StackAddress[ 3 ];
	r12 = StackAddress[ 4 ];
	lr  = StackAddress[ 5 ];
	pc  = StackAddress[ 6 ];
	psr = StackAddress[ 7 ];

	rtc_backup_write(10,  r0);
	rtc_backup_write(11,  r1);
	rtc_backup_write(12,  r2);
	rtc_backup_write(13,  r3);
	rtc_backup_write(14, r12);
	rtc_backup_write(15,  lr);
	rtc_backup_write(16,  pc);
	rtc_backup_write(17, psr);
	rtc_backup_write(19, vector);

	vector_register = rtc_backup_read(18);
	vector_register++;
	rtc_backup_write(18, vector_register);
#if 0
	while(1);
#else
	NVIC_SystemReset();
#endif
}

#define FAULT_HANDLER(value)                                      \
__asm volatile (                                                  \
      "tst   lr, #4                                           \n" \
      "ite   eq                                               \n" \
      "mrseq r0, msp                                          \n" \
      "mrsne r0, psp                                          \n" \
      "mov   r1, "#value                                     "\n" \
      "bl    GetRegistersFromStack                            \n" \
)


 void Default_Handler1(void) {
	 FAULT_HANDLER(1);
 }
 void Default_Handler2(void) {
	 FAULT_HANDLER(2);
 }
 void Default_Handler3(void) {
	 FAULT_HANDLER(3);
 }
 void Default_Handler4(void) {
	 FAULT_HANDLER(4);
 }
 void Default_Handler5(void) {
	 FAULT_HANDLER(5);
 }
 void Default_Handler6(void) {
	 FAULT_HANDLER(6);
 }
 void Default_Handler7(void) {
	 FAULT_HANDLER(7);
 }
 void Default_Handler8(void) {
	 FAULT_HANDLER(8);
 }
 void Default_Handler9(void) {
	 FAULT_HANDLER(9);
 }
 void Default_Handler10(void) {
	 FAULT_HANDLER(10);
 }
 void Default_Handler11(void) {
	 FAULT_HANDLER(11);
 }
 void Default_Handler12(void) {
	 FAULT_HANDLER(12);
 }
 void Default_Handler13(void) {
	 FAULT_HANDLER(13);
 }
 void Default_Handler14(void) {
	 FAULT_HANDLER(14);
 }
 void Default_Handler15(void) {
	 FAULT_HANDLER(15);
 }
 void Default_Handler16(void) {
	 FAULT_HANDLER(16);
 }
 void Default_Handler17(void) {
	 FAULT_HANDLER(17);
 }
 void Default_Handler18(void) {
	 FAULT_HANDLER(18);
 }
 void Default_Handler19(void) {
	 FAULT_HANDLER(19);
 }
 void Default_Handler20(void) {
	 FAULT_HANDLER(20);
 }
 void Default_Handler21(void) {
	 FAULT_HANDLER(21);
 }
 void Default_Handler22(void) {
	 FAULT_HANDLER(22);
 }
 void Default_Handler23(void) {
	 FAULT_HANDLER(23);
 }
 void Default_Handler24(void) {
	 FAULT_HANDLER(24);
 }
 void Default_Handler25(void) {
	 FAULT_HANDLER(25);
 }
 void Default_Handler26(void) {
	 FAULT_HANDLER(26);
 }
 void Default_Handler27(void) {
	 FAULT_HANDLER(27);
 }
 void Default_Handler28(void) {
	 FAULT_HANDLER(28);
 }
 void Default_Handler29(void) {
	 FAULT_HANDLER(29);
 }
 void Default_Handler30(void) {
	 FAULT_HANDLER(30);
 }
 void Default_Handler31(void) {
	 FAULT_HANDLER(31);
 }
 void Default_Handler32(void) {
	 FAULT_HANDLER(32);
 }
 void Default_Handler33(void) {
	 FAULT_HANDLER(33);
 }
 void Default_Handler34(void) {
	 FAULT_HANDLER(34);
 }
 void Default_Handler35(void) {
	 FAULT_HANDLER(35);
 }
 void Default_Handler36(void) {
	 FAULT_HANDLER(36);
 }
 void Default_Handler37(void) {
	 FAULT_HANDLER(37);
 }
 void Default_Handler38(void) {
	 FAULT_HANDLER(38);
 }
 void Default_Handler39(void) {
	 FAULT_HANDLER(39);
 }
 void Default_Handler40(void) {
	 FAULT_HANDLER(40);
 }
 void Default_Handler41(void) {
	 FAULT_HANDLER(41);
 }
 void Default_Handler42(void) {
	 FAULT_HANDLER(42);
 }
 void Default_Handler43(void) {
	 FAULT_HANDLER(43);
 }
 void Default_Handler44(void) {
	 FAULT_HANDLER(44);
 }
 void Default_Handler45(void) {
	 FAULT_HANDLER(45);
 }
 void Default_Handler46(void) {
	 FAULT_HANDLER(46);
 }
 void Default_Handler47(void) {
	 FAULT_HANDLER(47);
 }
 void Default_Handler48(void) {
	 FAULT_HANDLER(48);
 }
 void Default_Handler49(void) {
	 FAULT_HANDLER(49);
 }
 void Default_Handler50(void) {
	 FAULT_HANDLER(50);
 }
 void Default_Handler51(void) {
	 FAULT_HANDLER(51);
 }
 void Default_Handler52(void) {
	 FAULT_HANDLER(52);
 }
 void Default_Handler53(void) {
	 FAULT_HANDLER(53);
 }
 void Default_Handler54(void) {
	 FAULT_HANDLER(54);
 }
 void Default_Handler55(void) {
	 FAULT_HANDLER(55);
 }
 void Default_Handler56(void) {
	 FAULT_HANDLER(56);
 }
 void Default_Handler57(void) {
	 FAULT_HANDLER(57);
 }
 void Default_Handler58(void) {
	 FAULT_HANDLER(58);
 }
 void Default_Handler59(void) {
	 FAULT_HANDLER(59);
 }
 void Default_Handler60(void) {
	 FAULT_HANDLER(60);
 }
 void Default_Handler61(void) {
	 FAULT_HANDLER(61);
 }
 void Default_Handler62(void) {
	 FAULT_HANDLER(62);
 }
 void Default_Handler63(void) {
	 FAULT_HANDLER(63);
 }
 void Default_Handler64(void) {
	 FAULT_HANDLER(64);
 }
 void Default_Handler65(void) {
	 FAULT_HANDLER(65);
 }
 void Default_Handler66(void) {
	 FAULT_HANDLER(66);
 }
 void Default_Handler67(void) {
	 FAULT_HANDLER(67);
 }
 void Default_Handler68(void) {
	 FAULT_HANDLER(68);
 }
 void Default_Handler69(void) {
	 FAULT_HANDLER(69);
 }
 void Default_Handler70(void) {
	 FAULT_HANDLER(70);
 }
 void Default_Handler71(void) {
	 FAULT_HANDLER(71);
 }
 void Default_Handler72(void) {
	 FAULT_HANDLER(72);
 }
 void Default_Handler73(void) {
	 FAULT_HANDLER(73);
 }
 void Default_Handler74(void) {
	 FAULT_HANDLER(74);
 }
 void Default_Handler75(void) {
	 FAULT_HANDLER(75);
 }
 void Default_Handler76(void) {
	 FAULT_HANDLER(76);
 }
 void Default_Handler77(void) {
	 FAULT_HANDLER(77);
 }
 void Default_Handler78(void) {
	 FAULT_HANDLER(78);
 }
 void Default_Handler79(void) {
	 FAULT_HANDLER(79);
 }
 void Default_Handler80(void) {
	 FAULT_HANDLER(80);
 }
 void Default_Handler81(void) {
	 FAULT_HANDLER(81);
 }
 void Default_Handler82(void) {
	 FAULT_HANDLER(82);
 }
 void Default_Handler83(void) {
	 FAULT_HANDLER(83);
 }
 void Default_Handler84(void) {
	 FAULT_HANDLER(84);
 }
 void Default_Handler85(void) {
	 FAULT_HANDLER(85);
 }
 void Default_Handler86(void) {
	 FAULT_HANDLER(86);
 }
 void Default_Handler87(void) {
	 FAULT_HANDLER(87);
 }
 void Default_Handler88(void) {
	 FAULT_HANDLER(88);
 }
 void Default_Handler89(void) {
	 FAULT_HANDLER(89);
 }
 void Default_Handler90(void) {
	 FAULT_HANDLER(90);
 }
 void Default_Handler91(void) {
	 FAULT_HANDLER(91);
 }
 void Default_Handler92(void) {
	 FAULT_HANDLER(92);
 }
 void Default_Handler93(void) {
	 FAULT_HANDLER(93);
 }
 void Default_Handler94(void) {
	 FAULT_HANDLER(94);
 }
 void Default_Handler95(void) {
	 FAULT_HANDLER(95);
 }
 void Default_Handler96(void) {
	 FAULT_HANDLER(96);
 }
 void Default_Handler97(void) {
	 FAULT_HANDLER(97);
 }
 void Default_Handler98(void) {
	 FAULT_HANDLER(98);
 }
 void Default_Handler99(void) {
	 FAULT_HANDLER(99);
 }
 void Default_Handler100(void) {
	 FAULT_HANDLER(100);
 }

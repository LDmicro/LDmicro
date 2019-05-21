/*
Gestion des interruptions soous EmIde / GCCAVR:

Définir dans le programme la routine associée comme ci-dessous.
*/

void WWDG_Handler(void);		        	// WATCHDOG=0
void PVD_Handler(void);		        	    // PVD=1
void TAMP_STAMP_Handler(void);	    	    // TAMPER=2
void RTC_WKUP_Handler(void);		        // RTC=3
void FLASH_Handler(void);		    	    // FLASH=4
void RCC_Handler(void);			            // RCC=5
void EXTI0_Handler(void);		       	    // EXTI=6
void EXTI1_Handler(void);		    	    // EXTI=7
void EXTI2_Handler(void);			        // EXTI=8
void EXTI3_Handler(void);			        // EXTI=9
void EXTI4_Handler(void);			        // EXTI=10
void DMA1_S1_Handler(void);     	        // DMA1=11
void DMA1_S2_Handler(void);     	        // DMA1=12
void DMA1_S3_Handler(void);     	        // DMA1=13
void DMA1_S4_Handler(void);     	        // DMA1=14
void DMA1_S5_Handler(void);     	        // DMA1=15
void DMA1_S6_Handler(void);    	            // DMA1=16
void DMA1_S7_Handler(void);    	            // DMA1=17
void ADC_Handler(void);		        	    // ADC1-2-3=18
void CAN1_TX_Handler(void);		            // CAN1=19
void CAN1_RX0_Handler(void);	        	// CAN1=20
void CAN1_RX1_Handler(void);	        	// CAN1=21
void CAN1_SCE_Handler(void);	        	// CAN1=22
void EXTI9_5_Handler(void);	        	    // EXTI=23
void TIM1_BRK_Handler(void);	            // TIM1=24
void TIM1_UP_Handler(void);	                // TIM1=25
void TIM1_TRG_COM_TIM11_Handler(void);	    // TIM1=26
void TIM1_CC_Handler(void);		            // TIM1=27
void TIM2_Handler(void);   		            // TIM2=28
void TIM3_Handler(void);			        // TIM3=29
void TIM4_Handler(void);			        // TIM4=30
void I2C1_EV_Handler(void);	        	    // I2C1=31
void I2C1_ER_Handler(void);	        	    // I2C1=32
void I2C2_EV_Handler(void);	        	    // I2C2=33
void I2C2_ER_Handler(void);	        	    // I2C2=34
void SPI1_Handler(void);		        	// SPI1=35
void SPI2_Handler(void);		        	// SPI2=36
void USART1_Handler(void);	        	    // USART1=37
void USART2_Handler(void);	                // USART2=38
void USART3_Handler(void);	                // USART3=39
void EXTI15_10_Handler(void);               // EXTI=40
void RTC_ALARM_Handler(void);	            // RTC=41
void USB_WKUP_Handler(void);	            // USB=42


// Enable Interrupts (IRQ & FIQ) in CPSR (Current Program Status Reg)
void Interrupts_Enable()
    {
    __enable_irq();
    __enable_fault_irq();
    }

// Disable Interrupts (IRQ & FIQ) in CPSR (Current Program Status Reg)
void Interrupts_Disable()
    {
    __disable_irq();
    __disable_fault_irq();
    }



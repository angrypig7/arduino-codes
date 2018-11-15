/*===========================================================================
��ַ ��http://yhmcu.taobao.com/
���� ������  ԭ �ں͵��ӹ�����  �� �ڰ��ص��ӿƼ����޹�˾ 
�ʼ� ��yihe_liyong@126.com
�绰 ��18615799380
===========================================================================*/

#ifndef _BSP_H_
#define _BSP_H_

#include "STM8l10x_conf.h"
#include "nRF24L01.h"
#include "mytypedef.h"

// SPI���Ŷ���
#define PORT_SPI        GPIOB
#define PIN_SCLK        GPIO_Pin_5        
#define PIN_MOSI        GPIO_Pin_6
#define PIN_MISO        GPIO_Pin_7

// LED��KEY���Ŷ���
#define PORT_LED_R      GPIOB
#define PIN_LED_R       GPIO_Pin_0

#define PORT_LED_Y      GPIOD
#define PIN_LED_Y       GPIO_Pin_0

#define PORT_KEY        GPIOB
#define PIN_KEY         GPIO_Pin_1

#define PORT_USART      GPIOC
#define PIN_TXD         GPIO_Pin_3
#define PIN_RXD         GPIO_Pin_2

// LED����������(ON)��, (OFF)�رգ�(TOG)��ת
#define LED_R_ON()       GPIO_ResetBits(PORT_LED_R, PIN_LED_R)        
#define LED_R_OFF()      GPIO_SetBits(PORT_LED_R, PIN_LED_R)
#define LED_R_TOG()      GPIO_ToggleBits(PORT_LED_R, PIN_LED_R)

#define LED_Y_ON()       GPIO_ResetBits(PORT_LED_Y, PIN_LED_Y)        
#define LED_Y_OFF()      GPIO_SetBits(PORT_LED_Y, PIN_LED_Y)
#define LED_Y_TOG()      GPIO_ToggleBits(PORT_LED_Y, PIN_LED_Y)

void SClK_Initial(void);                // ��ʼ��ϵͳʱ�ӣ�ϵͳʱ�� = 16MHZ
void GPIO_Initial(void);                // ��ʼ��ͨ��IO�˿�
void SPI_Initial(void);                 // ��ʼ��SPI
void TIM3_Initial(void);                // ��ʼ����ʱ��3����ʱʱ��Ϊ1ms
void USART_Initial(void);               // ��ʼ������   
INT8U SPI_ExchangeByte(INT8U input);    // ͨ��SPI�������ݽ��� 

#endif //_BSP_H_

/*===========================================================================
-----------------------------------�ļ�����----------------------------------
===========================================================================*/

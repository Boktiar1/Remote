/*===========================================================================
TEST STM8L_REMOTE

FRAME DATA
 ____0_________1______________2___________3______4_______5______6_______7________8________9__
| Start | Add_Marker | Led_Left_Right | Time | multi | Goal | Sub_1 | Sub_2 | CheckSum | End |
---------------------------------------------------------------------------------------------
============================================================================*/

#include "bsp.h"  
//#include "spread_sheet.h" 
#include "SSD1306.h"

#define TX              1       
#define RX              0       

#define SEND_GAP        1000    
#define RECV_TIMEOUT    800     

#define ACK_LENGTH      10         
#define SEND_LENGTH     10     
extern INT8U Mode;
INT16U   Cnt1ms = 0;             
INT8U   SendFlag = 0;           

INT16U  SendTime = 1;           
INT16U  RecvWaitTime = 0;                      
INT16U  SendCnt = 0;  

INT16U  Current_Cnt = 0;
INT16U  Current_Cnt3GTarget = 0;         
INT16U  Current_Cnt4GTarget = 0;       
INT16U  Current_CntShootEarly = 0;

INT8U   Program_Run = 0;

INT8U   SendBuffer[SEND_LENGTH] = {0,0,0,0,0,0,0,0,0,0};

INT8U   COM_TxNeed = 0;
INT8U   COM_RxCounter = 0;
INT8U   COM_TxCounter = 0;
INT8U   COM_RxBuffer[65] = { 0 };
INT8U   COM_TxBuffer[65] = { 0 };

//==============================================================================

void delay_test(unsigned int x)
{
  while(x!=0){x--;}
}
#define USRAT_SendByte()    USART_SendData8(USART1,COM_TxBuffer[COM_TxCounter++])
#define USRAT_RecvByte()    COM_RxBuffer[COM_RxCounter++]=USART_ReceiveData8(USART1)

/*===========================================================================
TIMER2 CHECK BUTTON STATUS EVERY 200mS
============================================================================*/
void TIM2_ISR(void)// interrupt every 200ms
{
  Program_Run = Read_Button();
}

/*===========================================================================
TIMER3 WILL CREATE DELAY 1S
============================================================================*/
void DelayMs(INT16U x)
{
  TIM3_Cmd(ENABLE);
  Cnt1ms = 0;
  while (Cnt1ms <= x);
  TIM3_Cmd(DISABLE);
}

/*===========================================================================
TIMER3 WILL CREATE DELAY 1mS
============================================================================*/
void TIM3_1MS_ISR(void)
{
    Cnt1ms++;
    
    if (0 != RecvWaitTime)      { RecvWaitTime--; }     
        
    if (0 != SendTime)          
    { 
        if (--SendTime == 0)    { SendTime = SEND_GAP; SendFlag = 1; }
    } 
}

/*=============================================================================
UART SEND DATA FUNCTION
=============================================================================*/
void USART_Send(INT8U *buff, INT8U size)
{
    if (size == 0)          { return; }
    
    COM_TxNeed = 0;
    
    while (size --)         { COM_TxBuffer[COM_TxNeed++] = *buff++; }
    
    COM_TxCounter = 0;
    USART_ITConfig(USART1,USART_IT_TXE, ENABLE);
}

/*=============================================================================
UART RECEIVE DATA INTERRUPT FUNCTION
=============================================================================*/
void USART_RX_Interrupt(void)
{
    USRAT_RecvByte();
}

/*=============================================================================
UART SEND DATA INTERRUPT FUNCTION
=============================================================================*/
void USART_TX_Interrupt(void)
{
    if (COM_TxCounter < COM_TxNeed)     { USRAT_SendByte(); }   
    else    
    { 
        USART_ITConfig(USART1,USART_IT_TC, ENABLE);
        USART_ITConfig(USART1,USART_IT_TXE, DISABLE);

        if (USART_GetFlagStatus(USART1,USART_FLAG_TC))      
        {
            USART_ITConfig(USART1,USART_IT_TC, DISABLE); 
            COM_TxNeed = 0;
            COM_TxCounter = 0;
        }
    }
}

/*===========================================================================
INIT MCU FUNCTION
============================================================================*/
void MCU_Initial(void)
{
    SClK_Initial();             
    GPIO_Initial();                          
    TIM3_Initial();
    TIM2_Initial();
    USART_Initial();        
    SPI_Initial();                      
    Flash_Init();
    
    enableInterrupts();                   
}

/*===========================================================================
INIT RF FUNCTION
============================================================================*/
void RF_Initial(INT8U mode)
{
	CC1101Init();                                       
	if (RX == mode)     { CC1101SetTRMode(RX_MODE); }               
}

/*===========================================================================
INIT SYSTEM FUNCTION
============================================================================*/
void System_Initial(void)
{
    MCU_Initial();      
    RF_Initial(TX);             
}

/*===========================================================================
RF SEND DATA FUNCTION
============================================================================*/
INT8U RF_SendPacket(INT8U *Sendbuffer, INT8U length)
{
    //INT8U ack_flag = 0;        
    //INT8U error = 0, i=0, ack_len=0, ack_buffer[65]={ 0 };

    CC1101SendPacket(SendBuffer, length, ADDRESS_CHECK);   
    
    LEDDB_TOG();    
    
    //ack_flag = 1;
    return (1);  
}

/*===========================================================================
REMOTE SEND DATA FUNCTION
============================================================================*/
void Remote_SendData(uint8_t Add,uint32_t LedData,uint8_t Time,uint8_t Multi,uint8_t Goal,uint8_t Sub_1,uint8_t Sub_2)
{
  uint8_t CheckSum =0;
  CheckSum = Add+LedData+Time+Multi+Goal+Sub_1+Sub_2;
  SendBuffer[0]=STX;
  SendBuffer[1]=Add;
  
  SendBuffer[2]=LedData;
  SendBuffer[3]=Time;
  SendBuffer[4]=Multi;
  SendBuffer[5]=Goal;
  SendBuffer[6]=Sub_1;
  SendBuffer[7]=Sub_2;
  SendBuffer[8]=CheckSum;
  SendBuffer[9]=ETX; 
  RF_SendPacket(SendBuffer, SEND_LENGTH);
  SendFlag=0;
}
/*===========================================================================
MAIN FUNCTION
===============================REMOTE=============================================*/
void main(void)
{
  System_Initial();
  
  if (Delays_Code_Value != FLASH_ReadByte((FLASH_DATA_EEPROM_START_PHYSICAL_ADDRESS + Epr_Add_Delays_Code))) { Epr_Store_Factory_Delays(0); }  //stores factory delays first time program is run, but never again
  
  Clear_All_Key_Button();
  delay_test(0xffff);
  delay_test(0xffff);

  for (int i=0;i<1;i++)
  {
	LEDP5_TOG();
	DelayMs(800);
	LEDP5_TOG();
	LEDP4_TOG();
	DelayMs(600);
	LEDP4_TOG();
	LEDP3_TOG();
	DelayMs(400);
	LEDP3_TOG();
	LEDP2_TOG();
	DelayMs(200);
	LEDP2_TOG();
	LEDP1_TOG();
	DelayMs(100);
	LEDP1_TOG();
	USART_SendData8(USART1, 0XFF);
  }
  
  USART_Send("Start ok\r\n", 12); 

  while (1)
  {
    switch (Program_Run)
    {
      case P1_Run:
      {
        USART_Send("P1 Running\r\n", 13); 
        DelayMs(100);
        All_Program(P1_Run);
        Mode=0;// clear bien detetct select program
        Program_Run=0;
        break;//
      }
      case P2_Run:
      {
        USART_Send("P2 Running\r\n", 13);
        DelayMs(100);
	All_Program(P2_Run);
        Mode=0;
        Program_Run=0;
        break;
      }
      case P3_Run:
      {
        USART_Send("P3 Running\r\n", 13);
        DelayMs(100);
	All_Program(P3_Run);
        Mode=0;
        Program_Run=0;
        break;
      }
      case P4_Run:
      {
        USART_Send("P4 Running\r\n", 13);
        DelayMs(100);
	All_Program(P4_Run);
        Mode=0;
        Program_Run=0;
        break;
      }
      case P5_Run:
      {
        USART_Send("P5 Running\r\n", 13);
	DelayMs(100);
        All_Program(P5_Run);
        Mode=0;
        Program_Run=0;
        break;
      }
      
      case P1_C3G_Run:
      {
        USART_Send("P13G Running\r\n", 13);
	DelayMs(100);
	All_Program(P1_C3G_Run);
        Mode=0;
        Program_Run=0;
        break;
      }
      case P2_C3G_Run:
      {
        USART_Send("P23G Running\r\n", 13);
	DelayMs(100);
	All_Program(P2_C3G_Run);
        Mode=0;
        Program_Run=0;
        break;
      }
      case P3_C3G_Run:
      {
        USART_Send("P33G Running\r\n", 13);
	DelayMs(100);
	All_Program(P3_C3G_Run);
        Mode=0;
        Program_Run=0;
        break;
      }
      case P1_C4G_Run:
      {
        USART_Send("P14G Running\r\n", 13);
	DelayMs(100);
	All_Program(P1_C4G_Run);
        Mode=0;
        Program_Run=0;
        
        break;
      }
      case P2_C4G_Run:
      {
        USART_Send("P24G Running\r\n", 13);
	DelayMs(100);
	All_Program(P2_C4G_Run);
        Mode=0;
        Program_Run=0;
        break;
      }
      case P3_C4G_Run:
      {
        USART_Send("P34G Running\r\n", 13);
	DelayMs(100);
	All_Program(P3_C4G_Run);
        Mode=0;
        Program_Run=0;
        break;
      } 
    //----------------------------------Reverse Mode----------------------------
      case P1_Reverse:
      {
        USART_Send("P1RV Running\r\n", 13);
	DelayMs(100);
	All_Program(P1_Reverse);
        Mode=0;
        Program_Run=0;
        break;//
      }
      case P2_Reverse:
      {
        USART_Send("P2RV Running\r\n", 13);
	DelayMs(100);
	All_Program(P2_Reverse);
        Mode=0;
        Program_Run=0;
        break;
      }
      case P3_Reverse:
      {
        USART_Send("P3RV Running\r\n", 13);
	DelayMs(100);
	All_Program(P3_Reverse);
        Mode=0;
        Program_Run=0;
        break;
      }
      case P4_Reverse:
      {
        USART_Send("P4RV Running\r\n", 13);
	DelayMs(100);
	All_Program(P4_Reverse);
        Mode=0;
        Program_Run=0;
        break;
      }
      case P5_Reverse:
      {
        //OLED_print_string(2, 2, "Reverse P5 runing");
        USART_Send("P5RV Running\r\n", 13);
	DelayMs(100);
	All_Program(P5_Reverse);
        Mode=0;
        Program_Run=0;
        break;
      }
    default: {break;}
    }
  }
}
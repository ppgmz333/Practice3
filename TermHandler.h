/*
 * TermHandler.h
 *
 *  Created on: 21/10/2016
 *      Author: Patricio Gomez
 */

#ifndef SOURCES_TERMHANDLER_H_
#define SOURCES_TERMHANDLER_H_


#include "UART.h"
#include "GPIO.h"
#include "NVIC.h"
#include "FIFO.h"

//#define SYSTEM_CLOCK 110000000
#define SYSTEM_CLOCK 21000000

typedef enum{
	MenuDisp = 0,
	ReadI2CDisp,
	WriteI2CDisp,
	SetHourDisp,
	SetDateDisp,
	SetHourFormatDisp,
	ReadHourDisp,
	ReadDateDisp,
	CommunicationDisp
}Term_MenuDisplayType;

typedef enum{
	Option_param,
	Hour_param,
	Date_param,
	Address_param,
	Data_param,
	Len_param
}Term_inputParametersType;

typedef void (*ftpr_Disp)(UART_ChannelType);

typedef struct{
	char id;
	uint8 currentMenu :4;
	uint8 currentMenuParameter;
	uint16 address;
	char char_address [6];
	uint32 data;
	uint32 len;
	char char_len [8];
	uint32 tempdata;
	uint32 shift_counter;
	FIFO_Type f;
}Term_StateMachineType;

typedef struct{
	uint8 MemBusy :1;
	uint8 RTCBusy :1;
	uint8 Term1Com :1;
	uint8 Term2Com :1;
	uint8 LCDBusy :1;
}TermHandler_StateMachineType;

typedef void (*ftpr_Update)(UART_ChannelType, Term_StateMachineType*);

uint8 TERMHANDLER_init();

uint8 TERM1_init();

uint8 TERM2_init();

void TERM_UGLY_upd(UART_ChannelType uartChannel, Term_StateMachineType* statemachine);

uint8 TERM_upd();

void TERM_MenuDisp(UART_ChannelType uartChannel, Term_StateMachineType* statemachine);
void TERM_ReadMem(UART_ChannelType uartChannel, Term_StateMachineType* statemachine);
void TERM_WriteMem(UART_ChannelType uartChannel, Term_StateMachineType* statemachine);
void TERM_WriteHour(UART_ChannelType uartChannel, Term_StateMachineType* statemachine);
void TERM_WriteDate(UART_ChannelType uartChannel, Term_StateMachineType* statemachine);
void TERM_WriteFormat(UART_ChannelType uartChannel, Term_StateMachineType* statemachine);
void TERM_ReadHour(UART_ChannelType uartChannel, Term_StateMachineType* statemachine);
void TERM_ReadDate(UART_ChannelType uartChannel, Term_StateMachineType* statemachine);
void TERM_communication(UART_ChannelType uartChannel, Term_StateMachineType* statemachine);
void TERM_LCD(UART_ChannelType uartChannel, Term_StateMachineType* statemachine);




#endif /* SOURCES_TERMHANDLER_H_ */

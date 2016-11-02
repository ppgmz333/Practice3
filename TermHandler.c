/*
 * TermHandler.c
 *
 *  Created on: 21/10/2016
 *      Author: Patricio Gomez
 */

#include "TermHandler.h"
#include "MCP7940M.h"
#include "M24LC256.h"
#include "TermDisplay.h"
#include "PIT.h"
#include "DISP.h"
#include "LCDNokia5110.h"
#include "BTTN.h"

RTC_ConfigType Struct_RTC_W = {
		6,
		41,
		17,
		22,
		1,
		18,
		17,
		16
};

RTC_ConfigType Struct_RTC_R = {
		6,
		41,
		17,
		22,
		1,
		18,
		17,
		16
};

RTC_CharArray Struct_Char_W = {
		"00:00:00 AM",
		"\0",
		"01/01/2000",
		"\0",
		1
};

RTC_CharArray Struct_Char_R = {
		"00:00:00 AM",
		"\0",
		"01/01/2000",
		"\0",
		1
};

ftpr_Disp ftpr_Disp_Array[20] = {
								//////
		&TERM_menuDisp,			//0 //
		&TERM_readI2CDisp1,		//1 //
		&TERM_writeI2CDisp1,	//2 //
		&TERM_setHourDisp1,		//3 //
		&TERM_setDateDisp1,		//4 //
		&TERM_setHourFormatDisp,//5 //
		&TERM_readHourDisp,		//6 //
		&TERM_readDateDisp,		//7 //
		&TERM_communicationDisp,//8 //
		&TERM_lcdDisp,			//9 //

		&TERM_readI2CDisp2,		//10//
		&TERM_readI2CDisp3,		//11//
		&TERM_writeI2CDisp2,	//12//
		&TERM_writeI2CDisp3,	//13//
		&TERM_setHourDisp2,		//14//
		&TERM_setDateDisp2,		//15//

		&TERM_readHourDisp1,	//16//
		&TERM_readDateDisp1,	//17//

		&TERM_ErrorDisp,		//18//
		&TERM_BusyDisp			//19//
								//////
};

ftpr_Update ftpr_Update_Array [10]= {
		&TERM_MenuDisp,
		&TERM_ReadMem,
		&TERM_WriteMem,
		&TERM_WriteHour,
		&TERM_WriteDate,
		&TERM_WriteFormat,
		&TERM_ReadHour,
		&TERM_ReadDate,
		&TERM_communication,
		&TERM_LCD
};

Term_StateMachineType Term1_StateMachine = {
		'1',
		MenuDisp,
		Option_param,
		0x0,
		"0000",
		0,
		0,
		"00",
		0,
		0,
		{{},0,0}
};

Term_StateMachineType Term2_StateMachine = {
		'2',
		MenuDisp,
		Option_param,
		0x0,
		"0000",
		0,
		0,
		"00",
		0,
		0,
		{{},0,0}
};

TermHandler_StateMachineType TermHandler_StateMachine = {
		FALSE, FALSE, FALSE, FALSE, FALSE
};


static uint8 timeout_flag = FALSE;
static uint8 timeout_notification = FALSE;
static uint8 manual_mode = FALSE;
static uint8 manual_counter_hour = FALSE;
static uint8 manual_counter_date = FALSE;

uint8 TERM_upd(){

	if(UART_MailBoxFlag(UART_0)){
	TermHandler_StateMachine.id = '1';
	(*ftpr_Update_Array[Term1_StateMachine.currentMenu])(UART_0, &Term1_StateMachine);

	}

	if(UART_MailBoxFlag(UART_4)){
	TermHandler_StateMachine.id = '2';
	(*ftpr_Update_Array[Term2_StateMachine.currentMenu])(UART_4, &Term2_StateMachine);
	}

	TermHandler_StateMachine.id = '0';

	if(PIT_mailBoxFlag(PIT_0) == TRUE){

		timeout_Enable();
		while(I2C_busy(I2C_0) == TRUE);
		RTC_newRead(&Struct_RTC_R, &Struct_Char_R);
		timeout_Disable();
		if((Term1_StateMachine.currentMenu == ReadHourDisp)){
			(*ftpr_Disp_Array[16])(UART_0, &Struct_Char_R);
		} else if((Term1_StateMachine.currentMenu == ReadDateDisp)){
			(*ftpr_Disp_Array[17])(UART_0, &Struct_Char_R);
		}
		if((Term2_StateMachine.currentMenu == ReadHourDisp)){
			(*ftpr_Disp_Array[16])(UART_4, &Struct_Char_R);
		} else if((Term2_StateMachine.currentMenu == ReadDateDisp)){
			(*ftpr_Disp_Array[17])(UART_4, &Struct_Char_R);
		}

		if((TermHandler_StateMachine.LCDBusy == FALSE) && (manual_mode == FALSE)){
		Info_Display(&Struct_Char_R);
		}
	}

	if(BTTN_mailBoxFlag() == TRUE){
		Button_Hour();
	}
}


void TERM_ReadMem(UART_ChannelType uartChannel, Term_StateMachineType* statemachine){
	switch(statemachine->currentMenuParameter){
	case Option_param:

		statemachine->char_address[0] = (UART_MailBoxData(uartChannel));
		statemachine->shift_counter++;
		statemachine->currentMenuParameter = Address_param;
		break;

	case Address_param:
		if(UART_MailBoxData(uartChannel) != 13 && (statemachine->shift_counter != 4)){

			statemachine->char_address[statemachine->shift_counter] = (UART_MailBoxData(uartChannel));
			statemachine->shift_counter++;

		}
		if((UART_MailBoxData(uartChannel) == 13) || (statemachine->shift_counter == 4)){
			statemachine->shift_counter = 0;
			statemachine->currentMenuParameter = Len_param;
			(*ftpr_Disp_Array[10])(uartChannel, &Struct_Char_R);
		}
		break;

	case Len_param:
		if(UART_MailBoxData(uartChannel) != 13 && (statemachine->shift_counter != 2)){
			statemachine->char_len[statemachine->shift_counter] = (UART_MailBoxData(uartChannel));
			statemachine->shift_counter++;

		}
		if((UART_MailBoxData(uartChannel) == 13) || (statemachine->shift_counter == 2)){
			statemachine->shift_counter = 0;
			statemachine->currentMenuParameter = Data_param;

			(*ftpr_Disp_Array[11])(uartChannel, &Struct_Char_R);

			Cast_Memory_param(statemachine);

			while(statemachine->len > 0){

				timeout_Enable();
				UART_putChar(uartChannel, MEM_read(statemachine->address));
				timeout_Disable();
				statemachine->len -= 1;
				statemachine->address += 8;
			}

			statemachine->address = 0;

			UART_putString(uartChannel, "\r\nPresione cualquier tecla para continuar... ");
		}

		break;

	case Data_param:
		UART_MailBoxData(uartChannel);
		statemachine->currentMenuParameter = Option_param;
		statemachine->currentMenu = MenuDisp;
		(*ftpr_Disp_Array[statemachine->currentMenu])(uartChannel, &Struct_Char_R);
		TermHandler_StateMachine.MemBusy = FALSE;

		break;
	}
}
void TERM_WriteMem(UART_ChannelType uartChannel, Term_StateMachineType* statemachine){

	switch(statemachine->currentMenuParameter){
	case Option_param:

		statemachine->char_address[0] = (UART_MailBoxData(uartChannel));
		statemachine->shift_counter++;
		statemachine->currentMenuParameter = Address_param;
		break;

	case Address_param:
		if(UART_MailBoxData(uartChannel) != 13 && (statemachine->shift_counter != 4) && UART_MailBoxData(uartChannel) != 10){

			statemachine->char_address[statemachine->shift_counter] = (UART_MailBoxData(uartChannel));
			statemachine->shift_counter++;

		}
		if((UART_MailBoxData(uartChannel) == 13) || (statemachine->shift_counter == 4) || (UART_MailBoxData(uartChannel) == 10)){
			statemachine->shift_counter = 0;
			statemachine->currentMenuParameter = Data_param;
			(*ftpr_Disp_Array[12])(uartChannel, &Struct_Char_R);
		}
		break;

	case Data_param:

		if ((UART_MailBoxData(uartChannel) == 13)){
			UART_putString(uartChannel, "\r\n[Enter]");
			UART_putString(uartChannel, "\r\nSu texto ha sido guardado!\r\nPresione cualquier tecla para continuar");
			statemachine->currentMenuParameter = Len_param;
			statemachine->address = 0;
		//what if we receive any other type of data
			} else {
				////

				if(statemachine->address < 0x7FFF){
				timeout_Enable();
				MEM_write(statemachine->address, UART_MailBoxData(uartChannel));
				timeout_Disable();
				statemachine->address += 8;
				}
			}

		break;

	case Len_param:
		UART_MailBoxData(uartChannel);
		statemachine->currentMenuParameter = Option_param;
		statemachine->currentMenu = MenuDisp;
		(*ftpr_Disp_Array[statemachine->currentMenu])(uartChannel, &Struct_Char_R);
		TermHandler_StateMachine.MemBusy = FALSE;

		break;
	}
}

void TERM_WriteHour(UART_ChannelType uartChannel, Term_StateMachineType* statemachine){
	if(statemachine->shift_counter < 8){
		Struct_Char_W.Time_Char[statemachine->shift_counter] =
				(((UART_MailBoxData(uartChannel) - '0') >= 0)
						&& ((UART_MailBoxData(uartChannel) - '0') <= 9) )?
								((UART_MailBoxData(uartChannel))):((UART_MailBoxData(uartChannel) == ':')?
										(':'):('1'));
		statemachine->shift_counter++;
	} else if((UART_MailBoxData(uartChannel) == 13) && (statemachine->shift_counter == 8)){
		UART_MailBoxData(uartChannel);
		Struct_Char_W.Time_Char[2] = ':';
		Struct_Char_W.Time_Char[5] = ':';
		Hour_Check(&Struct_Char_W ,&Struct_RTC_W);

		timeout_Enable();
		RTC_writeHour(&Struct_RTC_W);
		timeout_Disable();

		(*ftpr_Disp_Array[14])(uartChannel, &Struct_Char_R);

		statemachine->currentMenuParameter = Len_param;

		return;
	} else if(statemachine->currentMenuParameter == Len_param){
		UART_MailBoxData(uartChannel);
		statemachine->shift_counter = 0;
		statemachine->currentMenu = MenuDisp;
		statemachine->currentMenuParameter = Option_param;
		(*ftpr_Disp_Array[statemachine->currentMenu])(uartChannel, &Struct_Char_R);
		TermHandler_StateMachine.RTCBusy = FALSE;

	} else {
		UART_putString(uartChannel, "\r\nSe ha excedido del numero de caracteres,\r\n");
		UART_putString(uartChannel, "se ignorara el utimo caracter ... (Presione ENTER)");
	}
}

void TERM_WriteDate(UART_ChannelType uartChannel, Term_StateMachineType* statemachine){
	if(statemachine->shift_counter < 10){
			Struct_Char_W.Date_Char[statemachine->shift_counter] =
					(((UART_MailBoxData(uartChannel) - '0') >= 0)
							&& ((UART_MailBoxData(uartChannel) - '0') <= 9) )?
									((UART_MailBoxData(uartChannel))):((UART_MailBoxData(uartChannel) == '/')?
											('/'):('1'));
			statemachine->shift_counter++;

		} else if((UART_MailBoxData(uartChannel) == 13) && (statemachine->shift_counter == 10)){
			UART_MailBoxData(uartChannel);
			Struct_Char_W.Date_Char[2] = '/';
			Struct_Char_W.Date_Char[5] = '/';
			Date_Check(&Struct_Char_W ,&Struct_RTC_W);

			timeout_Enable();
			RTC_writeDate(&Struct_RTC_W);
			timeout_Disable();

			Struct_Char_R.Date_Char[6] = Struct_Char_W.Date_Char[6];
			Struct_Char_R.Date_Char[7] = Struct_Char_W.Date_Char[7];

			(*ftpr_Disp_Array[15])(uartChannel, &Struct_Char_R);

			statemachine->currentMenuParameter = Len_param;

			return;
		} else if(statemachine->currentMenuParameter == Len_param){
			UART_MailBoxData(uartChannel);
			statemachine->shift_counter = 0;
			statemachine->currentMenu = MenuDisp;
			statemachine->currentMenuParameter = Option_param;
			(*ftpr_Disp_Array[statemachine->currentMenu])(uartChannel, &Struct_Char_R);
			TermHandler_StateMachine.RTCBusy = FALSE;

		} else {
			UART_putString(uartChannel, "\r\nSe ha excedido del numero de caracteres,\r\n");
			UART_putString(uartChannel, "se ignorara el utimo caracter ... (Presione ENTER)");
		}

}
void TERM_WriteFormat(UART_ChannelType uartChannel, Term_StateMachineType* statemachine){
	if((UART_MailBoxData(uartChannel) == 13)){

		statemachine->currentMenu = MenuDisp;
		statemachine->currentMenuParameter = Option_param;
		statemachine->shift_counter = 0;
		(*ftpr_Disp_Array[statemachine->currentMenu])(uartChannel, &Struct_Char_R);
		TermHandler_StateMachine.RTCBusy = FALSE;


	} else if(statemachine->shift_counter <= 1){
		statemachine->char_len[statemachine->shift_counter] = (UART_MailBoxData(uartChannel));
		statemachine->shift_counter++;

	if(statemachine->shift_counter == 2){
		if((((statemachine->char_len[0] == 'S'))&&((statemachine->char_len[1] == 'i')))
				||(((statemachine->char_len[0] == 's'))&&((statemachine->char_len[1] == 'i')))
				||(((statemachine->char_len[0] == 'S'))&&((statemachine->char_len[1] == 'I')))){


			Struct_RTC_W.format = (Struct_RTC_W.format == 1)?(FALSE):(TRUE);
			RTC_changeFormat(&Struct_RTC_W);

			timeout_Enable();
		    RTC_readHour(&Struct_RTC_R);
		    timeout_Disable();

			UART_putString(uartChannel, "\r\nSe ha cambiado el formato, presione ENTER para continuar... \r\n");

		} else if((((statemachine->char_len[0] == 'N'))&&((statemachine->char_len[1] == 'o')))
				||(((statemachine->char_len[0] == 'n'))&&((statemachine->char_len[1] == 'o')))
				||(((statemachine->char_len[0] == 'N'))&&((statemachine->char_len[1] == 'O')))){

			UART_putString(uartChannel, "\r\n No se ha cambiado el formato, presione ENTER para continuar... \r\n");

		}

		}
	}
}
void TERM_LCD(UART_ChannelType uartChannel, Term_StateMachineType* statemachine){

	//What if [ESC]
	if((UART_MailBoxData(uartChannel) == 27)){

					//go back to Main menu
					statemachine->currentMenu = MenuDisp;
					statemachine->currentMenuParameter = Option_param;
					TermHandler_StateMachine.LCDBusy = FALSE;
					(*ftpr_Disp_Array[statemachine->currentMenu])(uartChannel, &Struct_Char_R);


	//what if [ENTER]
				} else if ((UART_MailBoxData(uartChannel) == 13)){

					LCDNokia_clear();

					while(statemachine->f.tail != statemachine->f.head){

						Eco_Display(FIFO_POP(&statemachine->f));

					}
					UART_putString(uartChannel,"\r\n... Presione ESC para salir\r\n");
					FIFO_restart(&statemachine->f);

	//what if we receive any other type of data
				} else {
					FIFO_PUSH(&statemachine->f,UART_MailBoxData(uartChannel));
				}

}

void TERM_communication(UART_ChannelType uartChannel, Term_StateMachineType* statemachine){

	//What if [ESC]
	if((UART_MailBoxData(uartChannel) == 27)){

					//go back to Main menu
					statemachine->currentMenu = MenuDisp;
					statemachine->currentMenuParameter = Option_param;
					(*ftpr_Disp_Array[statemachine->currentMenu])(uartChannel, &Struct_Char_R);

					//If we are terminal 1 and terminal 2 is on, make ir know that we leave
					if(statemachine->id == '1' && TermHandler_StateMachine.Term2Com == TRUE){
						UART_putString(UART_4, "\r\nLa terminal 1 se ha desconectado ... [ESC] \r\n");
						TermHandler_StateMachine.Term1Com = FALSE;

					//If we are terminal 2 and terminal 1 is on, make it know that we leave
					} else if(statemachine->id == '2' && TermHandler_StateMachine.Term1Com == TRUE){
						UART_putString(UART_0, "\r\nLa terminal 2 se ha desconectado ... [ESC] \r\n");
						TermHandler_StateMachine.Term2Com = FALSE;
					}


	//what if [ENTER]
				} else if ((UART_MailBoxData(uartChannel) == 13)){

					//if both terminals are on, write on them
					if(TermHandler_StateMachine.Term2Com == TRUE && TermHandler_StateMachine.Term1Com == TRUE){
						UART_putString(UART_0, "\r\nTerminal");
						UART_putChar(UART_0, statemachine->id);
						UART_putString(UART_0, ": ");

						UART_putString(UART_4, "\r\nTerminal ");
						UART_putChar(UART_4, statemachine->id);
						UART_putString(UART_4, ": ");

					} else {
						UART_putString(uartChannel, "\r\nTerminal ");
						UART_putChar(uartChannel, statemachine->id);
						UART_putString(uartChannel, ": ");

					}

					while(statemachine->f.tail != statemachine->f.head){

						//save data
						char data = FIFO_POP(&statemachine->f);


						//if both terminals are on, write on them
						if(TermHandler_StateMachine.Term2Com == TRUE && TermHandler_StateMachine.Term1Com == TRUE){
							UART_putChar(UART_0, data);
							UART_putChar(UART_4, data);

						//if not, just write in '''''local'''''
						} else {
							UART_putChar(uartChannel, data);
						}
					}
					UART_putString(uartChannel,"\r\n[ENTER]\r\n");
					FIFO_restart(&statemachine->f);

	//what if we receive any other type of data
				} else {
					FIFO_PUSH(&statemachine->f,UART_MailBoxData(uartChannel));
				}
}

void TERM_ReadDate(UART_ChannelType uartChannel, Term_StateMachineType* statemachine){
	UART_MailBoxData(uartChannel);
	statemachine->currentMenuParameter = Option_param;
	statemachine->currentMenu = MenuDisp;
	(*ftpr_Disp_Array[MenuDisp])(uartChannel, &Struct_Char_R);
	TermHandler_StateMachine.RTCBusy = FALSE;


}

void TERM_ReadHour(UART_ChannelType uartChannel, Term_StateMachineType* statemachine){
	UART_MailBoxData(uartChannel);
	statemachine->currentMenuParameter = Option_param;
	statemachine->currentMenu = MenuDisp;

	(*ftpr_Disp_Array[statemachine->currentMenu])(uartChannel, &Struct_Char_R);
	TermHandler_StateMachine.RTCBusy = FALSE;

}

void TERM_MenuDisp(UART_ChannelType uartChannel, Term_StateMachineType* statemachine){

	statemachine->data = (UART_MailBoxData(uartChannel)) - '0';
	if((statemachine->data >= 1) && (statemachine->data <= 9)
			&& (statemachine->currentMenuParameter == Option_param)){
		statemachine->currentMenu = statemachine->data;
	}

	if(((TermHandler_StateMachine.RTCBusy == TRUE)
			&& (statemachine->currentMenu >= SetHourDisp
					&& statemachine->currentMenu <= ReadDateDisp))
			||((TermHandler_StateMachine.MemBusy == TRUE)
					&& (statemachine->currentMenu >= ReadI2CDisp
							&& statemachine->currentMenu <= WriteI2CDisp))
							||((TermHandler_StateMachine.LCDBusy == TRUE)
									&& (statemachine->currentMenu == LCDDisp)) ){

		statemachine->currentMenu = MenuDisp;
		(*ftpr_Disp_Array[statemachine->currentMenu])(uartChannel, &Struct_Char_R);
		(*ftpr_Disp_Array[19])(uartChannel, &Struct_Char_R);

		return;
	}

	if(((statemachine->currentMenu >= SetHourDisp
						&& statemachine->currentMenu <= ReadDateDisp))){

		TermHandler_StateMachine.RTCBusy = TRUE;

	}

	if(((statemachine->currentMenu >= ReadI2CDisp
							&& statemachine->currentMenu <= WriteI2CDisp))){

			TermHandler_StateMachine.MemBusy = TRUE;
			}

	if((statemachine->currentMenu == LCDDisp)){

			TermHandler_StateMachine.LCDBusy = TRUE;
			}

	(*ftpr_Disp_Array[statemachine->currentMenu])(uartChannel, &Struct_Char_R);

	if(statemachine->currentMenu == CommunicationDisp){
	switch(statemachine->id){
				case '1':
					TermHandler_StateMachine.Term1Com = TRUE;
					if(TermHandler_StateMachine.Term2Com){
					UART_putString(UART_4, "\r\nLa terminal 1 se ha conectado ... \r\n");
					UART_putString(UART_0, "\r\nLa terminal 2 est� conectada ... \r\n");


					}
					break;

				case '2':
					TermHandler_StateMachine.Term2Com = TRUE;
					if(TermHandler_StateMachine.Term1Com){
					UART_putString(UART_4, "\r\nLa terminal 1 est� conectada ... \r\n");
					UART_putString(UART_0, "\r\nLa terminal 2 se ha conectado ... \r\n");
					}
					break;
				}
	}

}

uint8 TERM1_init(){
	GPIO_clockGating(GPIOB);
	GPIO_pinControlRegisterType pinControlRegister = GPIO_MUX3;
	GPIO_pinControlRegister(GPIOB, BIT16, &pinControlRegister);
	GPIO_pinControlRegister(GPIOB, BIT17, &pinControlRegister);

	UART_init(UART_0, SYSTEM_CLOCK, BD_9600, BRFA_19);
	UART0_interruptEnable(UART_0);
	NVIC_enableInterruptAndPriority(UART0_IRQ, PRIORITY_10);

	return TRUE;
}

uint8 TERM2_init(){
	GPIO_clockGating(GPIOC);
	GPIO_pinControlRegisterType pinControlRegister = GPIO_MUX3;
	GPIO_pinControlRegister(GPIOC, BIT14, &pinControlRegister);
	GPIO_pinControlRegister(GPIOC, BIT15, &pinControlRegister);

	UART_init(UART_4, SYSTEM_CLOCK, BD_9600, BRFA_19);
	UART0_interruptEnable(UART_4);
	NVIC_enableInterruptAndPriority(UART4_IRQ, PRIORITY_10);

	return TRUE;
}

void Cast_Memory_param(Term_StateMachineType* statemachine){
	statemachine->address = 0;
	for(uint8 i=0;i<4;i++){
	statemachine->address |= ((((statemachine->char_address[i] - '0')>= 0) &&
			((statemachine->char_address[i] - '0') <= 9))?
					((statemachine->char_address[i] - '0'))
					:((((statemachine->char_address[i] - 'A')>= 0xA) &&
							((statemachine->char_address[i] - 'A') <= 0xF))?
									((statemachine->char_address[i] - 'A')):(0))) << (3-i)*4;

	}

	statemachine->len = 0;
	for(uint8 i=0;i<2;i++){
		statemachine->len |= ((((statemachine->char_len[i] - '0')>= 0) &&
				((statemachine->char_len[i] - '0') <= 9))?((statemachine->char_len[i] - '0')):(0)) << ((1-i)*4);
	}


}

uint8 TERMHANDLER_init(){

	NVIC_enableInterruptAndPriority(PIT_CH0_IRQ, PRIORITY_11);

	NVIC_enableInterruptAndPriority(PIT_CH1_IRQ, PRIORITY_12);

	EnableInterrupts;


	PIT_clockGating();
	PIT_enable();
	PIT_delay(PIT_0,SYSTEM_CLOCK,2);
	PIT_timerInterruptEnable(PIT_0);
	PIT_timerEnable(PIT_0);

	TERM1_init();
	TERM2_init();

	timeout_Enable();
	RTC_write(0,0x80);
	timeout_Disable();

	(*ftpr_Disp_Array[Term1_StateMachine.currentMenu])(UART_0, &Struct_Char_W);
	(*ftpr_Disp_Array[Term2_StateMachine.currentMenu])(UART_4, &Struct_Char_W);
}


void timeout_Ocurred(){
	timeout_flag = TRUE;
	PIT_timerInterruptDisable(PIT_1);
	PIT_timerDisable(PIT_1);
}

void timeout_Enable(){
	PIT_delay(PIT_1,SYSTEM_CLOCK,0.5);
	PIT_timerInterruptEnable(PIT_1);
	PIT_timerEnable(PIT_1);
}

void timeout_Disable(){
	if(timeout_flag == TRUE){
		timeout_flag = FALSE;

		if(((Term1_StateMachine.currentMenu != CommunicationDisp)
				&&(Term1_StateMachine.currentMenu != LCDDisp))){
			Term1_StateMachine.currentMenu = MenuDisp;
			Term1_StateMachine.currentMenuParameter = Option_param;

			(*ftpr_Disp_Array[Term1_StateMachine.currentMenu])(UART_0, &Struct_Char_W);
			(*ftpr_Disp_Array[18])(UART_0, &Struct_Char_W);

		}

		if(((Term2_StateMachine.currentMenu != CommunicationDisp)
				&&(Term2_StateMachine.currentMenu != LCDDisp))){
			Term2_StateMachine.currentMenu = MenuDisp;
			Term2_StateMachine.currentMenuParameter = Option_param;

			(*ftpr_Disp_Array[Term2_StateMachine.currentMenu])(UART_4, &Struct_Char_W);
			(*ftpr_Disp_Array[18])(UART_4, &Struct_Char_W);
		}

		timeout_notification = TRUE;

	}

	if((Term1_StateMachine.currentMenu == MenuDisp) && (timeout_notification == FALSE)){
				(*ftpr_Disp_Array[Term1_StateMachine.currentMenu])(UART_0, &Struct_Char_W);

			}

			if((Term2_StateMachine.currentMenu == MenuDisp) && (timeout_notification == FALSE)){

				(*ftpr_Disp_Array[Term2_StateMachine.currentMenu])(UART_4, &Struct_Char_W);

				timeout_notification = TRUE;

			}

	PIT_timerDisable(PIT_1);
	timeout_flag = FALSE;
}

uint8 timeout_Flag(){
	return timeout_flag;
}


void Button_Hour(){
	if(((TermHandler_StateMachine.RTCBusy == FALSE)
			&& (TermHandler_StateMachine.LCDBusy == FALSE)) || (manual_mode == TRUE)){
		TermHandler_StateMachine.RTCBusy = TRUE;
		TermHandler_StateMachine.LCDBusy = TRUE;

		if(manual_mode == FALSE){
		manual_mode = TRUE;

		Button_Display(&Struct_Char_W);

		BTTN_mailBoxData();
		manual_counter_hour = 0;
		manual_counter_date = 0;
		return;
		} else{
			switch(BTTN_mailBoxData()){
			case BUTTON_0:
				if(manual_counter_hour >= 8){
					return;
				}
				if(((Struct_Char_W.Time_Char[manual_counter_hour]) + 1) <= '9'){
					Struct_Char_W.Time_Char[manual_counter_hour] += 1;
				}
				break;

			case BUTTON_1:
				if(manual_counter_hour >= 8){
					return;
				}
				if((Struct_Char_W.Time_Char[manual_counter_hour] - 1) >= '0'){
					Struct_Char_W.Time_Char[manual_counter_hour] -= 1;
				}
				break;

			case BUTTON_2:

				manual_counter_hour++;
				if((manual_counter_hour == 2) || (manual_counter_hour == 5)){
					manual_counter_hour++;
				}
				if(manual_counter_hour >= 8){
					manual_counter_hour = 8;
				}

				break;

			case BUTTON_3:
				if(manual_counter_date >= 10){
					manual_counter_date = 10;
				}
				if((Struct_Char_W.Date_Char[manual_counter_date] + 1) <= '9'){
					Struct_Char_W.Date_Char[manual_counter_date] += 1;
				}
				break;

			case BUTTON_4:
				if(manual_counter_date >= 10){
					manual_counter_date = 10;
				}
				if((Struct_Char_W.Date_Char[manual_counter_date] - 1) >= '0'){
					Struct_Char_W.Date_Char[manual_counter_date] -= 1;
				}
				break;

			case BUTTON_5:

				manual_counter_date++;
				if((manual_counter_date == 2) || (manual_counter_date == 5)){
					manual_counter_date++;
				}
				if(manual_counter_date >= 10){
					manual_counter_date = 10;
				}

				break;

			default:
				break;
			}

			Button_Display(&Struct_Char_W);

		}

		if((manual_counter_date == 10) && (manual_counter_hour == 8)){


			Hour_Check(&Struct_Char_W,&Struct_RTC_W);
			Date_Check(&Struct_Char_W,&Struct_RTC_W);
			timeout_Enable();
			RTC_writeHour(&Struct_RTC_W);
			timeout_Disable();

			timeout_Enable();
			RTC_writeDate(&Struct_RTC_W);
			timeout_Disable();

		TermHandler_StateMachine.RTCBusy = FALSE;
		TermHandler_StateMachine.LCDBusy = FALSE;
		manual_mode = FALSE;


		}

	}

}

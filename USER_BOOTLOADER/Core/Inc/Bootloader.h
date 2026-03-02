/*
 * Bootloader.h
 *
 *  Created on: Jul 27, 2025
 *      Author: kadir
 */

#ifndef INC_BOOTLOADER_H_
#define INC_BOOTLOADER_H_

#include "main.h"
#include "stdbool.h"


#define BOOTLOADER_HEADER				0X7F
#define BOOTLOADER_VERSION				0X10
#define APPLICATION_HEADER				0X7E

#define GET_HELP						0X00
#define GET_VERSION						0X01
#define GET_ID							0X02
#define READ_MEMORY						0X11
#define GO_TO_ADDRESS					0X21
#define WRITE_MEMORY					0X31
#define ERASE 							0X43
#define WRITE_PROTECT_UNPROTECT			0X63
#define READOUT_PROTECT_UNPROTECT		0X82
#define GET_CHECKSUM					0XA1

#define ACK								0X79
#define NACK 							0X1F
#define UNKNOWN							0X99

#define SRAM1_END						0x2001BFFF
#define SRAM2_END						0x2001FFFF
#define BKPSRAM_END						0x40024FFF


extern bool canJumpToApplication;
extern uint32_t jumpAddress;

void handleEraseSector(void);
HAL_StatusTypeDef flashWrite(uint32_t address, uint8_t* data, uint32_t dataLength);
void handleWriteMemory(void);
void handleGoToAddress(void);
void processBootloaderCommand(void);
void handleGetVersion(void);
void handleGetHelp(void);
void handleGetID(void);
void handleReadMemory();
uint8_t verifyAddress(uint32_t address);

#endif /* INC_BOOTLOADER_H_ */

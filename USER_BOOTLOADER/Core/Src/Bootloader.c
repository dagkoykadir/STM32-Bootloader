#include "Bootloader.h"

extern char rxBuffer[BUFFER_SIZE];
extern uint8_t bufferIndex;


bool canJumpToApplication = false;
uint32_t jumpAddress;


void processBootloaderCommand(void)
{
	uint8_t command = rxBuffer[2];

	switch (command) {
	case GET_VERSION:
		handleGetVersion();
		break;
	case GET_HELP:
		handleGetHelp();
		break;
	case GET_ID:
		handleGetID();
		break;
	case READ_MEMORY:
		handleReadMemory();
		break;
	case GO_TO_ADDRESS:
		handleGoToAddress();
		break;
	case WRITE_MEMORY:
		handleWriteMemory();
		break;
	case ERASE:
		handleEraseSector();
		break;
	case WRITE_PROTECT_UNPROTECT:
		handleWriteProtection();
		break;
	default:
		break;
	}
	bufferIndex = 0;
	memset(rxBuffer, 0, BUFFER_SIZE);
}

void handleWriteProtection()
{

}

void handleEraseSector(void)
{
	uint8_t response[1] = { 0 };
	uint8_t offset = 3;

	/*
	 * örnek : 1 - 5 - 9 seçildi
	 *
	 * N değeri 3-1 = 2 rxbuffer[3]
	 *
	 *  gelecek veriler = 2,1-5-9,receivedCheckSum
	 */

	uint8_t N = rxBuffer[offset];
	uint8_t sectorCount = N + 1;
	uint8_t receivedCheckSum = N;
	uint32_t calculatedCheckSum = N;

	if(N == 0xFF)
	{
		receivedCheckSum = rxBuffer[offset + 1];
		calculatedCheckSum = 0xFF ^ 0x00;

		if(receivedCheckSum != calculatedCheckSum)
		{
			response[0] = NACK;
			HAL_UART_Transmit(UART_PORT, response, 1, HAL_MAX_DELAY);
			return;
		}

		HAL_FLASH_Unlock();

		FLASH_EraseInitTypeDef eraseInit;

		uint32_t sectorError;
		eraseInit.TypeErase    = FLASH_TYPEERASE_SECTORS;
		eraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;
		eraseInit.Sector       = FLASH_SECTOR_1;
		eraseInit.NbSectors    = 11;

		if(HAL_FLASHEx_Erase(&eraseInit, &sectorError))
		{
			response[0] = ACK;
			HAL_UART_Transmit(UART_PORT, response, 1, HAL_MAX_DELAY);

		}
		else{
			response[0] = NACK;
			HAL_UART_Transmit(UART_PORT, response, 1, HAL_MAX_DELAY);
		}
		HAL_FLASH_Lock();
		return;
	}

	for(uint8_t i = 1 ; i <= sectorCount ; i++)
	{
		calculatedCheckSum ^= rxBuffer[i + offset];
	}
	receivedCheckSum = rxBuffer[N + 2 + offset];

	if (receivedCheckSum != calculatedCheckSum) {
		response[0] = NACK;
		HAL_UART_Transmit(UART_PORT, response, sizeof(response), HAL_MAX_DELAY);
		return;
	}

	if(sectorCount == 0 || sectorCount > 12)
	{
		response[0] = NACK ; HAL_UART_Transmit(UART_PORT, response, 1, HAL_MAX_DELAY);
		return;
	}

	uint8_t sectors[12] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};

	for(uint8_t i = 0 ; i < sectorCount ; i++)
	{
		sectors[i] = rxBuffer[4 + i];
	}

    for (uint8_t i = 0; i < sectorCount; i++) {

    	HAL_FLASH_Unlock();
        FLASH_EraseInitTypeDef eraseInit = {0};
        uint32_t sectorError = 0xFFFFFFFF;

        eraseInit.TypeErase    = FLASH_TYPEERASE_SECTORS;
        eraseInit.VoltageRange = FLASH_VOLTAGE_RANGE_3;
        eraseInit.Sector       = sectors[i];
        eraseInit.NbSectors    = 1;

        if (HAL_FLASHEx_Erase(&eraseInit, &sectorError) != HAL_OK) {

            response[0] = NACK; HAL_UART_Transmit(UART_PORT, response, 1, HAL_MAX_DELAY);
            return;
        }
        HAL_FLASH_Lock();
    }
}

void handleWriteMemory(void)
{
	uint8_t response[1] = { 0 };
	uint8_t offset = 3;
	uint32_t address = (rxBuffer[offset] << 24)| (rxBuffer[offset + 1] << 16)
					   |(rxBuffer[offset + 2] << 8) | (rxBuffer[offset + 3]);

	uint32_t addressCheckSum = rxBuffer[offset + 4];
	uint16_t calculatedCheckSum = (rxBuffer[offset] ^ rxBuffer[offset + 1] ^ rxBuffer[offset + 2] ^ rxBuffer[offset + 3]);



	if (addressCheckSum != calculatedCheckSum) {
		response[0] = NACK;
		HAL_UART_Transmit(UART_PORT, response, sizeof(response), HAL_MAX_DELAY);
		return;
	}

	uint8_t addressIsValid = verifyAddress(address);
	if (!addressIsValid) {
		response[0] = NACK;
		HAL_UART_Transmit(UART_PORT, response, sizeof(response), HAL_MAX_DELAY);
		return;
	}

	uint32_t totalLength = (rxBuffer[offset + 5] << 24)| (rxBuffer[offset + 6] << 16)
						  | (rxBuffer[offset + 7] << 8) | (rxBuffer[offset + 8]);

	bufferIndex = 0;
	memset(rxBuffer, 0, BUFFER_SIZE);

	response[0] = ACK;
	HAL_UART_Transmit(UART_PORT, response, sizeof(response), HAL_MAX_DELAY);

	uint32_t offsetData = 0;

	while (offsetData < totalLength) {
			uint8_t N;  // (Number of Packet Size - CheckSum)

			if (HAL_UART_Receive(UART_PORT, &N, 1, HAL_MAX_DELAY) != HAL_OK) {
				response[0] = NACK;
				HAL_UART_Transmit(UART_PORT, response, 1, HAL_MAX_DELAY);
				return;
			}
			uint32_t dataLength = N + 1;  // max 256 olabilir



			uint8_t packet[256 + 1]; // data + checksum
			if (HAL_UART_Receive(UART_PORT, packet, dataLength + 1, HAL_MAX_DELAY)
					!= HAL_OK) {
				response[0] = NACK;
				HAL_UART_Transmit(UART_PORT, response, 1, HAL_MAX_DELAY);
				return;
			}

			uint8_t calculatedChecksum = N;

			// 1 >> 256 ya kadar hesaplıyor form kısmında
			// burada ise 0 >>
			for (uint32_t i = 0; i < dataLength; i++) {
				calculatedChecksum ^= packet[i];
			}

			uint8_t receivedChecksum = packet[dataLength];
			if (receivedChecksum != calculatedChecksum) {
				response[0] = NACK;
				HAL_UART_Transmit(UART_PORT, response, 1, HAL_MAX_DELAY);
				return;
			}

			if (flashWrite(address, packet, dataLength) == HAL_OK) {
				address += dataLength;
				offsetData += dataLength;
				memset(packet,0 ,257);
				if (offsetData == dataLength) {
					response[0] = ACK;
					HAL_UART_Transmit(UART_PORT, response, 1, HAL_MAX_DELAY);
				}
			} else {
				response[0] = NACK;
				HAL_UART_Transmit(UART_PORT, response, 1, HAL_MAX_DELAY);
				return;
			}
		}
}

HAL_StatusTypeDef flashWrite(uint32_t address, uint8_t* data, uint32_t dataLength)
{
	HAL_FLASH_Unlock();
	for(int i = 0; i < dataLength; i++)
	{
		if(HAL_FLASH_Program(FLASH_TYPEPROGRAM_BYTE, address + i, data[i]) != HAL_OK){
			HAL_FLASH_Lock();
			return HAL_ERROR;
		}

	}
	HAL_FLASH_Lock();
	return HAL_OK;
}

void handleGoToAddress(void)
{
	uint8_t response[1] = { 0 };
	uint8_t offset = 3;
	jumpAddress = (rxBuffer[offset] << 24 | rxBuffer[offset + 1] << 16 | rxBuffer[offset + 2] << 8 | rxBuffer[offset + 3]);

	uint32_t mspValue = *(volatile uint32_t *)jumpAddress;

	uint16_t addressChecksum = rxBuffer[offset + 4];
	uint16_t calculatedCheckSum = (rxBuffer[offset] ^ rxBuffer[offset + 1] ^ rxBuffer[offset + 2] ^ rxBuffer[offset + 3]);

	if (addressChecksum != calculatedCheckSum)
	{
		response[0] = NACK;
		HAL_UART_Transmit(UART_PORT, response, sizeof(response), HAL_MAX_DELAY);
		return;
	}

	uint8_t addressIsInvalid = verifyAddress(jumpAddress);

	if (!addressIsInvalid)
	{
		response[0] = NACK;
		HAL_UART_Transmit(UART_PORT, response, sizeof(response), HAL_MAX_DELAY);
		return;
	}

	if( mspValue != 0xFFFFFFFF)
	{
		__disable_irq();
		canJumpToApplication = true;
	}

}

void handleGetVersion(void)
{
	uint8_t response[2] = { 0 };
	if (BOOTLOADER_VERSION > 0  && BOOTLOADER_VERSION <= 255) {
		response[0] = ACK;
		response[1] = BOOTLOADER_VERSION;
	} else {
		response[0] = NACK;
		response[1] = UNKNOWN;
	}
	HAL_UART_Transmit(UART_PORT, response, sizeof(response), HAL_MAX_DELAY);
}

void handleGetHelp(void)
{
	uint8_t commands[] = {
			GET_HELP,
			GET_VERSION,
			GET_ID	,
			READ_MEMORY,
			GO_TO_ADDRESS,
			WRITE_MEMORY,
			ERASE,
			WRITE_PROTECT_UNPROTECT,
			READOUT_PROTECT_UNPROTECT,
			GET_CHECKSUM
	};

	uint8_t totalCommands = sizeof(commands)/sizeof(commands[0]);

	uint8_t response[1 + 1 + 1 + sizeof(commands)/sizeof(commands[0])] = { 0 };

	response[0] = ACK;
	response[1] = totalCommands;
	response[2] = BOOTLOADER_VERSION;

	memcpy(&response[3], commands, totalCommands);

	HAL_UART_Transmit(UART_PORT, response, sizeof(response), HAL_MAX_DELAY);
}

void handleGetID(void)
{
	uint8_t response [4] = { 0 };

	uint32_t IDCode = DBGMCU-> IDCODE;

	uint8_t PIDLSB = IDCode & 0XFF;

	response[0] = ACK;
	response[1] = 0x01;
	response[2] = 0x04;
	response[3] = PIDLSB;

	HAL_UART_Transmit(UART_PORT, response, sizeof(response), HAL_MAX_DELAY);
}

void handleReadMemory()
{
	uint8_t response[1] = { 0 };
	uint8_t buffer[256];
	uint8_t offset = 3;
	uint32_t address 			= (rxBuffer[offset] << 24| rxBuffer[offset + 1] << 16 |rxBuffer[offset + 2] << 8 | rxBuffer[offset + 3]);
	uint16_t addressChecksum 	= rxBuffer[offset + 4];
	//uint16_t length   		 	= rxBuffer[offset + 5];
	uint16_t calculatedCheckSum = (rxBuffer[offset] ^ rxBuffer[offset + 1]  ^ rxBuffer[offset + 2]  ^ rxBuffer[offset + 3]);

	if(addressChecksum != calculatedCheckSum)
	{
		response[0] = NACK;
		HAL_UART_Transmit(UART_PORT, response, sizeof(response), HAL_MAX_DELAY);
		return;
	}

	uint8_t N = rxBuffer[8];
	uint8_t NComplemt = rxBuffer[9];

	if((uint8_t)(N ^ NComplemt) != 0xFF)
	{
		response[0] = NACK;
		HAL_UART_Transmit(UART_PORT, response, sizeof(response), HAL_MAX_DELAY);
		return;
	}

	uint8_t addressIsInvalid = verifyAddress(address);

	if(!addressIsInvalid)
	{
		response[0] = NACK;
		HAL_UART_Transmit(UART_PORT, response, sizeof(response), HAL_MAX_DELAY);
		return;
	}

	response[0] = ACK;
	HAL_UART_Transmit(UART_PORT, response, sizeof(response), HAL_MAX_DELAY);

	memcpy(buffer, (uint8_t*)address ,N);
	HAL_UART_Transmit(UART_PORT, buffer, N, HAL_MAX_DELAY);
}

uint8_t verifyAddress(uint32_t address)
{
	if((address >= FLASH_BASE   && address <= FLASH_END) 	 ||
			(address >= SRAM1_BASE   && address <= SRAM1_END) 	 ||
			(address >= SRAM2_BASE   && address <= SRAM2_END) 	 ||
			(address >= BKPSRAM_BASE && address <= BKPSRAM_END))
	{
		return 1;
	}
	return 0;
}




















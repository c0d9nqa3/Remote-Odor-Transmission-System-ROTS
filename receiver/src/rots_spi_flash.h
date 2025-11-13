/**
 * @file rots_spi_flash.h
 * @brief ROTS SPI Flash Driver Header
 * @author ROTS Team
 * @date 2024
 * 
 * Header file for SPI Flash storage (16MB W25Q128)
 */

#ifndef ROTS_SPI_FLASH_H
#define ROTS_SPI_FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes */
#include "rots_receiver.h"

/* SPI Flash Commands */
#define SPI_FLASH_CMD_WRITE_ENABLE      0x06
#define SPI_FLASH_CMD_WRITE_DISABLE     0x04
#define SPI_FLASH_CMD_READ_STATUS_REG   0x05
#define SPI_FLASH_CMD_WRITE_STATUS_REG  0x01
#define SPI_FLASH_CMD_READ_DATA         0x03
#define SPI_FLASH_CMD_PAGE_PROGRAM      0x02
#define SPI_FLASH_CMD_SECTOR_ERASE      0x20
#define SPI_FLASH_CMD_BLOCK_ERASE       0xD8
#define SPI_FLASH_CMD_CHIP_ERASE        0xC7
#define SPI_FLASH_CMD_POWER_DOWN        0xB9
#define SPI_FLASH_CMD_RELEASE_POWER_DOWN 0xAB
#define SPI_FLASH_CMD_DEVICE_ID         0x90
#define SPI_FLASH_CMD_JEDEC_ID          0x9F
#define SPI_FLASH_CMD_READ_UNIQUE_ID    0x4B

/* SPI Flash Status Register Bits */
#define SPI_FLASH_STATUS_BUSY           0x01
#define SPI_FLASH_STATUS_WEL            0x02
#define SPI_FLASH_STATUS_BP0            0x04
#define SPI_FLASH_STATUS_BP1            0x08
#define SPI_FLASH_STATUS_BP2            0x10
#define SPI_FLASH_STATUS_TB             0x20
#define SPI_FLASH_STATUS_SEC            0x40
#define SPI_FLASH_STATUS_SRP            0x80

/* SPI Flash Constants */
#define SPI_FLASH_PAGE_SIZE             256
#define SPI_FLASH_SECTOR_SIZE           4096
#define SPI_FLASH_BLOCK_SIZE            65536
#define SPI_FLASH_TOTAL_SIZE            (16 * 1024 * 1024)  // 16MB

/* Recipe Storage Constants */
#define RECIPE_STORAGE_MAGIC            0x524F5453  // "ROTS"
#define RECIPE_STORAGE_VERSION          1
#define RECIPE_STORAGE_BASE_ADDR        0x00000000

/* Function Prototypes */
ROTS_StatusTypeDef ROTS_SPI_Flash_Init(void);
ROTS_StatusTypeDef ROTS_SPI_Flash_Read(uint32_t address, uint8_t* data, uint32_t length);
ROTS_StatusTypeDef ROTS_SPI_Flash_Write(uint32_t address, const uint8_t* data, uint32_t length);
ROTS_StatusTypeDef ROTS_SPI_Flash_EraseSector(uint32_t address);
ROTS_StatusTypeDef ROTS_SPI_Flash_EraseBlock(uint32_t address);
ROTS_StatusTypeDef ROTS_SPI_Flash_ChipErase(void);
ROTS_StatusTypeDef ROTS_SPI_Flash_GetDeviceID(uint16_t* device_id);
ROTS_StatusTypeDef ROTS_SPI_Flash_GetJEDECID(uint32_t* jedec_id);
ROTS_StatusTypeDef ROTS_SPI_Flash_WaitReady(void);

#ifdef __cplusplus
}
#endif

#endif /* ROTS_SPI_FLASH_H */

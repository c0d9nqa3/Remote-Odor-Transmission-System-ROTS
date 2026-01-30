/**
 * @file rots_spi_flash.c
 * @brief ROTS SPI Flash Driver
 * @author ROTS Team
 * @date 2024
 * 
 * SPI Flash driver for W25Q128 (16MB) storage
 */

#include "rots_receiver.h"
#include "rots_spi_flash.h"
#include "rots_debug.h"
#include <string.h>

/* SPI Handle */
static SPI_HandleTypeDef hspi_flash;

/* GPIO CS Pin (PC5; PA4 is used for ESP8266 reset) */
#define SPI_FLASH_CS_PORT   GPIOC
#define SPI_FLASH_CS_PIN    GPIO_PIN_5

/* Private function prototypes */
static void ROTS_SPI_Flash_CS_Select(void);
static void ROTS_SPI_Flash_CS_Deselect(void);
static ROTS_StatusTypeDef ROTS_SPI_Flash_SendCommand(uint8_t command);
static ROTS_StatusTypeDef ROTS_SPI_Flash_SendAddress(uint32_t address);
static uint8_t ROTS_SPI_Flash_ReadByte(void);
static void ROTS_SPI_Flash_WriteByte(uint8_t data);
static ROTS_StatusTypeDef ROTS_SPI_Flash_WriteEnable(void);
static ROTS_StatusTypeDef ROTS_SPI_Flash_ReadStatusRegister(uint8_t* status);

/**
 * @brief Initialize SPI Flash
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_SPI_Flash_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    SPI_InitTypeDef SPI_InitStruct = {0};
    
    /* Enable clocks */
    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    
    /* Configure SPI pins */
    GPIO_InitStruct.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
    
    /* Configure CS pin (PC5) */
    GPIO_InitStruct.Pin = SPI_FLASH_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(SPI_FLASH_CS_PORT, &GPIO_InitStruct);
    
    /* Deselect CS */
    ROTS_SPI_Flash_CS_Deselect();
    
    /* Configure SPI */
    hspi_flash.Instance = SPI1;
    hspi_flash.Init.Mode = SPI_MODE_MASTER;
    hspi_flash.Init.Direction = SPI_DIRECTION_2LINES;
    hspi_flash.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi_flash.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi_flash.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi_flash.Init.NSS = SPI_NSS_SOFT;
    hspi_flash.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
    hspi_flash.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi_flash.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi_flash.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi_flash.Init.CRCPolynomial = 10;
    
    if (HAL_SPI_Init(&hspi_flash) != HAL_OK) {
        DEBUG_ERROR("SPI Flash initialization failed\r\n");
        return ROTS_ERROR;
    }
    
    /* Release power down */
    ROTS_SPI_Flash_SendCommand(SPI_FLASH_CMD_RELEASE_POWER_DOWN);
    HAL_Delay(10);
    
    /* Read device ID to verify connection */
    uint16_t device_id = 0;
    if (ROTS_SPI_Flash_GetDeviceID(&device_id) == ROTS_OK) {
        DEBUG_INFO("SPI Flash initialized, Device ID: 0x%04X\r\n", device_id);
    } else {
        DEBUG_WARNING("SPI Flash Device ID read failed\r\n");
    }
    
    return ROTS_OK;
}

/**
 * @brief Select CS pin
 */
static void ROTS_SPI_Flash_CS_Select(void)
{
    HAL_GPIO_WritePin(SPI_FLASH_CS_PORT, SPI_FLASH_CS_PIN, GPIO_PIN_RESET);
}

/**
 * @brief Deselect CS pin
 */
static void ROTS_SPI_Flash_CS_Deselect(void)
{
    HAL_GPIO_WritePin(SPI_FLASH_CS_PORT, SPI_FLASH_CS_PIN, GPIO_PIN_SET);
}

/**
 * @brief Send command to SPI Flash
 * @param command Command byte
 * @return ROTS_OK if successful
 */
static ROTS_StatusTypeDef ROTS_SPI_Flash_SendCommand(uint8_t command)
{
    ROTS_SPI_Flash_CS_Select();
    ROTS_SPI_Flash_WriteByte(command);
    ROTS_SPI_Flash_CS_Deselect();
    return ROTS_OK;
}

/**
 * @brief Send address to SPI Flash
 * @param address Address (24-bit)
 * @return ROTS_OK if successful
 */
static ROTS_StatusTypeDef ROTS_SPI_Flash_SendAddress(uint32_t address)
{
    ROTS_SPI_Flash_WriteByte((address >> 16) & 0xFF);
    ROTS_SPI_Flash_WriteByte((address >> 8) & 0xFF);
    ROTS_SPI_Flash_WriteByte(address & 0xFF);
    return ROTS_OK;
}

/**
 * @brief Read byte from SPI Flash
 * @return Read byte
 */
static uint8_t ROTS_SPI_Flash_ReadByte(void)
{
    uint8_t data = 0;
    HAL_SPI_Receive(&hspi_flash, &data, 1, HAL_MAX_DELAY);
    return data;
}

/**
 * @brief Write byte to SPI Flash
 * @param data Data byte
 */
static void ROTS_SPI_Flash_WriteByte(uint8_t data)
{
    HAL_SPI_Transmit(&hspi_flash, &data, 1, HAL_MAX_DELAY);
}

/**
 * @brief Enable write operations
 * @return ROTS_OK if successful
 */
static ROTS_StatusTypeDef ROTS_SPI_Flash_WriteEnable(void)
{
    ROTS_SPI_Flash_SendCommand(SPI_FLASH_CMD_WRITE_ENABLE);
    HAL_Delay(1);
    return ROTS_OK;
}

/**
 * @brief Read status register
 * @param status Pointer to status byte
 * @return ROTS_OK if successful
 */
static ROTS_StatusTypeDef ROTS_SPI_Flash_ReadStatusRegister(uint8_t* status)
{
    if (!status) return ROTS_INVALID_PARAM;
    
    ROTS_SPI_Flash_CS_Select();
    ROTS_SPI_Flash_WriteByte(SPI_FLASH_CMD_READ_STATUS_REG);
    *status = ROTS_SPI_Flash_ReadByte();
    ROTS_SPI_Flash_CS_Deselect();
    
    return ROTS_OK;
}

/**
 * @brief Wait for flash to be ready
 * @return ROTS_OK if successful
 */
ROTS_StatusTypeDef ROTS_SPI_Flash_WaitReady(void)
{
    uint8_t status = 0;
    uint32_t timeout = HAL_GetTick() + 1000;  // 1 second timeout
    
    do {
        if (ROTS_SPI_Flash_ReadStatusRegister(&status) != ROTS_OK) {
            return ROTS_ERROR;
        }
        if (HAL_GetTick() > timeout) {
            return ROTS_TIMEOUT;
        }
    } while (status & SPI_FLASH_STATUS_BUSY);
    
    return ROTS_OK;
}

/**
 * @brief Read data from SPI Flash
 * @param address Start address
 * @param data Buffer to store data
 * @param length Number of bytes to read
 * @return ROTS_OK if successful
 */
ROTS_StatusTypeDef ROTS_SPI_Flash_Read(uint32_t address, uint8_t* data, uint32_t length)
{
    if (!data || length == 0) {
        return ROTS_INVALID_PARAM;
    }
    
    if (address + length > SPI_FLASH_TOTAL_SIZE) {
        return ROTS_INVALID_PARAM;
    }
    
    /* Wait for flash to be ready */
    if (ROTS_SPI_Flash_WaitReady() != ROTS_OK) {
        return ROTS_TIMEOUT;
    }
    
    /* Select CS */
    ROTS_SPI_Flash_CS_Select();
    
    /* Send read command */
    ROTS_SPI_Flash_WriteByte(SPI_FLASH_CMD_READ_DATA);
    
    /* Send address */
    ROTS_SPI_Flash_SendAddress(address);
    
    /* Read data */
    HAL_SPI_Receive(&hspi_flash, data, length, HAL_MAX_DELAY);
    
    /* Deselect CS */
    ROTS_SPI_Flash_CS_Deselect();
    
    return ROTS_OK;
}

/**
 * @brief Write data to SPI Flash (page program)
 * @param address Start address
 * @param data Data to write
 * @param length Number of bytes to write
 * @return ROTS_OK if successful
 */
ROTS_StatusTypeDef ROTS_SPI_Flash_Write(uint32_t address, const uint8_t* data, uint32_t length)
{
    if (!data || length == 0) {
        return ROTS_INVALID_PARAM;
    }
    
    if (address + length > SPI_FLASH_TOTAL_SIZE) {
        return ROTS_INVALID_PARAM;
    }
    
    uint32_t offset = 0;
    uint32_t bytes_to_write = 0;
    
    while (offset < length) {
        /* Wait for flash to be ready */
        if (ROTS_SPI_Flash_WaitReady() != ROTS_OK) {
            return ROTS_TIMEOUT;
        }
        
        /* Enable write */
        ROTS_SPI_Flash_WriteEnable();
        
        /* Calculate bytes to write in this page */
        uint32_t page_start = (address + offset) & ~(SPI_FLASH_PAGE_SIZE - 1);
        uint32_t page_offset = (address + offset) & (SPI_FLASH_PAGE_SIZE - 1);
        bytes_to_write = SPI_FLASH_PAGE_SIZE - page_offset;
        if (bytes_to_write > (length - offset)) {
            bytes_to_write = length - offset;
        }
        
        /* Select CS */
        ROTS_SPI_Flash_CS_Select();
        
        /* Send page program command */
        ROTS_SPI_Flash_WriteByte(SPI_FLASH_CMD_PAGE_PROGRAM);
        
        /* Send address */
        ROTS_SPI_Flash_SendAddress(address + offset);
        
        /* Write data */
        HAL_SPI_Transmit(&hspi_flash, (uint8_t*)data + offset, bytes_to_write, HAL_MAX_DELAY);
        
        /* Deselect CS */
        ROTS_SPI_Flash_CS_Deselect();
        
        offset += bytes_to_write;
    }
    
    /* Wait for write to complete */
    return ROTS_SPI_Flash_WaitReady();
}

/**
 * @brief Erase sector (4KB)
 * @param address Address in sector
 * @return ROTS_OK if successful
 */
ROTS_StatusTypeDef ROTS_SPI_Flash_EraseSector(uint32_t address)
{
    if (address >= SPI_FLASH_TOTAL_SIZE) {
        return ROTS_INVALID_PARAM;
    }
    
    /* Wait for flash to be ready */
    if (ROTS_SPI_Flash_WaitReady() != ROTS_OK) {
        return ROTS_TIMEOUT;
    }
    
    /* Enable write */
    ROTS_SPI_Flash_WriteEnable();
    
    /* Select CS */
    ROTS_SPI_Flash_CS_Select();
    
    /* Send sector erase command */
    ROTS_SPI_Flash_WriteByte(SPI_FLASH_CMD_SECTOR_ERASE);
    
    /* Send address */
    ROTS_SPI_Flash_SendAddress(address);
    
    /* Deselect CS */
    ROTS_SPI_Flash_CS_Deselect();
    
    /* Wait for erase to complete */
    return ROTS_SPI_Flash_WaitReady();
}

/**
 * @brief Erase block (64KB)
 * @param address Address in block
 * @return ROTS_OK if successful
 */
ROTS_StatusTypeDef ROTS_SPI_Flash_EraseBlock(uint32_t address)
{
    if (address >= SPI_FLASH_TOTAL_SIZE) {
        return ROTS_INVALID_PARAM;
    }
    
    /* Wait for flash to be ready */
    if (ROTS_SPI_Flash_WaitReady() != ROTS_OK) {
        return ROTS_TIMEOUT;
    }
    
    /* Enable write */
    ROTS_SPI_Flash_WriteEnable();
    
    /* Select CS */
    ROTS_SPI_Flash_CS_Select();
    
    /* Send block erase command */
    ROTS_SPI_Flash_WriteByte(SPI_FLASH_CMD_BLOCK_ERASE);
    
    /* Send address */
    ROTS_SPI_Flash_SendAddress(address);
    
    /* Deselect CS */
    ROTS_SPI_Flash_CS_Deselect();
    
    /* Wait for erase to complete */
    return ROTS_SPI_Flash_WaitReady();
}

/**
 * @brief Erase entire chip
 * @return ROTS_OK if successful
 */
ROTS_StatusTypeDef ROTS_SPI_Flash_ChipErase(void)
{
    /* Wait for flash to be ready */
    if (ROTS_SPI_Flash_WaitReady() != ROTS_OK) {
        return ROTS_TIMEOUT;
    }
    
    /* Enable write */
    ROTS_SPI_Flash_WriteEnable();
    
    /* Select CS */
    ROTS_SPI_Flash_CS_Select();
    
    /* Send chip erase command */
    ROTS_SPI_Flash_WriteByte(SPI_FLASH_CMD_CHIP_ERASE);
    
    /* Deselect CS */
    ROTS_SPI_Flash_CS_Deselect();
    
    /* Wait for erase to complete (this takes a long time) */
    return ROTS_SPI_Flash_WaitReady();
}

/**
 * @brief Get device ID
 * @param device_id Pointer to device ID
 * @return ROTS_OK if successful
 */
ROTS_StatusTypeDef ROTS_SPI_Flash_GetDeviceID(uint16_t* device_id)
{
    if (!device_id) {
        return ROTS_INVALID_PARAM;
    }
    
    uint8_t id_bytes[2] = {0};
    
    ROTS_SPI_Flash_CS_Select();
    ROTS_SPI_Flash_WriteByte(SPI_FLASH_CMD_DEVICE_ID);
    ROTS_SPI_Flash_WriteByte(0x00);
    ROTS_SPI_Flash_WriteByte(0x00);
    ROTS_SPI_Flash_WriteByte(0x00);
    id_bytes[0] = ROTS_SPI_Flash_ReadByte();
    id_bytes[1] = ROTS_SPI_Flash_ReadByte();
    ROTS_SPI_Flash_CS_Deselect();
    
    *device_id = (id_bytes[0] << 8) | id_bytes[1];
    
    return ROTS_OK;
}

/**
 * @brief Get JEDEC ID
 * @param jedec_id Pointer to JEDEC ID
 * @return ROTS_OK if successful
 */
ROTS_StatusTypeDef ROTS_SPI_Flash_GetJEDECID(uint32_t* jedec_id)
{
    if (!jedec_id) {
        return ROTS_INVALID_PARAM;
    }
    
    uint8_t id_bytes[3] = {0};
    
    ROTS_SPI_Flash_CS_Select();
    ROTS_SPI_Flash_WriteByte(SPI_FLASH_CMD_JEDEC_ID);
    id_bytes[0] = ROTS_SPI_Flash_ReadByte();
    id_bytes[1] = ROTS_SPI_Flash_ReadByte();
    id_bytes[2] = ROTS_SPI_Flash_ReadByte();
    ROTS_SPI_Flash_CS_Deselect();
    
    *jedec_id = (id_bytes[0] << 16) | (id_bytes[1] << 8) | id_bytes[2];
    
    return ROTS_OK;
}

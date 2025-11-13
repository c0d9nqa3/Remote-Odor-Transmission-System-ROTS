/**
 * @file rots_display.c
 * @brief ROTS Display Module - SSD1306 OLED Driver
 * @author ROTS Team
 * @date 2024
 * 
 * Complete SSD1306 OLED display driver with frame buffer
 */

#include "rots_receiver.h"
#include "rots_display.h"
#include "rots_system_monitor.h"
#include "rots_debug.h"
#include <stdio.h>
#include <string.h>

/* SSD1306 Constants */
#define SSD1306_I2C_ADDRESS             0x3C
#define SSD1306_WIDTH                   128
#define SSD1306_HEIGHT                  64
#define SSD1306_PAGES                   8
#define SSD1306_FRAMEBUFFER_SIZE        (SSD1306_WIDTH * SSD1306_PAGES)

/* SSD1306 Commands */
#define SSD1306_CMD_SET_CONTRAST        0x81
#define SSD1306_CMD_DISPLAY_ALL_ON_RESUME 0xA4
#define SSD1306_CMD_DISPLAY_ALL_ON      0xA5
#define SSD1306_CMD_NORMAL_DISPLAY      0xA6
#define SSD1306_CMD_INVERT_DISPLAY      0xA7
#define SSD1306_CMD_DISPLAY_OFF         0xAE
#define SSD1306_CMD_DISPLAY_ON          0xAF
#define SSD1306_CMD_SET_DISPLAY_OFFSET  0xD3
#define SSD1306_CMD_SET_COMPINS         0xDA
#define SSD1306_CMD_SET_VCOM_DETECT     0xDB
#define SSD1306_CMD_SET_DISPLAY_CLOCK_DIV 0xD5
#define SSD1306_CMD_SET_PRECHARGE       0xD9
#define SSD1306_CMD_SET_MULTIPLEX       0xA8
#define SSD1306_CMD_SET_LOW_COLUMN      0x00
#define SSD1306_CMD_SET_HIGH_COLUMN     0x10
#define SSD1306_CMD_SET_START_LINE      0x40
#define SSD1306_CMD_MEMORY_MODE         0x20
#define SSD1306_CMD_COLUMN_ADDR         0x21
#define SSD1306_CMD_PAGE_ADDR           0x22
#define SSD1306_CMD_COM_SCAN_INC        0xC0
#define SSD1306_CMD_COM_SCAN_DEC        0xC8
#define SSD1306_CMD_SEG_REMAP           0xA0
#define SSD1306_CMD_CHARGE_PUMP         0x8D
#define SSD1306_CMD_EXTERNAL_VCC        0x1
#define SSD1306_CMD_SWITCH_CAP_VCC      0x2
#define SSD1306_CMD_ACTIVATE_SCROLL     0x2F
#define SSD1306_CMD_DEACTIVATE_SCROLL   0x2E
#define SSD1306_CMD_SET_VERTICAL_SCROLL_AREA 0xA3

/* Private variables */
static I2C_HandleTypeDef hi2c_display;
static bool display_initialized = false;
static uint8_t frame_buffer[SSD1306_FRAMEBUFFER_SIZE];
static uint32_t last_update_time = 0;
static char display_line1[21];
static char display_line2[21];
static char display_line3[21];
static char display_line4[21];

/* Private function prototypes */
static ROTS_StatusTypeDef ROTS_Display_InitI2C(void);
static ROTS_StatusTypeDef ROTS_Display_InitSSD1306(void);
static ROTS_StatusTypeDef ROTS_Display_SendCommand(uint8_t command);
static ROTS_StatusTypeDef ROTS_Display_SendData(uint8_t* data, uint16_t length);
static void ROTS_Display_ClearBuffer(void);
static void ROTS_Display_DrawString(uint8_t x, uint8_t page, const char* str);
static void ROTS_Display_UpdateFrameBuffer(void);
static void ROTS_Display_FlushBuffer(void);
static void ROTS_Display_DrawChar(uint8_t x, uint8_t page, char c);

/* Font data (5x7) */
static const uint8_t font_5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00}, // Space
    {0x00, 0x00, 0x5F, 0x00, 0x00}, // !
    {0x00, 0x07, 0x00, 0x07, 0x00}, // "
    {0x14, 0x7F, 0x14, 0x7F, 0x14}, // #
    {0x24, 0x2A, 0x7F, 0x2A, 0x12}, // $
    {0x23, 0x13, 0x08, 0x64, 0x62}, // %
    {0x36, 0x49, 0x55, 0x22, 0x50}, // &
    {0x00, 0x05, 0x03, 0x00, 0x00}, // '
    {0x00, 0x1C, 0x22, 0x41, 0x00}, // (
    {0x00, 0x41, 0x22, 0x1C, 0x00}, // )
    {0x14, 0x08, 0x3E, 0x08, 0x14}, // *
    {0x08, 0x08, 0x3E, 0x08, 0x08}, // +
    {0x00, 0x00, 0xA0, 0x60, 0x00}, // ,
    {0x08, 0x08, 0x08, 0x08, 0x08}, // -
    {0x00, 0x60, 0x60, 0x00, 0x00}, // .
    {0x20, 0x10, 0x08, 0x04, 0x02}, // /
    {0x3E, 0x51, 0x49, 0x45, 0x3E}, // 0
    {0x00, 0x42, 0x7F, 0x40, 0x00}, // 1
    {0x42, 0x61, 0x51, 0x49, 0x46}, // 2
    {0x21, 0x41, 0x45, 0x4B, 0x31}, // 3
    {0x18, 0x14, 0x12, 0x7F, 0x10}, // 4
    {0x27, 0x45, 0x45, 0x45, 0x39}, // 5
    {0x3C, 0x4A, 0x49, 0x49, 0x30}, // 6
    {0x01, 0x71, 0x09, 0x05, 0x03}, // 7
    {0x36, 0x49, 0x49, 0x49, 0x36}, // 8
    {0x06, 0x49, 0x49, 0x29, 0x1E}, // 9
    {0x00, 0x36, 0x36, 0x00, 0x00}, // :
    {0x00, 0x56, 0x36, 0x00, 0x00}, // ;
    {0x08, 0x14, 0x22, 0x41, 0x00}, // <
    {0x14, 0x14, 0x14, 0x14, 0x14}, // =
    {0x00, 0x41, 0x22, 0x14, 0x08}, // >
    {0x02, 0x01, 0x51, 0x09, 0x06}, // ?
    {0x32, 0x49, 0x59, 0x51, 0x3E}, // @
    {0x7C, 0x12, 0x11, 0x12, 0x7C}, // A
    {0x7F, 0x49, 0x49, 0x49, 0x36}, // B
    {0x3E, 0x41, 0x41, 0x41, 0x22}, // C
    {0x7F, 0x41, 0x41, 0x22, 0x1C}, // D
    {0x7F, 0x49, 0x49, 0x49, 0x41}, // E
    {0x7F, 0x09, 0x09, 0x09, 0x01}, // F
    {0x3E, 0x41, 0x49, 0x49, 0x7A}, // G
    {0x7F, 0x08, 0x08, 0x08, 0x7F}, // H
    {0x00, 0x41, 0x7F, 0x41, 0x00}, // I
    {0x20, 0x40, 0x41, 0x3F, 0x01}, // J
    {0x7F, 0x08, 0x14, 0x22, 0x41}, // K
    {0x7F, 0x40, 0x40, 0x40, 0x40}, // L
    {0x7F, 0x02, 0x0C, 0x02, 0x7F}, // M
    {0x7F, 0x04, 0x08, 0x10, 0x7F}, // N
    {0x3E, 0x41, 0x41, 0x41, 0x3E}, // O
    {0x7F, 0x09, 0x09, 0x09, 0x06}, // P
    {0x3E, 0x41, 0x51, 0x21, 0x5E}, // Q
    {0x7F, 0x09, 0x19, 0x29, 0x46}, // R
    {0x46, 0x49, 0x49, 0x49, 0x31}, // S
    {0x01, 0x01, 0x7F, 0x01, 0x01}, // T
    {0x3F, 0x40, 0x40, 0x40, 0x3F}, // U
    {0x1F, 0x20, 0x40, 0x20, 0x1F}, // V
    {0x3F, 0x40, 0x38, 0x40, 0x3F}, // W
    {0x63, 0x14, 0x08, 0x14, 0x63}, // X
    {0x07, 0x08, 0x70, 0x08, 0x07}, // Y
    {0x61, 0x51, 0x49, 0x45, 0x43}, // Z
};

/**
 * @brief Initialize display module
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_Display_Init(void)
{
    ROTS_StatusTypeDef status = ROTS_OK;
    
    /* Initialize I2C for display */
    status = ROTS_Display_InitI2C();
    if (status != ROTS_OK) {
        DEBUG_ERROR("Display I2C init failed\r\n");
        return status;
    }
    
    /* Initialize SSD1306 */
    status = ROTS_Display_InitSSD1306();
    if (status != ROTS_OK) {
        DEBUG_ERROR("SSD1306 init failed\r\n");
        return status;
    }
    
    /* Clear frame buffer */
    ROTS_Display_ClearBuffer();
    
    /* Show startup message */
    strcpy(display_line1, "ROTS Receiver");
    strcpy(display_line2, "Initializing...");
    ROTS_Display_UpdateFrameBuffer();
    ROTS_Display_FlushBuffer();
    
    display_initialized = true;
    DEBUG_INFO("Display initialized\r\n");
    
    return ROTS_OK;
}

/**
 * @brief Show message on display
 * @param line1 First line text
 * @param line2 Second line text
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_Display_ShowMessage(const char* line1, const char* line2)
{
    if (!display_initialized) {
        return ROTS_DISPLAY_ERROR;
    }
    
    if (line1) {
        strncpy(display_line1, line1, 20);
        display_line1[20] = '\0';
    }
    if (line2) {
        strncpy(display_line2, line2, 20);
        display_line2[20] = '\0';
    }
    
    ROTS_Display_UpdateFrameBuffer();
    ROTS_Display_FlushBuffer();
    
    return ROTS_OK;
}

/**
 * @brief Show error message on display
 * @param error_code Error code
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_Display_ShowError(ROTS_StatusTypeDef error_code)
{
    if (!display_initialized) {
        return ROTS_DISPLAY_ERROR;
    }
    
    strcpy(display_line1, "ERROR");
    snprintf(display_line2, 21, "Code: %d", error_code);
    display_line2[20] = '\0';
    display_line3[0] = '\0';
    display_line4[0] = '\0';
    
    ROTS_Display_UpdateFrameBuffer();
    ROTS_Display_FlushBuffer();
    
    return ROTS_OK;
}

/**
 * @brief Update display with system status
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_Display_Update(void)
{
    if (!display_initialized) {
        return ROTS_DISPLAY_ERROR;
    }
    
    /* Update every 500ms */
    if ((HAL_GetTick() - last_update_time) < 500) {
        return ROTS_OK;
    }
    
    last_update_time = HAL_GetTick();
    
    /* Get system status */
    ROTS_SystemStatus_t system_status;
    if (ROTS_SystemMonitor_GetStatus(&system_status) == ROTS_OK) {
        /* Format status lines */
        strcpy(display_line1, "ROTS Receiver");
        
        uint32_t uptime = system_status.uptime;
        snprintf(display_line2, 21, "Uptime: %lu s", uptime);
        display_line2[20] = '\0';
        
        snprintf(display_line3, 21, "State: %d", system_status.state);
        display_line3[20] = '\0';
        
        snprintf(display_line4, 21, "Errors: %d", system_status.error_count);
        display_line4[20] = '\0';
    } else {
        strcpy(display_line1, "ROTS Receiver");
        strcpy(display_line2, "Status: OK");
        display_line3[0] = '\0';
        display_line4[0] = '\0';
    }
    
    /* Update and flush frame buffer */
    ROTS_Display_UpdateFrameBuffer();
    ROTS_Display_FlushBuffer();
    
    return ROTS_OK;
}

/**
 * @brief Initialize I2C for display
 * @return ROTS_OK if successful, error code otherwise
 */
static ROTS_StatusTypeDef ROTS_Display_InitI2C(void)
{
    /* I2C is already initialized in hardware init */
    /* Just get the handle from hardware module */
    /* Note: In real implementation, we should use shared I2C handle */
    hi2c_display.Instance = I2C1;
    hi2c_display.Init.ClockSpeed = 400000;
    hi2c_display.Init.DutyCycle = I2C_DUTYCYCLE_2;
    hi2c_display.Init.OwnAddress1 = 0;
    hi2c_display.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
    hi2c_display.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
    hi2c_display.Init.OwnAddress2 = 0;
    hi2c_display.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
    hi2c_display.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    
    if (HAL_I2C_Init(&hi2c_display) != HAL_OK) {
        return ROTS_DISPLAY_ERROR;
    }
    
    return ROTS_OK;
}

/**
 * @brief Initialize SSD1306 display
 * @return ROTS_OK if successful, error code otherwise
 */
static ROTS_StatusTypeDef ROTS_Display_InitSSD1306(void)
{
    HAL_Delay(100);
    
    /* Initialization sequence */
    ROTS_Display_SendCommand(SSD1306_CMD_DISPLAY_OFF);
    ROTS_Display_SendCommand(SSD1306_CMD_SET_DISPLAY_CLOCK_DIV);
    ROTS_Display_SendCommand(0x80);
    ROTS_Display_SendCommand(SSD1306_CMD_SET_MULTIPLEX);
    ROTS_Display_SendCommand(0x3F);
    ROTS_Display_SendCommand(SSD1306_CMD_SET_DISPLAY_OFFSET);
    ROTS_Display_SendCommand(0x00);
    ROTS_Display_SendCommand(SSD1306_CMD_SET_START_LINE | 0x0);
    ROTS_Display_SendCommand(SSD1306_CMD_CHARGE_PUMP);
    ROTS_Display_SendCommand(0x14);
    ROTS_Display_SendCommand(SSD1306_CMD_MEMORY_MODE);
    ROTS_Display_SendCommand(0x00);
    ROTS_Display_SendCommand(SSD1306_CMD_SEG_REMAP | 0x1);
    ROTS_Display_SendCommand(SSD1306_CMD_COM_SCAN_DEC);
    ROTS_Display_SendCommand(SSD1306_CMD_SET_COMPINS);
    ROTS_Display_SendCommand(0x12);
    ROTS_Display_SendCommand(SSD1306_CMD_SET_CONTRAST);
    ROTS_Display_SendCommand(0xCF);
    ROTS_Display_SendCommand(SSD1306_CMD_SET_PRECHARGE);
    ROTS_Display_SendCommand(0xF1);
    ROTS_Display_SendCommand(SSD1306_CMD_SET_VCOM_DETECT);
    ROTS_Display_SendCommand(0x40);
    ROTS_Display_SendCommand(SSD1306_CMD_DISPLAY_ALL_ON_RESUME);
    ROTS_Display_SendCommand(SSD1306_CMD_NORMAL_DISPLAY);
    ROTS_Display_SendCommand(SSD1306_CMD_DEACTIVATE_SCROLL);
    ROTS_Display_SendCommand(SSD1306_CMD_DISPLAY_ON);
    
    HAL_Delay(100);
    
    return ROTS_OK;
}

/**
 * @brief Send command to SSD1306
 * @param command Command byte
 * @return ROTS_OK if successful
 */
static ROTS_StatusTypeDef ROTS_Display_SendCommand(uint8_t command)
{
    uint8_t cmd[2] = {0x00, command};  // Control byte 0x00 = command
    HAL_I2C_Master_Transmit(&hi2c_display, SSD1306_I2C_ADDRESS << 1, cmd, 2, 100);
    return ROTS_OK;
}

/**
 * @brief Send data to SSD1306
 * @param data Data buffer
 * @param length Data length
 * @return ROTS_OK if successful
 */
static ROTS_StatusTypeDef ROTS_Display_SendData(uint8_t* data, uint16_t length)
{
    if (!data || length == 0) {
        return ROTS_INVALID_PARAM;
    }
    
    /* Send data in chunks with control byte */
    for (uint16_t i = 0; i < length; i++) {
        uint8_t data_packet[2] = {0x40, data[i]};  // Control byte 0x40 = data
        if (HAL_I2C_Master_Transmit(&hi2c_display, SSD1306_I2C_ADDRESS << 1, data_packet, 2, 100) != HAL_OK) {
            return ROTS_ERROR;
        }
    }
    
    return ROTS_OK;
}

/**
 * @brief Clear frame buffer
 */
static void ROTS_Display_ClearBuffer(void)
{
    memset(frame_buffer, 0, SSD1306_FRAMEBUFFER_SIZE);
}

/**
 * @brief Update frame buffer with current display content
 */
static void ROTS_Display_UpdateFrameBuffer(void)
{
    /* Clear buffer */
    ROTS_Display_ClearBuffer();
    
    /* Draw line 1 (page 0) */
    if (display_line1[0] != '\0') {
        ROTS_Display_DrawString(0, 0, display_line1);
    }
    
    /* Draw line 2 (page 1) */
    if (display_line2[0] != '\0') {
        ROTS_Display_DrawString(0, 1, display_line2);
    }
    
    /* Draw line 3 (page 2) */
    if (display_line3[0] != '\0') {
        ROTS_Display_DrawString(0, 2, display_line3);
    }
    
    /* Draw line 4 (page 3) */
    if (display_line4[0] != '\0') {
        ROTS_Display_DrawString(0, 3, display_line4);
    }
}

/**
 * @brief Draw string to frame buffer
 * @param x X position (column)
 * @param page Page (0-7)
 * @param str String to draw
 */
static void ROTS_Display_DrawString(uint8_t x, uint8_t page, const char* str)
{
    uint8_t pos = x;
    while (*str && pos < SSD1306_WIDTH) {
        ROTS_Display_DrawChar(pos, page, *str);
        pos += 6;  // Character width + spacing
        str++;
    }
}

/**
 * @brief Draw character to frame buffer
 * @param x X position (column)
 * @param page Page (0-7)
 * @param c Character to draw
 */
static void ROTS_Display_DrawChar(uint8_t x, uint8_t page, char c)
{
    if (x >= SSD1306_WIDTH || page >= SSD1306_PAGES) {
        return;
    }
    
    /* Get font index */
    uint8_t font_index = 0;
    if (c >= ' ' && c <= 'Z') {
        font_index = c - ' ';
    } else if (c >= 'a' && c <= 'z') {
        font_index = c - 'a' + 'A' - ' ';
    }
    
    /* Draw character (5 columns) */
    uint8_t page_offset = page * SSD1306_WIDTH;
    for (uint8_t col = 0; col < 5 && (x + col) < SSD1306_WIDTH; col++) {
        frame_buffer[page_offset + x + col] = font_5x7[font_index][col];
    }
}

/**
 * @brief Flush frame buffer to display
 */
static void ROTS_Display_FlushBuffer(void)
{
    /* Set column address */
    ROTS_Display_SendCommand(SSD1306_CMD_COLUMN_ADDR);
    ROTS_Display_SendCommand(0);
    ROTS_Display_SendCommand(SSD1306_WIDTH - 1);
    
    /* Set page address */
    ROTS_Display_SendCommand(SSD1306_CMD_PAGE_ADDR);
    ROTS_Display_SendCommand(0);
    ROTS_Display_SendCommand(SSD1306_PAGES - 1);
    
    /* Send frame buffer */
    ROTS_Display_SendData(frame_buffer, SSD1306_FRAMEBUFFER_SIZE);
}

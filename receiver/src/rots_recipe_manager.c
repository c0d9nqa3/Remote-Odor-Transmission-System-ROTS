/**
 * @file rots_recipe_manager.c
 * @brief ROTS Recipe Manager Module
 * @author ROTS Team
 * @date 2024
 * 
 * Manages odor recipes and mixing formulas
 */

#include "rots_receiver.h"
#include "rots_recipe_manager.h"
#include "rots_spi_flash.h"
#include "rots_debug.h"
#include <string.h>

/* Recipe storage constants */
#define RECIPE_STORAGE_MAGIC            0x524F5453  // "ROTS"
#define RECIPE_STORAGE_VERSION          1
#define RECIPE_STORAGE_BASE_ADDR        0x00000000

/* Recipe structure */
typedef struct {
    ROTS_OdorType_t odor_type;
    char name[16];
    uint8_t pump_ratios[ROTS_MAX_PUMPS];  // Pump ratios (0-100%)
    bool valve_states[ROTS_MAX_VALVES];   // Valve states
    uint16_t mixing_time;                 // Mixing time in ms
    uint8_t fan_speed;                    // Fan speed (0-100%)
} ROTS_Recipe_t;

/* Predefined recipes */
static const ROTS_Recipe_t predefined_recipes[] = {
    // Coffee recipe
    {
        .odor_type = ROTS_ODOR_COFFEE,
        .name = "Coffee",
        .pump_ratios = {80, 0, 0, 0, 20},  // Pump 1: 80%, Pump 5: 20%
        .valve_states = {true, false, false, false, true},
        .mixing_time = 2000,
        .fan_speed = 60
    },
    // Alcohol recipe
    {
        .odor_type = ROTS_ODOR_ALCOHOL,
        .name = "Alcohol",
        .pump_ratios = {0, 90, 0, 0, 10},
        .valve_states = {false, true, false, false, true},
        .mixing_time = 1500,
        .fan_speed = 70
    },
    // Lemon recipe
    {
        .odor_type = ROTS_ODOR_LEMON,
        .name = "Lemon",
        .pump_ratios = {0, 0, 85, 0, 15},
        .valve_states = {false, false, true, false, true},
        .mixing_time = 1800,
        .fan_speed = 50
    },
    // Mint recipe
    {
        .odor_type = ROTS_ODOR_MINT,
        .name = "Mint",
        .pump_ratios = {0, 0, 0, 80, 20},
        .valve_states = {false, false, false, true, true},
        .mixing_time = 2200,
        .fan_speed = 65
    },
    // Lavender recipe
    {
        .odor_type = ROTS_ODOR_LAVENDER,
        .name = "Lavender",
        .pump_ratios = {70, 0, 0, 0, 30},
        .valve_states = {true, false, false, false, true},
        .mixing_time = 2500,
        .fan_speed = 55
    },
    // Mixed recipe
    {
        .odor_type = ROTS_ODOR_MIXED,
        .name = "Mixed",
        .pump_ratios = {30, 30, 20, 20, 0},
        .valve_states = {true, true, true, true, false},
        .mixing_time = 3000,
        .fan_speed = 75
    }
};

/* Recipe storage header structure */
typedef struct {
    uint32_t magic;              // Magic number "ROTS"
    uint16_t version;            // Storage format version
    uint16_t recipe_count;       // Number of custom recipes
    uint32_t checksum;           // Checksum for data integrity
} ROTS_RecipeStorageHeader_t;

/* Private variables */
static bool recipe_manager_initialized = false;
static ROTS_Recipe_t custom_recipes[10];  // Space for custom recipes
static uint8_t custom_recipe_count = 0;
static bool spi_flash_available = false;

/* Private function prototypes */
static ROTS_StatusTypeDef ROTS_RecipeManager_LoadCustomRecipes(void);
static ROTS_StatusTypeDef ROTS_RecipeManager_SaveCustomRecipes(void);
static uint32_t ROTS_RecipeManager_CalculateChecksum(const uint8_t* data, uint32_t length);

/**
 * @brief Initialize recipe manager
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_RecipeManager_Init(void)
{
    // Initialize SPI Flash for recipe storage
    ROTS_StatusTypeDef status = ROTS_SPI_Flash_Init();
    if (status == ROTS_OK) {
        spi_flash_available = true;
        DEBUG_INFO("SPI Flash available for recipe storage\r\n");
    } else {
        spi_flash_available = false;
        DEBUG_WARNING("SPI Flash not available, using RAM storage only\r\n");
    }
    
    // Load custom recipes from storage
    status = ROTS_RecipeManager_LoadCustomRecipes();
    if (status != ROTS_OK) {
        // If loading fails, initialize with empty custom recipes
        custom_recipe_count = 0;
        memset(custom_recipes, 0, sizeof(custom_recipes));
        DEBUG_WARNING("Failed to load custom recipes from storage\r\n");
    }
    
    recipe_manager_initialized = true;
    return ROTS_OK;
}

/**
 * @brief Get recipe for specific odor type
 * @param odor_type Odor type
 * @param recipe Pointer to recipe structure
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_RecipeManager_GetRecipe(ROTS_OdorType_t odor_type, ROTS_Recipe_t* recipe)
{
    if (!recipe_manager_initialized || !recipe) {
        return ROTS_INVALID_PARAM;
    }
    
    // Search predefined recipes
    for (int i = 0; i < sizeof(predefined_recipes) / sizeof(ROTS_Recipe_t); i++) {
        if (predefined_recipes[i].odor_type == odor_type) {
            memcpy(recipe, &predefined_recipes[i], sizeof(ROTS_Recipe_t));
            return ROTS_OK;
        }
    }
    
    // Search custom recipes
    for (int i = 0; i < custom_recipe_count; i++) {
        if (custom_recipes[i].odor_type == odor_type) {
            memcpy(recipe, &custom_recipes[i], sizeof(ROTS_Recipe_t));
            return ROTS_OK;
        }
    }
    
    return ROTS_RECIPE_ERROR;
}

/**
 * @brief Add custom recipe
 * @param recipe Recipe structure
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_RecipeManager_AddCustomRecipe(ROTS_Recipe_t* recipe)
{
    if (!recipe_manager_initialized || !recipe) {
        return ROTS_INVALID_PARAM;
    }
    
    if (custom_recipe_count >= 10) {
        return ROTS_MEMORY_ERROR;  // No space for more recipes
    }
    
    // Validate recipe
    if (recipe->odor_type < ROTS_ODOR_COFFEE || recipe->odor_type > ROTS_ODOR_MIXED) {
        return ROTS_INVALID_PARAM;
    }
    
    // Check if recipe already exists
    ROTS_Recipe_t existing_recipe;
    if (ROTS_RecipeManager_GetRecipe(recipe->odor_type, &existing_recipe) == ROTS_OK) {
        return ROTS_RECIPE_ERROR;  // Recipe already exists
    }
    
    // Add recipe
    memcpy(&custom_recipes[custom_recipe_count], recipe, sizeof(ROTS_Recipe_t));
    custom_recipe_count++;
    
    // Save to storage
    return ROTS_RecipeManager_SaveCustomRecipes();
}

/**
 * @brief Update existing recipe
 * @param recipe Recipe structure
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_RecipeManager_UpdateRecipe(ROTS_Recipe_t* recipe)
{
    if (!recipe_manager_initialized || !recipe) {
        return ROTS_INVALID_PARAM;
    }
    
    // Search and update custom recipes
    for (int i = 0; i < custom_recipe_count; i++) {
        if (custom_recipes[i].odor_type == recipe->odor_type) {
            memcpy(&custom_recipes[i], recipe, sizeof(ROTS_Recipe_t));
            return ROTS_RecipeManager_SaveCustomRecipes();
        }
    }
    
    // If not found in custom recipes, add as new custom recipe
    return ROTS_RecipeManager_AddCustomRecipe(recipe);
}

/**
 * @brief Delete custom recipe
 * @param odor_type Odor type to delete
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_RecipeManager_DeleteRecipe(ROTS_OdorType_t odor_type)
{
    if (!recipe_manager_initialized) {
        return ROTS_INVALID_PARAM;
    }
    
    // Search and remove custom recipe
    for (int i = 0; i < custom_recipe_count; i++) {
        if (custom_recipes[i].odor_type == odor_type) {
            // Shift remaining recipes
            for (int j = i; j < custom_recipe_count - 1; j++) {
                memcpy(&custom_recipes[j], &custom_recipes[j + 1], sizeof(ROTS_Recipe_t));
            }
            custom_recipe_count--;
            return ROTS_RecipeManager_SaveCustomRecipes();
        }
    }
    
    return ROTS_RECIPE_ERROR;  // Recipe not found
}

/**
 * @brief Get all available recipes
 * @param recipes Array to store recipes
 * @param max_count Maximum number of recipes to return
 * @param actual_count Pointer to actual count returned
 * @return ROTS_OK if successful, error code otherwise
 */
ROTS_StatusTypeDef ROTS_RecipeManager_GetAllRecipes(ROTS_Recipe_t* recipes, uint8_t max_count, uint8_t* actual_count)
{
    if (!recipe_manager_initialized || !recipes || !actual_count) {
        return ROTS_INVALID_PARAM;
    }
    
    uint8_t count = 0;
    
    // Copy predefined recipes
    for (int i = 0; i < sizeof(predefined_recipes) / sizeof(ROTS_Recipe_t) && count < max_count; i++) {
        memcpy(&recipes[count], &predefined_recipes[i], sizeof(ROTS_Recipe_t));
        count++;
    }
    
    // Copy custom recipes
    for (int i = 0; i < custom_recipe_count && count < max_count; i++) {
        memcpy(&recipes[count], &custom_recipes[i], sizeof(ROTS_Recipe_t));
        count++;
    }
    
    *actual_count = count;
    return ROTS_OK;
}

/**
 * @brief Load custom recipes from SPI Flash storage
 * @return ROTS_OK if successful, error code otherwise
 */
static ROTS_StatusTypeDef ROTS_RecipeManager_LoadCustomRecipes(void)
{
    if (!spi_flash_available) {
        // SPI Flash not available, use empty recipes
        custom_recipe_count = 0;
        memset(custom_recipes, 0, sizeof(custom_recipes));
        return ROTS_OK;
    }
    
    ROTS_RecipeStorageHeader_t header;
    ROTS_StatusTypeDef status;
    
    // Read storage header
    status = ROTS_SPI_Flash_Read(RECIPE_STORAGE_BASE_ADDR, (uint8_t*)&header, sizeof(header));
    if (status != ROTS_OK) {
        return status;
    }
    
    // Verify magic number
    if (header.magic != RECIPE_STORAGE_MAGIC) {
        DEBUG_WARNING("Invalid recipe storage magic number\r\n");
        custom_recipe_count = 0;
        memset(custom_recipes, 0, sizeof(custom_recipes));
        return ROTS_OK;  // Not an error, just no valid data
    }
    
    // Verify version
    if (header.version != RECIPE_STORAGE_VERSION) {
        DEBUG_WARNING("Recipe storage version mismatch\r\n");
        custom_recipe_count = 0;
        memset(custom_recipes, 0, sizeof(custom_recipes));
        return ROTS_OK;
    }
    
    // Limit recipe count
    uint16_t recipe_count = header.recipe_count;
    if (recipe_count > 10) {
        recipe_count = 10;
    }
    
    // Read recipes
    if (recipe_count > 0) {
        uint32_t recipe_data_size = recipe_count * sizeof(ROTS_Recipe_t);
        status = ROTS_SPI_Flash_Read(RECIPE_STORAGE_BASE_ADDR + sizeof(header), 
                                     (uint8_t*)custom_recipes, recipe_data_size);
        if (status != ROTS_OK) {
            return status;
        }
        
        // Verify checksum
        uint32_t calculated_checksum = ROTS_RecipeManager_CalculateChecksum(
            (uint8_t*)custom_recipes, recipe_data_size);
        if (calculated_checksum != header.checksum) {
            DEBUG_WARNING("Recipe storage checksum mismatch\r\n");
            custom_recipe_count = 0;
            memset(custom_recipes, 0, sizeof(custom_recipes));
            return ROTS_OK;
        }
    }
    
    custom_recipe_count = recipe_count;
    DEBUG_INFO("Loaded %d custom recipes from SPI Flash\r\n", custom_recipe_count);
    
    return ROTS_OK;
}

/**
 * @brief Save custom recipes to SPI Flash storage
 * @return ROTS_OK if successful, error code otherwise
 */
static ROTS_StatusTypeDef ROTS_RecipeManager_SaveCustomRecipes(void)
{
    if (!spi_flash_available) {
        // SPI Flash not available, recipes only stored in RAM
        return ROTS_OK;
    }
    
    ROTS_RecipeStorageHeader_t header;
    ROTS_StatusTypeDef status;
    
    // Prepare header
    header.magic = RECIPE_STORAGE_MAGIC;
    header.version = RECIPE_STORAGE_VERSION;
    header.recipe_count = custom_recipe_count;
    
    // Calculate checksum
    uint32_t recipe_data_size = custom_recipe_count * sizeof(ROTS_Recipe_t);
    header.checksum = ROTS_RecipeManager_CalculateChecksum(
        (uint8_t*)custom_recipes, recipe_data_size);
    
    // Erase sector before writing
    status = ROTS_SPI_Flash_EraseSector(RECIPE_STORAGE_BASE_ADDR);
    if (status != ROTS_OK) {
        DEBUG_ERROR("Failed to erase recipe storage sector\r\n");
        return status;
    }
    
    // Write header
    status = ROTS_SPI_Flash_Write(RECIPE_STORAGE_BASE_ADDR, (uint8_t*)&header, sizeof(header));
    if (status != ROTS_OK) {
        DEBUG_ERROR("Failed to write recipe storage header\r\n");
        return status;
    }
    
    // Write recipes
    if (custom_recipe_count > 0) {
        status = ROTS_SPI_Flash_Write(RECIPE_STORAGE_BASE_ADDR + sizeof(header),
                                      (uint8_t*)custom_recipes, recipe_data_size);
        if (status != ROTS_OK) {
            DEBUG_ERROR("Failed to write recipes to SPI Flash\r\n");
            return status;
        }
    }
    
    DEBUG_INFO("Saved %d custom recipes to SPI Flash\r\n", custom_recipe_count);
    
    return ROTS_OK;
}

/**
 * @brief Calculate checksum for data integrity
 * @param data Data buffer
 * @param length Data length
 * @return Calculated checksum
 */
static uint32_t ROTS_RecipeManager_CalculateChecksum(const uint8_t* data, uint32_t length)
{
    uint32_t checksum = 0;
    
    for (uint32_t i = 0; i < length; i++) {
        checksum += data[i];
        checksum = (checksum << 1) | (checksum >> 31);  // Rotate left
    }
    
    return checksum;
}

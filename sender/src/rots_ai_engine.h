// ROTS AI Engine Header
#ifndef ROTS_AI_ENGINE_H
#define ROTS_AI_ENGINE_H

#include "rots_sender.h"

#ifdef __cplusplus
extern "C" {
#endif

// AI配置
#define ROTS_AI_FEATURE_SIZE      15
#define ROTS_AI_MODEL_SIZE        90  // 6 odors * 15 features
#define ROTS_AI_MAX_CONFIDENCE    1.0f
#define ROTS_AI_MIN_CONFIDENCE    0.0f

// AI状态结构
typedef struct {
    bool initialized;
    uint32_t last_inference_time;
    ROTS_OdorType_t last_odor_type;
    float last_confidence;
    uint32_t inference_count;
} ROTS_AIStatus_t;

// 函数声明
ROTS_StatusTypeDef ROTS_AIEngine_Init(void);
ROTS_StatusTypeDef ROTS_AIEngine_ProcessOdor(ROTS_OdorResult_t* result);
ROTS_StatusTypeDef ROTS_AIEngine_GetStatus(ROTS_AIStatus_t* status);
ROTS_StatusTypeDef ROTS_AIEngine_UpdateModel(const float* new_weights, uint16_t size);
ROTS_StatusTypeDef ROTS_AIEngine_Reset(void);

#ifdef __cplusplus
}
#endif

#endif /* ROTS_AI_ENGINE_H */

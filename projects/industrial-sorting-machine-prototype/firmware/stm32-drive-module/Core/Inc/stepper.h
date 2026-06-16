#ifndef STEPPER_H
#define STEPPER_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

typedef enum {
    STEPPER_MODE_IDLE = 0,
    STEPPER_MODE_SPEED,
    STEPPER_MODE_POSITION
} StepperMode_t;

typedef struct {
    // GPIO
    GPIO_TypeDef *dir_port;
    uint16_t dir_pin;

    GPIO_TypeDef *enable_port;
    uint16_t enable_pin;

    // Timer PWM for STEP
    TIM_HandleTypeDef *htim;
    uint32_t tim_channel;

    // Motor parameters
    uint8_t microstep;
    float acceleration;      // rev/s^2
    float maxSpeed;          // rev/s
    uint32_t pulsesPerRev;

    // PWM
    uint32_t pulseWidthUs;   // STEP high pulse width

    // Ramp
    uint32_t stepLUT[3200];  // intervals in us
    uint16_t lutLength;
    volatile uint16_t lutIndex;
    volatile uint32_t targetInterval;

    // Position
    volatile int32_t currentPos;
    volatile int32_t targetPos;

    // State
    volatile uint8_t running;
    volatile uint8_t dir;
    volatile StepperMode_t mode;

} Stepper_t;


// Init
void Stepper_GPIO_Timer_Init(Stepper_t *s,
                             GPIO_TypeDef *dir_port, uint16_t dir_pin,
                             GPIO_TypeDef *enable_port, uint16_t enable_pin,
                             TIM_HandleTypeDef *htim,
                             uint32_t tim_channel);

void Stepper_Init(Stepper_t *s,
                  float acc,
                  float maxSpeed,
                  uint8_t microstep,
                  uint32_t pulseWidthUs);


// Motion
void Stepper_RunSpeed(Stepper_t *s, float speed, uint8_t dir);
void Stepper_GoToPos(Stepper_t *s, float speed, float pos);
void Stepper_Stop(Stepper_t *s);
void Stepper_StopHard(Stepper_t *s);


// Timer interrupt callback
void Stepper_TimerCallback(Stepper_t *s, TIM_HandleTypeDef *htim);


// Helpers
uint8_t Stepper_IsRunning(Stepper_t *s);
uint8_t Stepper_GetDir(Stepper_t *s);
void Stepper_SetCurrentPos(Stepper_t *s, float pos);

#endif


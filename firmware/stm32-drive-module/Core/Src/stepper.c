#include "stepper.h"
#include <math.h>
#include <stdlib.h>

static void Stepper_SetPWM(Stepper_t *s, uint32_t intervalUs)
{
    if (intervalUs <= s->pulseWidthUs + 2)
        intervalUs = s->pulseWidthUs + 2;

    __HAL_TIM_SET_AUTORELOAD(s->htim, intervalUs - 1);
    __HAL_TIM_SET_COMPARE(s->htim, s->tim_channel, s->pulseWidthUs);
}


static void Stepper_PWM_Start(Stepper_t *s)
{
    __HAL_TIM_DISABLE_IT(s->htim, TIM_IT_UPDATE);
    __HAL_TIM_CLEAR_FLAG(s->htim, TIM_FLAG_UPDATE);

    __HAL_TIM_SET_COUNTER(s->htim, 0);

    if (s->htim->Instance == TIM1)
    {
        __HAL_TIM_MOE_ENABLE(s->htim);
    }

    HAL_TIM_PWM_Start(s->htim, s->tim_channel);

    __HAL_TIM_CLEAR_FLAG(s->htim, TIM_FLAG_UPDATE);
    __HAL_TIM_ENABLE_IT(s->htim, TIM_IT_UPDATE);
}

static void Stepper_PWM_Stop(Stepper_t *s)
{
    __HAL_TIM_DISABLE_IT(s->htim, TIM_IT_UPDATE);
    HAL_TIM_PWM_Stop(s->htim, s->tim_channel);

    if (s->htim->Instance == TIM1)
    {
        __HAL_TIM_MOE_DISABLE(s->htim);
    }
}

void Stepper_GPIO_Timer_Init(Stepper_t *s,
                             GPIO_TypeDef *dir_port, uint16_t dir_pin,
                             GPIO_TypeDef *enable_port, uint16_t enable_pin,
                             TIM_HandleTypeDef *htim,
                             uint32_t tim_channel)
{
    s->dir_port = dir_port;
    s->dir_pin = dir_pin;

    s->enable_port = enable_port;
    s->enable_pin = enable_pin;

    s->htim = htim;
    s->tim_channel = tim_channel;

    HAL_GPIO_WritePin(s->dir_port, s->dir_pin, GPIO_PIN_RESET);

    // Enable pin is active-low.
    HAL_GPIO_WritePin(s->enable_port, s->enable_pin, GPIO_PIN_SET);

    s->running = 0;
    s->mode = STEPPER_MODE_IDLE;
    s->dir = 0;
    s->currentPos = 0;
    s->targetPos = 0;
    s->lutIndex = 0;
    s->lutLength = 0;
    s->targetInterval = 1000;
}

void Stepper_Init(Stepper_t *s,
                  float acc,
                  float maxSpeed,
                  uint8_t microstep,
                  uint32_t pulseWidthUs)
{
    s->microstep = microstep;
    s->acceleration = acc;
    s->maxSpeed = maxSpeed;
    s->pulseWidthUs = pulseWidthUs;

    s->pulsesPerRev = 200UL * (uint32_t)microstep;

    s->lutIndex = 0;
    s->lutLength = 0;
    s->targetInterval = 1000;

    float accPls = acc * (float)s->pulsesPerRev;
    float maxSpeedPls = maxSpeed * (float)s->pulsesPerRev;

    uint32_t len = sizeof(s->stepLUT) / sizeof(s->stepLUT[0]);

    if (accPls <= 0.0f)
        accPls = 1.0f;

    if (maxSpeedPls <= 0.0f)
        maxSpeedPls = 1.0f;

    float n_max_f = (maxSpeedPls * maxSpeedPls) / (2.0f * accPls);

    if (n_max_f > (float)len)
        n_max_f = (float)len;

    uint32_t n_max = (uint32_t)n_max_f;

    if (n_max < 1)
        n_max = 1;

    float prev_t = 0.0f;

    for (uint32_t n = 1; n <= n_max; n++)
    {
        float t = sqrtf((2.0f * (float)n) / accPls);
        float dt = t - prev_t;

        uint32_t interval = (uint32_t)(dt * 1000000.0f);

        if (interval <= pulseWidthUs + 2)
            interval = pulseWidthUs + 2;

        s->stepLUT[n - 1] = interval;
        prev_t = t;
    }

    s->lutLength = (uint16_t)n_max;
}

void Stepper_RunSpeed(Stepper_t *s, float speed, uint8_t dir)
{
    if (speed < 0.0f)
        speed = -speed;

    if (speed <= 0.0f)
    {
        Stepper_Stop(s);
        return;
    }

    if (speed > s->maxSpeed)
        speed = s->maxSpeed;

    float pulses = speed * (float)s->pulsesPerRev;

    if (pulses <= 0.0f || s->lutLength == 0)
        return;

    uint32_t newTargetInterval = (uint32_t)(1000000.0f / pulses);

    if (newTargetInterval <= s->pulseWidthUs + 2)
        newTargetInterval = s->pulseWidthUs + 2;

    // If it is already running in the same mode and direction, update speed only.
    if (s->running && s->mode == STEPPER_MODE_SPEED && s->dir == (dir ? 1 : 0))
    {
        s->targetInterval = newTargetInterval;
        return;
    }

    s->targetInterval = newTargetInterval;
    s->dir = dir ? 1 : 0;
    s->mode = STEPPER_MODE_SPEED;
    s->lutIndex = 0;
    s->running = 1;

    HAL_GPIO_WritePin(s->dir_port, s->dir_pin, s->dir ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(s->enable_port, s->enable_pin, GPIO_PIN_RESET);

    Stepper_SetPWM(s, s->stepLUT[0]);
    Stepper_PWM_Start(s);
}

void Stepper_GoToPos(Stepper_t *s, float speed, float pos)
{
    int32_t target = (int32_t)(pos * (float)s->pulsesPerRev);

    if (target == s->currentPos)
    {
        Stepper_StopHard(s);
        return;
    }

    if (speed < 0.0f)
        speed = -speed;

    if (speed <= 0.0f || s->lutLength == 0)
        return;

    if (speed > s->maxSpeed)
        speed = s->maxSpeed;

    float pulses = speed * (float)s->pulsesPerRev;

    uint32_t newTargetInterval = (uint32_t)(1000000.0f / pulses);

    if (newTargetInterval <= s->pulseWidthUs + 2)
        newTargetInterval = s->pulseWidthUs + 2;

    s->targetInterval = newTargetInterval;
    s->targetPos = target;

    s->dir = (s->targetPos > s->currentPos) ? 1 : 0;

    s->mode = STEPPER_MODE_POSITION;
    s->lutIndex = 0;
    s->running = 1;

    HAL_GPIO_WritePin(s->dir_port, s->dir_pin, s->dir ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(s->enable_port, s->enable_pin, GPIO_PIN_RESET);

    Stepper_SetPWM(s, s->stepLUT[0]);
    Stepper_PWM_Start(s);
}

void Stepper_Stop(Stepper_t *s)
{
    if (!s->running)
        return;

    if (s->mode == STEPPER_MODE_SPEED)
    {
        int32_t brakingSteps = (int32_t)s->lutIndex;

        if (brakingSteps < 1)
            brakingSteps = 1;

        if (s->dir)
            s->targetPos = s->currentPos + brakingSteps;
        else
            s->targetPos = s->currentPos - brakingSteps;

        s->mode = STEPPER_MODE_POSITION;
    }
}

void Stepper_StopHard(Stepper_t *s)
{
    Stepper_PWM_Stop(s);

    s->running = 0;
    s->mode = STEPPER_MODE_IDLE;
    s->lutIndex = 0;

    HAL_GPIO_WritePin(s->enable_port, s->enable_pin, GPIO_PIN_SET);
}

void Stepper_TimerCallback(Stepper_t *s, TIM_HandleTypeDef *htim)
{
    if (s->htim != htim)
        return;

    if (!s->running)
        return;

    if (s->dir)
        s->currentPos++;
    else
        s->currentPos--;

    if (s->mode == STEPPER_MODE_POSITION)
    {
        int32_t remaining;

        if (s->targetPos > s->currentPos)
            remaining = s->targetPos - s->currentPos;
        else
            remaining = s->currentPos - s->targetPos;

        if (remaining <= 0)
        {
            Stepper_StopHard(s);
            return;
        }

        if (remaining <= (int32_t)s->lutIndex)
        {
            if (s->lutIndex > 0)
                s->lutIndex--;
        }
        else
        {
            if ((s->lutIndex + 1) < s->lutLength)
            {
                if (s->stepLUT[s->lutIndex] > s->targetInterval)
                    s->lutIndex++;
            }
        }
    }
    else if (s->mode == STEPPER_MODE_SPEED)
    {
        if ((s->lutIndex + 1) < s->lutLength)
        {
            if (s->stepLUT[s->lutIndex] > s->targetInterval)
                s->lutIndex++;
        }
    }

    uint32_t interval;

    if (s->stepLUT[s->lutIndex] <= s->targetInterval)
        interval = s->targetInterval;
    else
        interval = s->stepLUT[s->lutIndex];

    Stepper_SetPWM(s, interval);
}

uint8_t Stepper_IsRunning(Stepper_t *s)
{
    return s->running;
}

uint8_t Stepper_GetDir(Stepper_t *s)
{
    return s->dir;
}

void Stepper_SetCurrentPos(Stepper_t *s, float pos)
{
    s->currentPos = (int32_t)(pos * (float)s->pulsesPerRev);
}

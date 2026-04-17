#include "hx711.h"
#include "stm32f3xx_hal_gpio.h"
#include <stdint.h>

static void HX711_Delay(uint8_t value){
    for(volatile uint32_t i = 0; i < value; i++);
}

void HX711_Init(HX711_Handle_t *hx){
    HAL_GPIO_WritePin(hx->sck_Port, hx->sck_Pin, GPIO_PIN_RESET);

    hx->offset = 0;

    if(hx->scale == 0.0f){
        hx->scale = 1.0f;
    }
}

int32_t HX711_Raw(HX711_Handle_t *hx){
    int32_t value = 0;

    //Esperar DOUT Ficar LOW
    while(HAL_GPIO_ReadPin(hx->dt_Port, hx->dt_Pin));

    for(int i = 0; i < 24; i++){
        //Gera um pulso de clock borda de subida
        HAL_GPIO_WritePin(hx->sck_Port, hx->sck_Pin, GPIO_PIN_SET);
        HX711_Delay(5);

        value = value << 1; //Desloca os bits para a esquerda
        if(HAL_GPIO_ReadPin(hx->dt_Port, hx->dt_Pin)){
            value++;
        }

        //Gera um pulso de clock borda de descida
        HAL_GPIO_WritePin(hx->sck_Port, hx->sck_Pin, GPIO_PIN_RESET); 
        HX711_Delay(5);
    }

    for(uint8_t i = 0; i < hx->gain; i++){
        HAL_GPIO_WritePin(hx->sck_Port, hx->sck_Pin, GPIO_PIN_SET);
        HX711_Delay(5);
        HAL_GPIO_WritePin(hx->sck_Port, hx->sck_Pin, GPIO_PIN_RESET);
        HX711_Delay(5);
    }

    if(value & 0x800000){
        value |= 0xFF000000;
    }

    return value;
}

void HX711_Tare (HX711_Handle_t *hx, uint8_t samples){
  int64_t sum = 0;

  for(uint8_t i = 0; i < samples; i++){
    sum += HX711_Raw(hx);
  }

  hx->offset = (int32_t)(sum / samples);
}

float HX711_GetWeight(HX711_Handle_t *hx, uint8_t samples, float callibration){
  int64_t sum = 0;

  for(uint8_t i = 0; i < samples; i++){
    sum+=HX711_Raw(hx);
  }

  float raw_avg = (float)(sum/samples);

  return (raw_avg - hx->offset) / callibration;
}
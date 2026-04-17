#ifndef HX711_H
#define HX711_H

#include "stm32f334x8.h"
#ifdef __cplusplus
extern "C" {
#endif

/*------INCLUDES-------*/
#include "stm32f3xx_hal.h"
#include <stdint.h>

/*------DEFINES-------*/
/**
* @brief Ganho do HX711
*/

#define HX711_GAIN_128 1 //Canal A, Ganho - 128
#define HX711_GAIN_64 3 //Canal B, Ganho - 64
#define HX711_GAIN_32 2 //Canal C (A - segundo datasheet), Ganho - 32

/*------TYPES-------*/
/**
* @brief Estrutura de controle
*/

typedef struct{
    GPIO_TypeDef *dt_Port;
    uint16_t dt_Pin;

    GPIO_TypeDef *sck_Port;
    uint16_t sck_Pin;

    uint8_t gain;
    int32_t offset; //Tara - Valor médio
    float scale; //Calibração - Calculado a mão
} HX711_Handle_t;

/*------Functions-------*/

/**
* @brief Inicializa o HX711
* @param hx pinteiro para o handle
*/
void HX711_Init(HX711_Handle_t *hx);

/**
* @brief Lê valor bruto de 24 bits do HX711
* @param hx pinteiro para o handle
* @return valor cru com sinal
*/
int32_t HX711_Raw(HX711_Handle_t *hx);

/**
* @brief Calibra o valor de zero (tara)
* @param hx pinteiro para o handle
* @param samples número de amostras
*/
void HX711_Tare(HX711_Handle_t *hx, uint8_t samples);

/**
* @brief Obtém peso calculado
* @param hx pinteiro para o handle
* @param callibration valor de calibração - calculado a mão
* @param samples número de amostras
*/
float HX711_GetWeight(HX711_Handle_t *hx, uint8_t samples, float callibration);

#ifdef __cplusplus
}
#endif

#endif /* HX711_H */
 /*
 * MAIN Generated Driver File
 * 
 * @file main.c
 * 
 * @defgroup main MAIN
 * 
 * @brief This is the generated driver implementation file for the MAIN driver.
 *
 * @version MAIN Driver Version 1.0.0
*/

/*
© [2023] Microchip Technology Inc. and its subsidiaries.

    Subject to your compliance with these terms, you may use Microchip 
    software and any derivatives exclusively with Microchip products. 
    You are responsible for complying with 3rd party license terms  
    applicable to your use of 3rd party software (including open source  
    software) that may accompany Microchip software. SOFTWARE IS ?AS IS.? 
    NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, APPLY TO THIS 
    SOFTWARE, INCLUDING ANY IMPLIED WARRANTIES OF NON-INFRINGEMENT,  
    MERCHANTABILITY, OR FITNESS FOR A PARTICULAR PURPOSE. IN NO EVENT 
    WILL MICROCHIP BE LIABLE FOR ANY INDIRECT, SPECIAL, PUNITIVE, 
    INCIDENTAL OR CONSEQUENTIAL LOSS, DAMAGE, COST OR EXPENSE OF ANY 
    KIND WHATSOEVER RELATED TO THE SOFTWARE, HOWEVER CAUSED, EVEN IF 
    MICROCHIP HAS BEEN ADVISED OF THE POSSIBILITY OR THE DAMAGES ARE 
    FORESEEABLE. TO THE FULLEST EXTENT ALLOWED BY LAW, MICROCHIP?S 
    TOTAL LIABILITY ON ALL CLAIMS RELATED TO THE SOFTWARE WILL NOT 
    EXCEED AMOUNT OF FEES, IF ANY, YOU PAID DIRECTLY TO MICROCHIP FOR 
    THIS SOFTWARE.
*/
#include "mcc_generated_files/system/system.h"

/*
    Main application
*/

#define RSHUNT                                      10.0                                        /* ohms */
#define OPA_GAIN                                    4.0
#define VREF                                        3300.0                                      /* mV */
#define MAX_ADC_STEPS                               4096.0                                      /* ADC result is right justified */
#define MAX_DCY_REG                                 63999.0                                     /* MCC generated value for PWM period register */

#define ADC_STEPS_TO_CURRENT(adcVal)                (float)(((adcVal) * VREF) / (RSHUNT * OPA_GAIN * MAX_ADC_STEPS)) 
#define CURRENT_TO_ADC_STEPS(mA)                    (adc_result_t)(((mA) * RSHUNT * OPA_GAIN * MAX_ADC_STEPS) / VREF)

#define PWM_STEPS_TO_DCY(pwm)                       (float)((pwm) / MAX_DCY_REG)
#define DCY_TO_PWM_STEPS(dcy)                       (uint32_t)((dcy) * MAX_DCY_REG)                          

#define ADC_STEPS_TO_POT_POSITION(adcVal)           (float)((adcVal) / MAX_ADC_STEPS)
#define POT_POSITION_TO_ADC_STEPS(potPos)           (adc_result_t)((potPos) * MAX_ADC_STEPS)

#define MAX_CURRENT_ADC_STEPS                        CURRENT_TO_ADC_STEPS(40.0)                 /* mA */
#define MIN_POT                                      POT_POSITION_TO_ADC_STEPS(0.01)            /* PWM is OFF below this POT position value */
#define MIN_PWM_DCY                                  DCY_TO_PWM_STEPS(0.0)                      /* min PWM value */
#define MAX_PWM_DCY                                  DCY_TO_PWM_STEPS(0.999)                    /* max PWM value */

static volatile adc_result_t adcResultPOT = 0;
static volatile adc_result_t adcResultOPA = 0;
static volatile uint32_t pwmDutyCycle = 0;
static volatile adc_context_t currentContext = 0;

void PWM_Handler(void)
{
    currentContext = 0;    
    ADC_SelectContext(currentContext);
    ADC_StartConversion();
}

void ADC_Handler(void)
{
    if(currentContext == 0)
    {
        currentContext = 1;
        ADC_SelectContext(currentContext);
        ADC_StartConversion();
    }
}

void ADC_Context1_Handler(void)
{
    adcResultOPA = ADC_GetFilterValue();
}

void ADC_Context2_Handler(void)
{
    adcResultPOT = ADC_GetFilterValue();
    if(adcResultOPA == 0)
    {
        pwmDutyCycle = 0;
    }
    else
    {
       pwmDutyCycle = (uint32_t)((adcResultPOT * MAX_DCY_REG * MAX_CURRENT_ADC_STEPS) / (adcResultOPA * MAX_ADC_STEPS)); 
    }
    if(adcResultPOT <= MIN_POT)
    {
        pwmDutyCycle = MIN_PWM_DCY;
    }
    if(pwmDutyCycle > MAX_PWM_DCY)
    {
        pwmDutyCycle = MAX_PWM_DCY;
    }
    PWM2_16BIT_SetSlice1Output1DutyCycleRegister((uint16_t)(pwmDutyCycle));
    PWM2_16BIT_LoadBufferRegisters();
}

int main(void)
{
    adc_result_t copyOfAdcResultPOT = 0;
    adc_result_t copyOfAdcResultOPA = 0;  
    uint32_t copyOfPwmDutyCycle = 0;
    
    SYSTEM_Initialize();

    PWM2_16BIT_Period_SetInterruptHandler(PWM_Handler);
    ADC_SetADIInterruptHandler(ADC_Handler);
    ADC_SetContext1ThresholdInterruptHandler(ADC_Context1_Handler);
    ADC_SetContext2ThresholdInterruptHandler(ADC_Context2_Handler);

    // If using interrupts in PIC18 High/Low Priority Mode you need to enable the Global High and Low Interrupts 
    // If using interrupts in PIC Mid-Range Compatibility Mode you need to enable the Global Interrupts 
    // Use the following macros to: 

    // Enable the Global High Interrupts 
    INTERRUPT_GlobalInterruptHighEnable(); 

    // Disable the Global High Interrupts 
    //INTERRUPT_GlobalInterruptHighDisable(); 

    // Enable the Global Low Interrupts 
    INTERRUPT_GlobalInterruptLowEnable(); 

    // Disable the Global Low Interrupts 
    //INTERRUPT_GlobalInterruptLowDisable(); 
    
    __delay_ms(2000);
    printf(" PROGRAM START \n\r");
    
    while(1)
    {
        INTERRUPT_GlobalInterruptHighDisable();                 /* atomic block for safe copy of shared variables */
        copyOfAdcResultPOT = adcResultPOT;
        copyOfAdcResultOPA = adcResultOPA;
        copyOfPwmDutyCycle = pwmDutyCycle;
        INTERRUPT_GlobalInterruptHighEnable();
        
        printf(" POT (ADC) = %u |", copyOfAdcResultPOT);
        printf(" POT = %.2f%% |", ADC_STEPS_TO_POT_POSITION(copyOfAdcResultPOT) * 100.0);
        printf(" ON LED Current (ADC) = %u |", copyOfAdcResultOPA);
        printf(" ON LED Current = %.3f mA |", ADC_STEPS_TO_CURRENT(copyOfAdcResultOPA));
        printf(" PWM DCY (reg val) = %lu |", copyOfPwmDutyCycle);
        printf(" PWM DCY = %.2f%% |", PWM_STEPS_TO_DCY(copyOfPwmDutyCycle) * 100.0);
        printf(" AVG LED Current = %.3f mA \n\r", ADC_STEPS_TO_CURRENT(copyOfAdcResultOPA) * PWM_STEPS_TO_DCY(copyOfPwmDutyCycle));
        __delay_ms(1000);
    }    
}
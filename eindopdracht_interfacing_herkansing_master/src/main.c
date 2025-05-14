/*!
 * \file    main.c
 * \author  Rob Beaufort 
 * \brief   Dit proggamma leest de waardes van de accelerometer uit.
 *          Deze waardes worden omgezet naar G-waarden en verzonden via NRF. 
 *          Er wordt hierbij gebruik gemaakt van een master slave configuratie 
 *          waarbij dit is de code is voor de master.
 * \version 2.0
 * \date    6-4-2025
 */
#define F_CPU 32000000UL

#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "clock.h"
#include "serialF0.h"
#include "nrf24_pindef.h"
#include "nrf24L01.h"
#include "nrf24spiXM2.h"
#include <string.h>
#include "i2c.h"
#include "HVA_accel.h"


#define NRF_CHANNEL  76

// Hier worden alle globalen variabelen en arrays gedefinieerd.
uint8_t rx_packet[128];
uint8_t master[5] = "MTOSP"; // Master to slave pipe
uint8_t slave[5] = "STOMP"; // Slave to master pipe
volatile int8_t measurementsFlag = 1;

//deze functie is voor het uitlezen van de adc en is gebaseerd op de practicum handleiding
/*
* Hier worden de variabelen gedefinieerd waar de uitgelezen waarden in opgeslagen gaan worden.
* Voor elke as is er een Low en een High. Dit is omdat we een 16-bit waarde willen uitlezen uit 2 8-bit registers.
* De 16-bit waarde is daardoor in 2 delen opgedeelt. 
* Low = de minst significante kant van het getal en High = de meest significante kant van het getal. 
* 
* Voorbeeld:
*                        16-bit getal=
*                    | 0110 1011 0001 1100 |
*                      High=          Low=
*                  | 0110 1011 | | 0001 1100 |
*/
typedef struct {
    int8_t xLow;
    int8_t xHigh;
    int8_t yLow;
    int8_t yHigh;
    int8_t zLow;
    int8_t zHigh;
    int16_t x;
    int16_t y;
} AccelerometerReadings;
  
typedef struct {
  float x;
  float y;
} ACCOmgerekend;

void nrf_init(void){
  nrfspiInit();
  nrfBegin();
  nrfSetRetries(NRF_SETUP_ARD_1000US_gc, NRF_SETUP_ARC_8RETRANSMIT_gc);
  nrfSetPALevel(NRF_RF_SETUP_PWR_6DBM_gc);
  nrfSetDataRate(NRF_RF_SETUP_RF_DR_250K_gc);
  nrfSetCRCLength(NRF_CONFIG_CRC_16_gc);
  nrfSetChannel(NRF_CHANNEL);
  nrfSetAutoAck(1);
  nrfEnableDynamicPayloads();
  nrfClearInterruptBits();
  nrfFlushRx();
  nrfFlushTx();

  // Interrupt Pin
  NRF24_IRQ_PORT.INT0MASK |= NRF24_IRQ_PIN;
  NRF24_IRQ_PORT.NRF24_IRQ_CTRL = PORT_ISC_FALLING_gc;
  // Interrupts On
  NRF24_IRQ_PORT.INTCTRL |=
  (NRF24_IRQ_PORT.INTCTRL & ~PORT_INT0LVL_gm) | PORT_INT0LVL_LO_gc;

  // Opening pipes
  nrfOpenWritingPipe((uint8_t *) master);
  nrfOpenReadingPipe(0, (uint8_t *) slave);
  nrfStartListening();
  nrfPowerUp();
}

void changeModeWake(TWI_t *twi){
  i2c_start(twi, ACC_ID, I2C_WRITE);
  i2c_write(twi, ACC_MODE);
  i2c_write(twi, ACC_WAKE);
  i2c_stop(twi);
}

// De versnellingen van de X-, Y- en Z-assen uit de accelerometer worden hier uitgelezen.
uint8_t readRegisterAccelerometer(TWI_t *twi, AccelerometerReadings *ACCData){
    i2c_start(twi, ACC_ID, I2C_WRITE);
    i2c_write(twi, XOUT_EX_L);
    i2c_restart(twi, ACC_ID, I2C_READ);
    ACCData->xLow = i2c_read(twi, I2C_ACK);
    ACCData->xHigh = i2c_read(twi, I2C_ACK);
    ACCData->yLow = i2c_read(twi, I2C_ACK);
    ACCData->yHigh = i2c_read(twi, I2C_NACK);
    i2c_stop(twi);
  
    // Hier worden de Low en High bytes samengevoegd. 
    // Dit wordt gedaan door de high waardes 8 plekken naar links te verschuiven.
  
    ACCData->x = (ACCData->xHigh <<8) + ACCData->xLow;
    ACCData->y = (ACCData->yHigh <<8) + ACCData->yLow;
    return 0;
  }
  
// Rekent de gemeten waardes om naar G-waarden. 
void calculateAcceleration(AccelerometerReadings *rawData, ACCOmgerekend *RealAcceleration){
  RealAcceleration->x = (float)(rawData->x * ValpBit_G2);
  RealAcceleration->y = (float)(-1* rawData->y * ValpBit_G2);

}

// Hier worden alle waardes geprint naar de buffer. Vervolgens wordt deze buffer verzonden via NRF.
void nrfSend(float x, float y){
  char buffer[128];
  sprintf(buffer,"%f,%f", x, y); 
  nrfStopListening();
  printf("%f,%f\n", x, y); 
  cli();
  nrfWrite((uint8_t *)buffer, strlen(buffer));
  sei();
  nrfStartListening();

}

// Deze timer wordt gebruikt om de frequentie van de metingen aan te passen.
void init_measurements_timer(void)
{
  TCE0.CTRLB = TC_WGMODE_NORMAL_gc;
  TCE0.CTRLA = TC_CLKSEL_DIV64_gc;
  TCE0.INTCTRLA = TC_OVFINTLVL_LO_gc;
  TCE0.PER = 31999;
}

// Wanneer de timer overloopt zal measurementsFlag op 1 gezet worden en zal er een meting gedaan worden.
ISR(TCE0_OVF_vect){
  measurementsFlag = 1;
}

int main(void){   
  
  //Hier worden alle initialisaties gedaan.
  init_clock();
  init_stream(F_CPU);
  i2c_init(&TWIE, TWI_BAUD(F_CPU, BAUD_400K));
  changeModeWake(&TWIE);
  nrf_init();
  init_measurements_timer();
  clear_screen();
  
  // In deze structs worden alle waardes van de accelerometer opgeslagen.
  AccelerometerReadings rawAcceleration;
  ACCOmgerekend accelarationG;

  sei();

  while (1) { 
    if(measurementsFlag){
      measurementsFlag = 0;

      readRegisterAccelerometer(&TWIE, &rawAcceleration);
      calculateAcceleration(&rawAcceleration, &accelarationG);
      nrfSend(accelarationG.x, accelarationG.y);
    }
  }
}

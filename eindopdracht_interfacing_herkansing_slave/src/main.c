/*!
 * \file    main.c
 * \author  Rob Beaufort 
 * \brief   Dit proggamma ontvangt een x en y waarde van de master.
 *          VerVolgens worden er ballen getekend op een TFT display.
 *          Daarna worden de ballen verplaatst gebaseerd op de ontvangen x en y waardes. 
 *          
 *          Het doel van het spel is om alle ballen op tergelijk over elkaar te laten bewegen.
 * \version 2.0
 * \date    11-4-2025
 */
#define F_CPU 32000000UL

#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "clock.h"
#include "serialF0.h"
#include "ucglib_xmega.h"
#include "moving_discs.h"
#include "nrf24_pindef.h"
#include "nrf24L01.h"
#include "nrf24spiXM2.h"
#include <string.h>
#include "balls.h"

#define NRF_CHANNEL  76

uint8_t rx_packet[128];
uint8_t master[5] = "MTOSP"; // Master to slave pipe
uint8_t slave[5] = "STOMP";  // Slave to master pipe
volatile uint8_t rx_flag;

// Hier wordt de nrf geinitialiseerd.
// Deze functie is gebaseerd op het NRF master-slave voorbeeld van Caspar Treijtel.
void nrf_init(){
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
  nrfOpenWritingPipe((uint8_t *) slave);
  nrfOpenReadingPipe(0, (uint8_t *) master);
  nrfStartListening();
  nrfPowerUp();
}

typedef struct {
  float x;
  float y;
} ACCOmgerekend;

// Hier wordt de ugc library geinitialiseerd.
// Deze functie is gebaseerd op tft_display_ucg van Caspar Treijtel uit 2023.
void ucg_init(ucg_t *ucg) {
  ucg_com_fnptr ucg_xmega_func = &ucg_com_xmega_cb;
  ucg_Init(ucg, ucg_dev_st7735_18x128x160, ucg_ext_st7735_18, ucg_xmega_func);

}

int main(void){   
  
  //Hier worden alle initialisaties gedaan.
  init_clock();
  init_stream(F_CPU);
  clear_screen();

  ACCOmgerekend accelarationG;
  char full_buffer[32];
  char x_buffer[16];
  char y_buffer[16];
  
  // Hier wordt ucg geinitialiseerd en worden er al direct dingen getekent met de ucg. 
  // Deze functie is gebaseerd op tft_display_ucg van Caspar Treijtel uit 2023.
  ucg_t ucg;
  ucg_init(&ucg);
  ucg_SetFontMode(&ucg, UCG_FONT_MODE_TRANSPARENT);
  ucg_ClearScreen(&ucg);
  ucg_SetFont(&ucg, ucg_font_8x13_mr);
  ucg_SetColor(&ucg, 0, 255, 255, 0);
  ucg_SetRotate90(&ucg);
  
  // Hier worden er kleuren gedefinieerd als C1, C2 en C3.
  color_t c1, c2, c3, c4;
  md_set_color(&c1, ball1.red, ball1.green, ball1.blue); // roze
  md_set_color(&c2, ball2.red, ball2.green, ball2.blue); // groen
  md_set_color(&c3, ball3.red, ball3.green, ball3.blue); // blauw
  md_set_color(&c4, ball4.red, ball4.green, ball4.blue); // Rood
  
  disc_t disc1, disc2, disc3, disc4;
  md_init_disc(&disc1, &ucg, 1, ball1.size, &c1);
  md_init_disc(&disc2, &ucg, 2, ball2.size, &c2);
  md_init_disc(&disc3, &ucg, 3, ball3.size, &c3);
  md_init_disc(&disc4, &ucg, 4, ball4.size, &c4);
  md_set_disc_position(&disc1, 25, 75);
  md_set_disc_position(&disc2, X_LINES, 95);
  md_set_disc_position(&disc3, 0, 115);
  md_set_disc_position(&disc4, 50, 50);

  sei();
  nrf_init();

while (1) { 
    
  
  if (rx_flag) {
    rx_flag = 0;

    // Hier wordt de ontvangen data van de NRF in uit de string gehaald. 
    // Dit gebeurd door de ontvangen string naar een buffer te kopieren. 
    // Daarna wordt deze string gesplitst om de x en y waardes op te kunnen slaan.
    strncpy(full_buffer, rx_packet, sizeof(full_buffer));

    strncpy(x_buffer, strtok(full_buffer, ","), sizeof(x_buffer));
    strncpy(y_buffer, strtok(NULL, ","), sizeof(y_buffer));

    accelarationG.x = atof(x_buffer);
    accelarationG.y = atof(y_buffer);

  }
    
    // Hier worden alle ballen verplaatst gebaseerd op de versnelling die gemeten is door de master.
    md_move_disc(&disc1, ball1.weight*accelarationG.y, ball1.weight*accelarationG.x);
    md_move_disc(&disc2, ball2.weight*accelarationG.y, ball2.weight*accelarationG.x);
    md_move_disc(&disc3, ball3.weight*accelarationG.y, ball3.weight*accelarationG.x);
    md_move_disc(&disc4, ball4.weight*accelarationG.y, ball4.weight*accelarationG.x);
  }
}

// Dit is een interupt die aan gaat wanneer de NRF module op de Xmega een signaal ontvangt.
// Deze functie is gebaseerd op het NRF master-slave voorbeeld van Caspar Treijtel.
ISR(NRF24_IRQ_VEC)
{
  uint8_t tx_ds, max_rt, rx_dr;
  uint8_t packet_length;

  nrfWhatHappened(&tx_ds, &max_rt, &rx_dr);

  if (rx_dr) {
    packet_length = nrfGetDynamicPayloadSize();
    nrfRead(rx_packet, packet_length);
    rx_packet[packet_length] = '\0';
    rx_flag = 1;
  }
}
/*!
 * \file    balls.h
 * \author  Rob Beaufort 
 * \brief   Hier wordt een struct gedefinieerd voor alle eigenschappen van de ballen.
 * \version 1.0
 * \date    6-4-2025
 */

#ifndef BALLS_H_
#define BALLS_H_

#include <stdint.h>

typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t weight;
    uint8_t size;
  } ball;
  
  extern ball ball1, ball2, ball3, ball4;
  
#endif
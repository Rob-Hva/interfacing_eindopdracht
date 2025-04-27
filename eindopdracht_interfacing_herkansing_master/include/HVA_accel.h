/* file    HVA_accel.h
 * author  Menno Leemans, Rob Beaufort, Mats van den Bolt
 * brief   In dit bestand worden alle gegevens van de HVA accelerometer gedefinieerd.
 * version 0.1
 * date    2025-3-17
 */

//Hier wordt het Mode register gedefinieerd. Deze wordt gebruikt om de accelerometer van mode te veranderen.
#define ACC_MODE 0x07
#define ACC_WAKE 0b00000001

//Hier worden het lees adres, schrijf adres en ID van de accelerometer gedefinieerd. ACC=accelerometer.
#define ACC_ID 0x4C
#define ACC_READ 0x99
#define ACC_WRITE 0x98

//Hier worden alle registers van de accelerometer gedefineerd.
#define XOUT_EX_L 0x0D
#define XOUT_EX_H 0x0E
#define YOUT_EX_L 0x0F
#define YOUT_EX_H 0x10 
#define ZOUT_EX_L 0x11 
#define ZOUT_EX_H 0x12

//Hier worden de verschillende meetswaarden per bit gedefinieerd.
//ValpBit staat voor Value per Bit.
#define ValpBit_G2 0.000061
#define ValpBit_G4 0.000122
#define ValpBit_G8 0.000244
#define ValpBit_G12 0.000366
#define ValpBit_G16 0.000488
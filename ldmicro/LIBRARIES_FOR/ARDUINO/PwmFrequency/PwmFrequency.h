/*
 * https://playground.arduino.cc/Code/PwmFrequency
 *
 * Here are some usage examples of the function:
 * Set pin 9's PWM frequency to 3906 Hz (31250/8 = 3906)
 * Note that the base frequency for pins 3, 9, 10, and 11 is 31250 Hz
 *   setPwmFrequency(9, 8);
 *
 * Set pin 6's PWM frequency to 62500 Hz (62500/1 = 62500)
 * Note that the base frequency for pins 5 and 6 is 62500 Hz
 *   setPwmFrequency(6, 1);
 *
 * Set pin 10's PWM frequency to 31 Hz (31250/1024 = 31)
 *   setPwmFrequency(10, 1024);
 *
 *
 * Divides a given PWM pin frequency by a divisor.
 *
 * The resulting frequency is equal to the base frequency divided by
 * the given divisor:
 *   - Base frequencies:
 *     The base frequency for pins 3, 9, 10, and 11 is 31250 Hz.
 *     The base frequency for pins 5 and 6          is 62500 Hz.
 *   - Divisors:
 *     The divisors available on pins 5, 6, 9 and 10 are: 1, 8, 64,
 *     256, and 1024.
 *     The divisors available on pins 3 and 11 are: 1, 8, 32, 64,
 *     128, 256, and 1024.
 *
 * PWM frequencies are tied together in pairs of pins. If one in a
 * pair is changed, the other is also changed to match:
 *   - Pins 5 and 6 are paired on Timer0
 *   - Pins 9 and 10 are paired on Timer1
 *   - Pins 3 and 11 are paired on Timer2
 *
 * Note that this function will have side effects on anything else
 * that uses timers:
 *   - Changes on pins 3, 5, 6, or 11 may cause the delay() and
 *     millis() functions to stop working. Other timing-related
 *     functions may also be affected.
 *   - Changes on pins 9 or 10 will cause the Servo library to function
 *     incorrectly.
 *
 * Please keep in mind that changing the PWM frequency changes the Atmega's
 * timers and disrupts the normal operation of many functions that rely
 * on time (delay(), millis(), Servo library).
 *
 * Thanks to macegr of the Arduino forums for his documentation of the
 * PWM frequency divisors. His post can be viewed at:
 *   http://forum.arduino.cc/index.php?topic=16612#msg121031
 */
#ifndef __PwmFrequency_H__
#define __PwmFrequency_H__

#include "Arduino.h"

extern boolean setPwmFrequency(int pin, int divisor);

#endif


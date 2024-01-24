/* tempsensor.c
Bla bla
contains functions for the temperature sensor */
#include <stdint.h>
#include <pic32mx.h>
#include "mipslab.h"
#include <stdio.h>
#include <stdbool.h>

#define TEMP_ADDR 0x48

#define TEMP_REG 0x0
#define TEMP_CONF 0x1
#define TEMP_HYST 0x2
#define TEMP_LIM 0x3


char buf[1024];
int avg = 0;
int avgDecimal = 0;
int numOfTemps = 0;
float tempSum = 0;
int maxTempInt = 0;
int maxTempDecimal = 0;
int minTempInt = 100;
int minTempDecimal = 100;


int getSwitch(void)
{
  return ((PORTD >> 8) & 0xf);
}

/* Waits for the TRSTAT bit to be 0
--> Master transmit not in progress */
void tempWait()
{
  while((I2C1CON & 0x1F || I2C1STAT & PIC32_I2CSTAT_TRSTAT));
}

/* Sends one byte of data on the SDA
returns status on the send (ack = true, nack = false) */
void tempSend(uint8_t byte)
{
  tempWait();
  I2C1TRN = byte;
  tempWait();
}

bool tempWaitAck()
{
    return !(I2C1STAT & PIC32_I2CSTAT_ACKSTAT);
}

/* Recives one byte of data on the SDA */
int8_t tempRecvMSB()
{
  tempWait();
  I2C1CONSET = PIC32_I2CCON_RCEN;
  tempWait();
  I2C1STATCLR = PIC32_I2CSTAT_I2COV;
  return I2C1RCV;
}

uint8_t tempRecvLSB()
{
  tempWait();
  I2C1CONSET = PIC32_I2CCON_RCEN;
  tempWait();
  I2C1STATCLR = PIC32_I2CSTAT_I2COV;
  return I2C1RCV;
}

/* Send ACK on the SDA */
void tempAck()
{
  tempWait();
  I2C1CONCLR = PIC32_I2CCON_ACKDT;
  I2C1CONSET = PIC32_I2CCON_ACKEN;
}

/* Send NACK on the SDA */
void tempNack()
{
  tempWait();
  I2C1CONSET = PIC32_I2CCON_ACKDT;
  I2C1CONSET = PIC32_I2CCON_ACKEN;
}


void tempStart()
{
  tempWait();
  I2C1CONSET = PIC32_I2CCON_SEN;
  tempWait();
}

void tempRestart()
{
  tempWait();
  I2C1CONSET = PIC32_I2CCON_RSEN;
  tempWait();
}


void tempStop()
{
  tempWait();
  I2C1CONSET = PIC32_I2CCON_PEN;
  tempWait();
}


void tempInit(void)
{
  I2C1CON = 0x0;
  I2C1BRG = 0x0C2;
  I2C1STAT = 0x0;
  I2C1CONSET = PIC32_I2CCON_SIDL;
  I2C1CONSET = PIC32_I2CCON_ON;

  /* Begin by setting SEN bit to start event. */
  tempStart(); // SEN bit set = start event

  /* Writing to TRN starts a master transmission.
  Data sent is the tempsensor address with lsb = 0 indicating write */
  tempSend(TEMP_ADDR << 1);
  while(!tempWaitAck());

  /* Access the config register */
  tempSend(TEMP_CONF);
  while(!tempWaitAck());

  /* Clear the config register */
  tempSend(0x20);
  while(!tempWaitAck());
  /* Stop transmission */
  tempStop();

}

void work()
{
  int8_t tempMSB;
  uint8_t tempLSB;
  /* Begin by setting SEN bit to start event. */
  tempStart();

  /* Send the register containing the temperature
  lsb = 0, indicating a write */
  tempSend(TEMP_ADDR << 1);
  while(!tempWaitAck());
  tempSend(TEMP_REG);
  while(!tempWaitAck());

  /* Send restart condition */
  tempRestart();
  /* Send address with lsb = 1, indicating read */
  tempSend((TEMP_ADDR << 1) | 1);
  while(!tempWaitAck());

  /* Recive the temperature then send ack so next byte can be read */
  tempMSB = tempRecvMSB();
  tempAck();
  tempLSB = tempRecvLSB();

  /* Send nack and stop to end transmission */
  tempNack();
  tempStop();

  char decimalPart[32];
  char maxTempStr[32];
  char maxTempDecimalStr[32];
  char minTempStr[32];
  char minTempDecimalStr[32];
  char avgTempStr[32];
  char avgDecimalStr[32];
  int decimal = 0;

  int16_t t = tempMSB;

  /* If the 7th bit of the decimal number is 1, add 50 to decimal part */
  if (tempLSB & 0x80)
  {
    decimal += 50;
  }

  /* If the 6th bit of the decimal number is 1, add 25 to decimal part */
  if (tempLSB & 0x40)
  {
    decimal += 25;
  }

  if (t > maxTempInt | (t == maxTempInt & decimal > maxTempDecimal)){
    maxTempInt = t;
    maxTempDecimal = decimal;
  }

  if (t < minTempInt | (t == minTempInt & decimal < minTempDecimal)){
    minTempInt = t;
    minTempDecimal = decimal;
  }

  tempSum += (t*100) + decimal;
  numOfTemps++;
  avg = tempSum/(numOfTemps*100);
  avgDecimal = ((tempSum)/numOfTemps) - avg*100;

  if (getSwitch() & 0x1)
  {
    if (avgDecimal + 15 >= 100)
    {
      avg += 1;
      avgDecimal -= 100;

    }
    sprintf(buf, "CURTEMP:%d.", t + 273);
    sprintf(decimalPart, "%02d K", decimal + 15);

    sprintf(maxTempStr, "MAXTEMP:%d.", maxTempInt + 273);
    sprintf(maxTempDecimalStr, "%02d K", maxTempDecimal + 15);

    sprintf(minTempStr, "MINTEMP:%d.", minTempInt + 273);
    sprintf(minTempDecimalStr, "%02d K", minTempDecimal + 15);

    sprintf(avgTempStr, "AVGTEMP:%d.", avg + 273);
    sprintf(avgDecimalStr, "%02d K", avgDecimal + 15);
  }
  else if (getSwitch() & 0x2){
    int displayint, displaydec;


    displayint = (t * 18)/10 + 32;
    displaydec = (decimal * 18)/10;
    if (displaydec >= 100)
    {
      displayint += 1;
      displaydec -= 100;

    }
    sprintf(buf, "CURTEMP:%d.", displayint);
    sprintf(decimalPart, "%02d F", displaydec);

    displayint = (maxTempInt * 18)/10 + 32;
    displaydec = (maxTempDecimal * 18)/10;
    if (displaydec >= 100)
    {
      displayint += 1;
      displaydec -= 100;

    }
    sprintf(maxTempStr, "MAXTEMP:%d.", displayint);
    sprintf(maxTempDecimalStr, "%02d F", displaydec);

    displayint = (minTempInt * 18)/10 + 32;
    displaydec = (minTempDecimal * 18)/10;
    if (displaydec >= 100)
    {
      displayint += 1;
      displaydec -= 100;
    }
    sprintf(minTempStr, "MINTEMP:%d.", displayint);
    sprintf(minTempDecimalStr, "%02d F", displaydec);

    displayint = (avg * 18)/10 + 32;
    displaydec = (avgDecimal * 18)/10;
    if (displaydec >= 100)
    {
      displayint += 1;
      displaydec -= 100;
    }
    sprintf(avgTempStr, "AVGTEMP:%d.", displayint);
    sprintf(avgDecimalStr, "%02d F", displaydec);

  } else {
    /* Convert to string */
    sprintf(buf, "CURTEMP:%d.", t);
    sprintf(decimalPart, "%02d C", decimal);

    sprintf(maxTempStr, "MAXTEMP:%d.", maxTempInt);
    sprintf(maxTempDecimalStr, "%02d C", maxTempDecimal);

    sprintf(minTempStr, "MINTEMP:%d.", minTempInt);
    sprintf(minTempDecimalStr, "%02d C", minTempDecimal);

    sprintf(avgTempStr, "AVGTEMP:%d.", avg);
    sprintf(avgDecimalStr, "%02d C", avgDecimal);
  }

  /* Concat the decimalpart to the temperature */
  strcat(buf, decimalPart);
  strcat(maxTempStr, maxTempDecimalStr);
  strcat(minTempStr, minTempDecimalStr);
  strcat(avgTempStr, avgDecimalStr);
  display_string(0, maxTempStr);
  display_string(1, minTempStr);
  display_string(2, avgTempStr);
  display_string(3, buf);
  display_update();




}

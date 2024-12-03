/**
 ******************************************************************************
 * @file      detection.c
 * @author    STMicroelectronics
 * @brief     Implement event detection actions
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */

/* Includes */
#include "main.h"
#include "detection.h"
#include "stdio.h"
#include "string.h"

// TODO : uncomment to enable encryption
#define OUTPUT_ENC_DATA

#undef OUTPUT_ENC_DATA

#ifdef OUTPUT_ENC_DATA
#include "crypto.h"
#endif

extern RTC_HandleTypeDef hrtc;
uint32_t  g_EventDetected = 0;
RTC_DateTypeDef sDate;
RTC_TimeTypeDef sTime;

#define OUT_BUFFER_SIZE (4 * 16)
char OutBuffer[OUT_BUFFER_SIZE];

void HAL_RTCEx_TimeStampEventCallback(RTC_HandleTypeDef *RTC_Handle)
{
  if ( g_EventDetected == 0 )
  {
    HAL_RTCEx_GetTimeStamp(&hrtc, &sTime, &sDate, RTC_FORMAT_BIN);
    g_EventDetected = 1;
  }
}

char *BuildOutputEvent(void);

char *BuildOutputEvent(void)
{
	memset(OutBuffer, 0, OUT_BUFFER_SIZE);
	sprintf(OutBuffer, "Detect button\r\nTime : %.2d:%.2d:%.2d\n\rDate : %.2d-%.2d-%.2d\n\r",
			 sTime.Hours, sTime.Minutes, sTime.Seconds,sDate.Month, sDate.Date, 2023);
	return OutBuffer;
}


static void OutputEventInClear(char *buffer);
static void OutputEventInClear(char *buffer)
{
#ifndef OUTPUT_ENC_DATA
	  printf("\n\r********************Event detected********************\n\r");
	  printf(buffer);
#endif
}
#ifdef OUTPUT_ENC_DATA
static void Encrypt_data(uint8_t *input, size_t inSize, uint8_t *output, size_t *outSize);
static void Encrypt_data(uint8_t *input, size_t inSize, uint8_t *output, size_t *outSize)
{
	  psa_status_t psa_status;
	  size_t size = 0;

	  /* Encrypt single part functions */
	  psa_status = psa_cipher_encrypt(0x23, PSA_ALG_CBC_NO_PADDING, (uint8_t const*) input,
			  	  	  	  	  	  inSize,
								  output,
	                              *outSize ,
	                              &size);
	  if (psa_status != PSA_SUCCESS)
	  {
	    printf("Error encrypting with the single-shot API\n\r");
	  }
	  else
	  {
		  if (size != *outSize)
		  {
			  printf("Issue out size ...\r\n");
		  }
		  *outSize=size;
	  }
}

#define ENC_BUF_SIZE OUT_BUFFER_SIZE + PSA_BLOCK_CIPHER_BLOCK_LENGTH(PSA_KEY_TYPE_AES)

static void OutputEventInEncrypted(char *buffer);
static void OutputEventInEncrypted(char *buffer)
{
uint8_t encBuffer[ENC_BUF_SIZE] = {0};
size_t encBufferSize=ENC_BUF_SIZE;

	Encrypt_data((uint8_t *)buffer, OUT_BUFFER_SIZE, encBuffer, &encBufferSize);
	printf("\n\r********************Event detected (enc) ********************\n\r");
	for(uint32_t i=0; i<encBufferSize;i++)
	{
		printf("%02x", encBuffer[i]);
	}
	printf("\r\n");
}
#endif

void ProcessEventDetection()
{
	if (g_EventDetected)
	{
		char *event = BuildOutputEvent();
		OutputEventInClear(event);
#ifdef OUTPUT_ENC_DATA
		OutputEventInEncrypted(event);
#endif
		g_EventDetected=0;
	}
}


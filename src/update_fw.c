/**
 ******************************************************************************
 * @file      update_fw.c
 * @author    STMicroelectronics
 * @brief     Implement update management
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
#include "update_fw.h"
#include "ymodem.h"
#include "stdio.h"


extern UART_HandleTypeDef huart1;

#define IMAGE_ID(slot,type) (((uint8_t)slot) | (type << 8));


static uint32_t DownloadNewFirmware(psa_image_id_t imageId);
static uint32_t RequestFirmwareInstall(psa_image_id_t imageId);

#define NUMBER_OF_STRING 6
#define MAX_STRING_SIZE 26

char psa_image_state[NUMBER_OF_STRING][MAX_STRING_SIZE] =
{
	"PSA_IMAGE_UNDEFINED",
	"PSA_IMAGE_CANDIDATE",
	"PSA_IMAGE_INSTALLED",
	"PSA_IMAGE_REJECTED",
	"PSA_IMAGE_PENDING_INSTALL",
	"PSA_IMAGE_REBOOT_NEEDED"
};



HAL_StatusTypeDef Ymodem_Packet_Receive(uint8_t *pData, uint32_t uOffset, uint32_t uSize, psa_image_id_t imageId)
{
  psa_status_t status;
    /* Write the data block with block_offset 0. */
  status = psa_fwu_write(imageId,uOffset, pData, uSize);
  if (status != PSA_SUCCESS)
  {
    (void)printf("psa_fwu_write() failure");
    return HAL_ERROR;
  }
  return HAL_OK;
}

#define IMAGE_SIZE_MAX 0xA0000

static uint32_t DownloadNewFirmware(psa_image_id_t imageId)
{

  COM_StatusTypeDef resDownload;
  uint32_t ret=0;
  uint32_t fw_size;

  // Set fimware maximum size (depends on secure manager configuration
  fw_size = IMAGE_SIZE_MAX;



  // Request download of sign binary
  printf("Please send binary application using  File> Transfer> YMODEM> Send\r\n");


  /*Init of Ymodem*/
  Ymodem_Init();
  /*Receive through Ymodem*/
  resDownload = Ymodem_Receive(&fw_size, imageId);
  printf("\r\n");

  if ((resDownload == COM_OK))
  {
    printf("Firmware download successful\r\n");
    printf("Received bytes: %ld\r\n\n", fw_size);
    ret=1;
  }
  else if (resDownload == COM_ABORT)
  {
    printf("Download aborted by user!!\r\n");
    __HAL_UART_FLUSH_DRREGISTER(&huart1);
  }
  else
  {
    printf("Error during file download : %d!!\r\n\n", resDownload);
    HAL_Delay(500U);
    __HAL_UART_FLUSH_DRREGISTER(&huart1);
  }

  return ret;
}

static uint32_t RequestFirmwareInstall(psa_image_id_t imageId)
{
	uint32_t ret=0;
	psa_image_id_t dependency_uuid = {0};
	psa_image_version_t dependency_version = {0};

	psa_status_t status = psa_fwu_install(imageId, &dependency_uuid, &dependency_version);

	if (status == PSA_SUCCESS_REBOOT)
	{
		printf("Install successful\r\n");
		ret = 1;
	}
	else
	{
		printf("Error install : %ld  \r\n", status);
	}

	return ret;
}

static uint8_t GetImageState(uint32_t id)
{
psa_image_id_t image_id;
psa_image_info_t info  = { 0 };
psa_status_t res;

	image_id = IMAGE_ID(id, FWU_IMAGE_TYPE_NONSECURE);

	res=psa_fwu_query(image_id, &info);
	if (res != PSA_SUCCESS)
	{
		printf("\r\nError psa_fwu_query : %ld\r\n", res);
		return 0;
	}
	return info.state;
}
static void DownloadAndInstallNewFirmware(void)
{
psa_image_id_t image_id;
psa_status_t res;
uint32_t result;

	// Build non secure application image ID
	image_id = IMAGE_ID(PSA_FWU_SLOT_ID_DL, FWU_IMAGE_TYPE_NONSECURE);

	printf("Active slot state before download : %s \r\n", psa_image_state[GetImageState(PSA_FWU_SLOT_ID_ACTIVE)]);
	printf("Download slot state before download : %s \r\n", psa_image_state[GetImageState(PSA_FWU_SLOT_ID_DL)]);

	// Manage FW download through Ymodem
	result=DownloadNewFirmware(image_id);

	if(!result)
	{
		return;
	}

	printf("Active slot state after download : %s \r\n", psa_image_state[GetImageState(PSA_FWU_SLOT_ID_ACTIVE)]);
	printf("Download slot state after download : %s \r\n", psa_image_state[GetImageState(PSA_FWU_SLOT_ID_DL)]);

	printf("Request firmware installation\r\n");
	// Request installation of this new firmware
	result=RequestFirmwareInstall(image_id);

	if(!result)
	{
		return;
	}

	printf("Active slot state after install request : %s \r\n", psa_image_state[GetImageState(PSA_FWU_SLOT_ID_ACTIVE)]);
	printf("Download slot state after install request : %s \r\n", psa_image_state[GetImageState(PSA_FWU_SLOT_ID_DL)]);

	/* Check the image state. */
	if (GetImageState(PSA_FWU_SLOT_ID_DL) != (uint8_t)PSA_IMAGE_REBOOT_NEEDED)
	{
		printf("\rWrong Image state\r\n");
	}

	printf("Request reboot ...\r\n");
	res=psa_fwu_request_reboot();
	if (res != PSA_SUCCESS)
	{
		printf("Error psa_fwu_request_reboot : %ld\r\n", res);
		return;
	}
}

void CheckUpdateRequest(void)
{
	uint8_t cmd;
	static uint32_t FirstCall=1;
	if (FirstCall)
	{
		FirstCall=0;
		printf("Active slot state after boot : %s \r\n", psa_image_state[GetImageState(PSA_FWU_SLOT_ID_ACTIVE)]);
		printf("Download slot state after boot : %s \r\n", psa_image_state[GetImageState(PSA_FWU_SLOT_ID_DL)]);
		if (GetImageState(PSA_FWU_SLOT_ID_ACTIVE) == (uint8_t)PSA_IMAGE_PENDING_INSTALL)
		{
			psa_image_id_t image_id;
			psa_status_t res;

			image_id = IMAGE_ID(PSA_FWU_SLOT_ID_ACTIVE, FWU_IMAGE_TYPE_NONSECURE);
			res=psa_fwu_accept(image_id);
			if (res != PSA_SUCCESS)
			{
				printf("Error psa_fwu_accept : %ld\r\n", res);
			} else {
				printf("Active slot validated : %s \r\n", psa_image_state[GetImageState(PSA_FWU_SLOT_ID_ACTIVE)]);
			}
			printf("\r*************************************\r\n");

		}
		printf("Please press key '1' to trigger a firmware update\r\n");
	}

	if ( HAL_UART_Receive(&huart1, (uint8_t *)&cmd, 1, 100) == HAL_OK)
	{
	  switch (cmd)
	  {
		case '1':
		  printf("\n\r***************Firmware update triggered *****************\n\r");
		  DownloadAndInstallNewFirmware();
		  break;
	  default:
		break;

	  }
	}
}

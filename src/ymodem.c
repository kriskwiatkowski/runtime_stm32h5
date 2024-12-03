/**
  ******************************************************************************
  * @file    ymodem.c
  * @author  MCD Application Team
  * @brief   Ymodem module.
  *          This file provides set of firmware functions to manage Ymodem
  *          functionalities.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/** @addtogroup USER_APP User App Example
  * @{
  */

/** @addtogroup  FW_UPDATE Firmware Update Example
  * @{
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32h5xx_hal.h"


#include "ymodem.h"
#include "string.h"
#include "main.h"
#include "stdio.h"

/* Private const -------------------------------------------------------------*/
const char BACK_SLASH_POINT[] = "\b.";
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* @note ATTENTION - please keep this variable 32bit aligned */
static uint32_t m_uFileSizeYmodem = 0U;    /* !< Ymodem File size*/
static uint32_t m_uNbrBlocksYmodem = 0U;   /* !< Ymodem Number of blocks*/
static uint32_t m_uPacketsReceived = 0U;   /* !< Ymodem packets received*/
static uint8_t m_aPacketData[PACKET_1K_SIZE + PACKET_DATA_INDEX +
                             PACKET_TRAILER_SIZE]; /*!<Array used to store Packet Data*/
uint8_t m_aFileName[FILE_NAME_LENGTH + 1U]; /*!< Array used to store File Name data */
static CRC_HandleTypeDef CrcHandle; /*!<CRC handle*/

/* Private function prototypes -----------------------------------------------*/
static HAL_StatusTypeDef ReceivePacket(uint8_t *pData, uint32_t *puLength, uint32_t uTimeout);

/** @addtogroup USER_APP User App Example
  * @{
  */

/** @addtogroup USER_APP_COMMON Common
  * @{
  */

/** @defgroup  COM_Private_Defines Private Defines
  * @{
  */
const char YMODEM_IT_MSG[] = "\r\nYmodem stop by printf\r\n";
#define BAUDRATE 115200U
/**
  * @}
  */

/** @defgroup  COM_Private_Variables Exported Variables
  * @{
  */
extern UART_HandleTypeDef huart1;
#define UartHandle huart1

static __IO int32_t Ymodem = 0;              /*!< set to 1 when Ymodem uses UART> */
static uint8_t Abort;
/**
  * @}
  */

/** @defgroup  COM_Exported_Functions Exported Functions
  * @{
  */

/** @defgroup  COM_Initialization_Functions Initialization Functions
  * @{
  */


/**
  * @}
  */

/** @defgroup  COM_Control_Functions Control Functions
  * @{
  */
uint32_t Str2Int(uint8_t *pInputStr, uint32_t *pIntNum)
{
  uint32_t i = 0U;
  uint32_t res = 0U;
  uint32_t val = 0U;

  if ((pInputStr[0U] == (uint8_t)'0') && ((pInputStr[1U] == (uint8_t)'x') || (pInputStr[1U] == (uint8_t)'X')))
  {
    i = 2U;
    while ((i < 11U) && (pInputStr[i] != (uint8_t)'\0'))
    {
      if (ISVALIDHEX((char)pInputStr[i]))
      {
        val = (val << 4U) + CONVERTHEX((char)pInputStr[i]);
      }
      else
      {
        /* Return 0, Invalid input */
        res = 0U;
        break;
      }
      i++;
    }

    /* valid result */
    if (pInputStr[i] == (uint8_t)'\0')
    {
      *pIntNum = val;
      res = 1U;
    }
  }
  else /* max 10-digit decimal input */
  {
    while ((i < 11U) && (res != 1U))
    {
      if (pInputStr[i] == (uint8_t)'\0')
      {
        *pIntNum = val;
        /* return 1 */
        res = 1U;
      }
      else if (((pInputStr[i] == (uint8_t)'k') || (pInputStr[i] == (uint8_t)'K')) && (i > 0U))
      {
        val = val << 10U;
        *pIntNum = val;
        res = 1U;
      }
      else if (((pInputStr[i] == (uint8_t)'m') || (pInputStr[i] == (uint8_t)'M')) && (i > 0U))
      {
        val = val << 20U;
        *pIntNum = val;
        res = 1U;
      }
      else if (ISVALIDDEC((char)pInputStr[i]))
      {
        val = (val * 10U) + (uint32_t)CONVERTDEC((uint8_t)pInputStr[i]);
      }
      else
      {
        /* return 0, Invalid input */
        res = 0U;
        break;
      }
      i++;
    }
  }

  return res;
}


/**
  * @brief  Transmit a byte to the HyperTerminal
  * @param  param The byte to be sent
  * @retval HAL_StatusTypeDef HAL_OK if OK
  */
HAL_StatusTypeDef Serial_PutByte(uint8_t uParam)
{
  return COM_Transmit_Y(&uParam, 1U, TX_TIMEOUT);
}
/**
  * @brief Transmit Data.
  * @param uDataLength: Data pointer to the Data to transmit.
  * @param uTimeout: Timeout duration.
  * @retval Status of the Transmit operation.
  */
HAL_StatusTypeDef COM_Transmit(uint8_t *Data, uint16_t uDataLength, uint32_t uTimeout)
{
  HAL_StatusTypeDef status;
  uint32_t i;
  if (Ymodem)
  {
    /* send abort Y modem transfer sequence */
    for (i = 0; i < (uint32_t)5; i++)
    {
      status = HAL_UART_Transmit(&UartHandle, (uint8_t *)&Abort, 1, uTimeout);
      if (status != HAL_OK)
      {
        return status;
      }
    }
    status = HAL_UART_Transmit(&UartHandle, (uint8_t *)YMODEM_IT_MSG, (uint16_t)sizeof(YMODEM_IT_MSG), uTimeout);
    if (status != HAL_OK)
    {
      return status;
    }
    Ymodem = 0;
  }
  return HAL_UART_Transmit(&UartHandle, (uint8_t *)Data, uDataLength, uTimeout);
}

/**
  * @brief Transmit Data function for Y modem.
  * @param uDataLength: Data pointer to the Data to transmit.
  * @param uTimeout: Timeout duration.
  * @retval Status of the Transmit operation.
  */
HAL_StatusTypeDef COM_Transmit_Y(uint8_t *Data, uint16_t uDataLength, uint32_t uTimeout)
{
  if (Ymodem)
  {
    return HAL_UART_Transmit(&UartHandle, (uint8_t *)Data, uDataLength, uTimeout);
  }
  else
  {
    return HAL_ERROR;
  }
}


/**
  * @brief Receive Data.
  * @param uDataLength: Data pointer to the Data to receive.
  * @param uTimeout: Timeout duration.
  * @retval Status of the Receive operation.
  */
HAL_StatusTypeDef COM_Receive(uint8_t *Data, uint16_t uDataLength, uint32_t uTimeout)
{
  if (!Ymodem)
  {
    return HAL_UART_Receive(&UartHandle, (uint8_t *)Data, uDataLength, uTimeout);
  }
  else
  {
    return HAL_ERROR;
  }
}
/**
  * @brief Receive Data function Y Modem.
  * @param uDataLength: Data pointer to the Data to receive.
  * @param uTimeout: Timeout duration.
  * @retval Status of the Receive operation.
  */
HAL_StatusTypeDef COM_Receive_Y(uint8_t *Data, uint16_t uDataLength, uint32_t uTimeout)
{
  if (Ymodem)
  {
    return HAL_UART_Receive(&UartHandle, (uint8_t *)Data, uDataLength, uTimeout);
  }
  else
  {
    return HAL_BUSY;
  }
}

/**
  * @brief  Flush COM Input.
  * @param None.
  * @retval HAL_Status.
  */
HAL_StatusTypeDef COM_Flush(void)
{
  /* Clean the input path */
  __HAL_UART_FLUSH_DRREGISTER(&UartHandle);
  return HAL_OK;
}


HAL_StatusTypeDef  COM_Y_On(uint8_t AbortChar)
{
  HAL_StatusTypeDef status = HAL_ERROR;
  if (!Ymodem)
  {
    Ymodem = 1;
    Abort = AbortChar;
    status = HAL_OK;
  }
  return status;
}
HAL_StatusTypeDef  COM_Y_Off(void)
{
  HAL_StatusTypeDef status = HAL_ERROR;
  if (Ymodem)
  {
    Ymodem = 0;
    status = HAL_OK;
  }
  return status;
}
/**
  * @}
  */

/** @addtogroup  COM_Private_Functions
  * @{
  */

/**
  * @}
  */

/** @defgroup COM_Callback_Functions Callback Functions
  * @{
  */


HAL_StatusTypeDef Ymodem_HeaderPktRxCpltCallback(uint32_t uOffset, uint32_t uFileSize, psa_image_id_t imageId)
{
  UNUSED(uOffset);
  /*Reset of the ymodem variables */
  m_uFileSizeYmodem = 0U;
  m_uPacketsReceived = 0U;
  m_uNbrBlocksYmodem = 0U;

  /*Filesize information is stored*/
  m_uFileSizeYmodem = uFileSize;

  /* compute the number of 1K blocks */
  m_uNbrBlocksYmodem = (m_uFileSizeYmodem + (PACKET_1K_SIZE - 1U)) / PACKET_1K_SIZE;

  /* NOTE : delay inserted for Ymodem protocol*/
  HAL_Delay(1000);
  return HAL_OK;
}

extern uint32_t total_size_received;
/**
  * @brief  Ymodem Data Packet Transfer completed callback.
  * @param  pData Pointer to the buffer.
  * @param  uSize Packet dimension (Bytes).
  * @retval None
  */
HAL_StatusTypeDef Ymodem_DataPktRxCpltCallback(uint8_t *pData, uint32_t uOffset, uint32_t uSize, psa_image_id_t imageId)
{
  int32_t ret;
  uint32_t size = uSize;
  m_uPacketsReceived++;
  HAL_StatusTypeDef status;

  /*Increase the number of received packets*/
  if (m_uPacketsReceived == m_uNbrBlocksYmodem) /*Last Packet*/
  {
    /*Extracting actual payload from last packet*/
    if ((m_uFileSizeYmodem % PACKET_1K_SIZE) == 0U)
    {
      /* The last packet must be fully considered */
      size = PACKET_1K_SIZE;
    }
    else
    {
      /* The last packet is not full, drop the extra bytes */
      size = m_uFileSizeYmodem - ((uint32_t)(m_uFileSizeYmodem / PACKET_1K_SIZE) * PACKET_1K_SIZE);
    }

    m_uPacketsReceived = 0U;
  }

  
  /* Write the data block with block_offset 0. */
  status = Ymodem_Packet_Receive(pData, uOffset,  size, imageId);
  if (status != HAL_OK)
  {
    printf("psa_fwu_write() failure");
    return HAL_ERROR;
  }
  else
  {
    ret = 0;
  }
  if (ret != 0)
  {
    /*Reset of the ymodem variables */
    m_uFileSizeYmodem = 0U;
    m_uPacketsReceived = 0U;
    m_uNbrBlocksYmodem = 0U;
    return HAL_ERROR;
  }
  else
  {
    return HAL_OK;
  }
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Receive a packet from sender
  * @param  pData
  * @param  puLength
  *     0: end of transmission
  *     2: abort by sender
  *    >0: packet length
  * @param  uTimeout
  * @retval HAL_OK: normally return
  *         HAL_BUSY: abort by user
  */
static HAL_StatusTypeDef ReceivePacket(uint8_t *pData, uint32_t *puLength, uint32_t uTimeout)
{
  uint32_t crc;
  uint32_t packet_size = 0U;
  HAL_StatusTypeDef status;
  uint8_t char1;
  uint8_t char2;
  uint8_t char3;

  *puLength = 0U;

  /* If the SecureBoot configured the IWDG, UserApp must reload IWDG counter with
   *value defined in the reload register */
  status = (HAL_StatusTypeDef)COM_Receive_Y(&char1, 1, uTimeout);

  if (status == HAL_OK)
  {
    switch (char1)
    {
      /* Type of receive packet */
      case SOH:
        packet_size = PACKET_SIZE;
        break;
      case STX:
        packet_size = PACKET_1K_SIZE;
        break;
      case EOT:
        packet_size = 0;
        break;
      case CA:
        if ((COM_Receive_Y(&char1, 1U, uTimeout) == HAL_OK) && (char1 == CA))
        {
          packet_size = 2U;
        }
        else
        {
          status = HAL_ERROR;
        }
        break;
      case ABORT1:
      case ABORT2:
        status = HAL_BUSY;
        break;
      case RB:
        /* Ymodem startup sequence : rb ==> 0x72 + 0x62 + 0x0D */
        if ((COM_Receive_Y(&char2, 1U, uTimeout) == HAL_OK) &&
            (COM_Receive_Y(&char3, 1U, uTimeout) == HAL_OK) &&
            (char2 == 0x62U) &&
            (char3 == 0xdU))
        {
          packet_size = 3U;
          break;
        }
      default:
        status = HAL_ERROR;
        break;
    }
    *pData = char1;

    if (packet_size >= PACKET_SIZE)
    {
      status = COM_Receive_Y(&pData[PACKET_NUMBER_INDEX], packet_size + PACKET_OVERHEAD_SIZE, uTimeout);

      /* Simple packet sanity check */
      if (status == HAL_OK)
      {
        if (pData[PACKET_NUMBER_INDEX] != ((pData[PACKET_CNUMBER_INDEX]) ^ NEGATIVE_BYTE))
        {
          packet_size = 0U;
          status = HAL_ERROR;
        }
        else
        {
          /* Check packet CRC */
          crc = (uint32_t)pData[ packet_size + PACKET_DATA_INDEX ] << 8U;
          crc += pData[ packet_size + PACKET_DATA_INDEX + 1U ];
          if (HAL_CRC_Calculate(&CrcHandle, (uint32_t *)&pData[PACKET_DATA_INDEX], packet_size) != crc)
          {
            packet_size = 0U;
            status = HAL_ERROR;
          }
        }
      }
      else
      {
        packet_size = 0U;
      }
    }
  }
  *puLength = packet_size;
  return status;
}


/**
  * @brief  Init of Ymodem module.
  * @param None.
  * @retval None.
  */
void Ymodem_Init(void)
{
  __HAL_RCC_CRC_CLK_ENABLE();

  /*-1- Configure the CRC peripheral */
  CrcHandle.Instance = CRC;
  /* The CRC-16-CCIT polynomial is used */
  CrcHandle.Init.DefaultPolynomialUse    = DEFAULT_POLYNOMIAL_DISABLE;
  CrcHandle.Init.GeneratingPolynomial    = 0x1021U;
  CrcHandle.Init.CRCLength               = CRC_POLYLENGTH_16B;
  /* The zero init value is used */
  CrcHandle.Init.DefaultInitValueUse     = DEFAULT_INIT_VALUE_DISABLE;
  CrcHandle.Init.InitValue               = 0U;
  /* The input data are not inverted */
  CrcHandle.Init.InputDataInversionMode  = CRC_INPUTDATA_INVERSION_NONE;
  /* The output data are not inverted */
  CrcHandle.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
  /* The input data are 32-bit long words */
  CrcHandle.InputDataFormat              = CRC_INPUTDATA_FORMAT_BYTES;

  if (HAL_CRC_Init(&CrcHandle) != HAL_OK)
  {
    /* Initialization Error */
    while (1) {};
  }
  if (COM_Y_On(CA) != HAL_OK)
  {
    while (1) {};
  }
}

/**
  * @brief  Receive a file using the ymodem protocol with CRC16.
  * @param  puSize The uSize of the file.
  * @param  uFlashDestination where the file has to be downloaded.
  * @retval COM_StatusTypeDef result of reception/programming
  */
COM_StatusTypeDef Ymodem_Receive(uint32_t *puSize, psa_image_id_t imageId)
{
  uint32_t i;
  uint32_t packet_length;
  uint32_t session_done = 0U;
  uint32_t file_done;
  uint32_t errors = 0U;
  uint32_t session_begin = 0U;
  uint32_t eot = 0U;
  uint32_t ramsource;
  uint32_t filesize = 0U;
  uint32_t offset = 0U;
  uint8_t *file_ptr;
  uint8_t file_size[FILE_SIZE_LENGTH + 1U];
  uint8_t tmp;
  uint32_t packets_received;
  COM_StatusTypeDef e_result = COM_OK;
  uint32_t cause = 0;
  while ((session_done == 0U) && (e_result == COM_OK))
  {
    packets_received = 0U;
    file_done = 0U;
    while ((file_done == 0U) && (e_result == COM_OK))
    {
      switch (ReceivePacket(m_aPacketData, &packet_length, DOWNLOAD_TIMEOUT))
      {
        case HAL_OK:
          errors = 0U;
          switch (packet_length)
          {
            case 3U:
              /* Startup sequence */
              break;
            case 2U:
              /* Abort by sender : transmit acknowledge signal*/
              (void)Serial_PutByte(ACK);
              e_result = COM_ABORT;
              break;
            case 0U:
              /* End of transmission */
              if (!eot)
              {
                (void)Serial_PutByte(NAK);
                eot = 1;
              }
              else
              {
                /* Send acknowledge signal */
                (void)Serial_PutByte(ACK);
                *puSize = filesize;
                file_done = 1U;
              }
              break;
            default:
              /* Normal packet */
              if (m_aPacketData[PACKET_NUMBER_INDEX] != (packets_received & 0xffU))
              {
                /*             Serial_PutByte(NAK);*/
              }
              else
              {
                if (packets_received == 0U)
                {
                  /* File name packet */
                  if (m_aPacketData[PACKET_DATA_INDEX] != 0U)
                  {
                    /* File name extraction */
                    i = 0U;
                    file_ptr = m_aPacketData + PACKET_DATA_INDEX;
                    while ((*file_ptr != 0U) && (i < FILE_NAME_LENGTH))
                    {
                      m_aFileName[i++] = *file_ptr++;
                    }
                    /* File size extraction */
                    m_aFileName[i++] = '\0';
                    i = 0U;
                    file_ptr ++;
                    while ((*file_ptr != ' ') && (i < FILE_SIZE_LENGTH))
                    {
                      file_size[i++] = *file_ptr++;
                    }
                    file_size[i++] = '\0';
                    (void)Str2Int(file_size, &filesize);
                    if ((uint32_t)filesize > *puSize)
                    {
                      *puSize = 0;
                      tmp = CA;
                      e_result = COM_ABORT;
                      cause = 1;
                    }
                    /* Header packet received callback call*/
                    if ((*puSize) && (Ymodem_HeaderPktRxCpltCallback(offset, (uint32_t) filesize, imageId) == HAL_OK))
                    {
                      (void)Serial_PutByte(ACK);
                      (void)COM_Flush();
                      (void)Serial_PutByte(CRC16);
                    }
                    else
                    {
                      /* End session */
                      (void)COM_Transmit_Y(&tmp, 1U, NAK_TIMEOUT);
                      (void)COM_Transmit_Y(&tmp, 1U, NAK_TIMEOUT);
                      cause = 2;
                      break;
                    }
                  }
                  /* File header packet is empty, end session */
                  else
                  {
                    (void)Serial_PutByte(ACK);
                    file_done = 1;
                    session_done = 1;
                    cause = 3;
                    break;
                  }
                }
                else /* Data packet */
                {
                  ramsource = (uint32_t) & m_aPacketData[PACKET_DATA_INDEX];
                  /* Data packet received callback call*/
                  if ((*puSize) && (Ymodem_DataPktRxCpltCallback((uint8_t *)ramsource, \
                                                                  offset, (uint32_t)packet_length, imageId) == HAL_OK))
                  {
                    offset += (packet_length);
                    (void)Serial_PutByte(ACK);
                  }
                  else /* An error occurred while writing to Flash memory */
                  {
                    /* End session */
                    (void)COM_Transmit_Y(&tmp, 1U, NAK_TIMEOUT);
                    (void)COM_Transmit_Y(&tmp, 1U, NAK_TIMEOUT);
                    cause = 4;
                  }
                }
                packets_received ++;
                session_begin = 1U;
              }
              break;
          }
          break;
        case HAL_BUSY:
          /* Abort actually */
          (void)Serial_PutByte(CA);
          (void)Serial_PutByte(CA);
          e_result = COM_ABORT;
          cause  = 5;
          break;
        default:
          if (session_begin > 0U)
          {
            errors ++;
          }
          if (errors > MAX_ERRORS)
          {
            /* Abort communication */
            (void)Serial_PutByte(CA);
            (void)Serial_PutByte(CA);
          }
          else
          {
            /* Ask for a packet */
            (void)Serial_PutByte(CRC16);
            /* Replace C char by . on display console */
            (void)COM_Transmit_Y((uint8_t *)BACK_SLASH_POINT, sizeof(BACK_SLASH_POINT) - 1U, TX_TIMEOUT);
          }
          break;
      }
    }
  }
  /* Turned off the ymodem */
  (void)COM_Y_Off();
#if defined(__ARMCC_VERSION)
  printf("e_result = %x , %u\n", e_result, cause);
#else
  printf("e_result = %x , %lu\n", e_result, cause);
#endif /* __ARMCC_VERSION */
  return e_result;
}

/**
  * @}
  */

/**
  * @}
  */

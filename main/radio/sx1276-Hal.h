/*
 * THE FOLLOWING FIRMWARE IS PROVIDED: (1) "AS IS" WITH NO WARRANTY; AND 
 * (2)TO ENABLE ACCESS TO CODING INFORMATION TO GUIDE AND FACILITATE CUSTOMER.
 * CONSEQUENTLY, SEMTECH SHALL NOT BE HELD LIABLE FOR ANY DIRECT, INDIRECT OR
 * CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE CONTENT
 * OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING INFORMATION
 * CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
 * 
 * Copyright (C) SEMTECH S.A.
 */
/*! 
 * \file       sx1276-Hal.h
 * \brief      SX1276 Hardware Abstraction Layer
 *
 * \version    2.0.B2 
 * \date       May 6 2013
 * \author     Gregory Cristian
 *
 * Last modified by Miguel Luis on Jun 19 2013
 */
#ifndef __SX1276_HAL_H__
#define __SX1276_HAL_H__

//#include "ioe.h"
/*!
 * SX1276 RESET I/O definitions
 */

#define RESET_IOPORT                                GPIOB
#define RESET_PIN                                   GPIO_PIN_6

/*!
 * SX1276 SPI NSS I/O definitions
 */

#define NSS_IOPORT                                  GPIOB
#define NSS_PIN                                     GPIO_PIN_7

/*!
 * SX1276 DIO pins  I/O definitions
 */

#define DIO0_IOPORT                                 GPIOA
#define DIO0_PIN                                    GPIO_PIN_1

#define DIO1_IOPORT                                 GPIOB
#define DIO1_PIN                                    GPIO_PIN_0

#define DIO2_IOPORT                                 GPIOB
#define DIO2_PIN                                    GPIO_PIN_1

#define DIO3_IOPORT                                 GPIOA
#define DIO3_PIN                                    GPIO_PIN_0


#define DIO4_IOPORT                                 GPIOB
#define DIO4_PIN                                    GPIO_PIN_2


#define DIO5_IOPORT                                 GPIOA
#define DIO5_PIN                                    GPIO_PIN_4


#define RXTX_IOPORT                                 
#define RXTX_PIN                                    FEM_CTX_PIN

   
#define RST_SET_HIGH()                             RESET_IOPORT->BSRR=RESET_PIN
#define RST_SET_LOW()                              RESET_IOPORT->BRR=RESET_PIN

   
   
#define NSS_SET_HIGH()                              NSS_IOPORT->BSRR=NSS_PIN
#define NSS_SET_LOW()                               NSS_IOPORT->BRR=NSS_PIN

/*!
 * DIO state read functions mapping
 */
#define DIO0                                        dio0_read_state()
#define DIO1                                        0//(READ_BIT(DIO1_IOPORT->IDR, DIO0_PIN) == (DIO1_PIN))//SX1276ReadDio1( )
#define DIO2                                        0//(READ_BIT(DIO2_IOPORT->IDR, DIO0_PIN) == (DIO2_PIN))//SX1276ReadDio2( )
#define DIO3                                        0//(READ_BIT(DIO3_IOPORT->IDR, DIO0_PIN) == (DIO3_PIN))//SX1276ReadDio3( )
#define DIO4                                        0//(READ_BIT(DIO4_IOPORT->IDR, DIO0_PIN) == (DIO4_PIN))//SX1276ReadDio4( )
#define DIO5                                        0//(READ_BIT(DIO5_IOPORT->IDR, DIO0_PIN) == (DIO5_PIN))//SX1276ReadDio5( )

// RXTX pin control see errata note
#define RXTX( txEnable )                            SX1276WriteRxTx( txEnable );

extern volatile uint32_t TickCounter;

#define LoRa_SPI_ExchangeByte SPI1_ExchangeByte

#define GET_TICK_COUNT( )                           ( TickCounter )
#define TICK_RATE_MS( ms )                          ( ms )

typedef enum
{
    RADIO_RESET_OFF,
    RADIO_RESET_ON,
}tRadioResetState;

/*!
 * \brief Initializes the radio interface I/Os
 */
void SX1276InitIo( void );

/*!
 * \brief Set the radio reset pin state
 * 
 * \param state New reset pin state
 */
void SX1276SetReset( uint8_t state );

/*!
 * \brief Writes the radio register at the specified address
 *
 * \param [IN]: addr Register address
 * \param [IN]: data New register value
 */
void SX1276Write( uint8_t addr, uint8_t data );

/*!
 * \brief Reads the radio register at the specified address
 *
 * \param [IN]: addr Register address
 * \param [OUT]: data Register value
 */
void SX1276Read( uint8_t addr, uint8_t *data );

/*!
 * \brief Writes multiple radio registers starting at address
 *
 * \param [IN] addr   First Radio register address
 * \param [IN] buffer Buffer containing the new register's values
 * \param [IN] size   Number of registers to be written
 */
void SX1276WriteBuffer( uint8_t addr, uint8_t *buffer, uint8_t size );

/*!
 * \brief Reads multiple radio registers starting at address
 *
 * \param [IN] addr First Radio register address
 * \param [OUT] buffer Buffer where to copy the registers data
 * \param [IN] size Number of registers to be read
 */
void SX1276ReadBuffer( uint8_t addr, uint8_t *buffer, uint8_t size );

/*!
 * \brief Writes the buffer contents to the radio FIFO
 *
 * \param [IN] buffer Buffer containing data to be put on the FIFO.
 * \param [IN] size Number of bytes to be written to the FIFO
 */
void SX1276WriteFifo( uint8_t *buffer, uint8_t size );

/*!
 * \brief Reads the contents of the radio FIFO
 *
 * \param [OUT] buffer Buffer where to copy the FIFO read data.
 * \param [IN] size Number of bytes to be read from the FIFO
 */
void SX1276ReadFifo( uint8_t *buffer, uint8_t size );

/*!
 * \brief Gets the SX1276 DIO0 hardware pin status
 *
 * \retval status Current hardware pin status [1, 0]
 */
uint8_t SX1276ReadDio0( void );

/*!
 * \brief Gets the SX1276 DIO1 hardware pin status
 *
 * \retval status Current hardware pin status [1, 0]
 */
uint8_t SX1276ReadDio1( void );

/*!
 * \brief Gets the SX1276 DIO2 hardware pin status
 *
 * \retval status Current hardware pin status [1, 0]
 */
uint8_t SX1276ReadDio2( void );

/*!
 * \brief Gets the SX1276 DIO3 hardware pin status
 *
 * \retval status Current hardware pin status [1, 0]
 */
uint8_t SX1276ReadDio3( void );

/*!
 * \brief Gets the SX1276 DIO4 hardware pin status
 *
 * \retval status Current hardware pin status [1, 0]
 */
uint8_t SX1276ReadDio4( void );

/*!
 * \brief Gets the SX1276 DIO5 hardware pin status
 *
 * \retval status Current hardware pin status [1, 0]
 */
uint8_t SX1276ReadDio5( void );

/*!
 * \brief Writes the external RxTx pin value
 *
 * \remark see errata note
 *
 * \param [IN] txEnable [1: Tx, 0: Rx]
 */
void SX1276WriteRxTx( uint8_t txEnable );

#endif //__SX1276_HAL_H__

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
 * \file       sx1276-Hal.c
 * \brief      SX1276 Hardware Abstraction Layer
 *
 * \version    2.0.B2 
 * \date       Nov 21 2012
 * \author     Miguel Luis
 *
 * Last modified by Miguel Luis on Jun 19 2013
 */
#include <stdint.h>
#include <stdbool.h> 

#include "../gpio.h"

#include "platform.h"

#include "sx1276-Hal.h"
#include "../spidev.h"

void SX1276InitIo( void )
{
return ;
}

void SX1276SetReset( uint8_t state )
{
    if( state == RADIO_RESET_ON )
    {
        // Set RESET pin to 0
         //RESET_IOPORT->BRR=RESET_PIN;
        // Configure RESET as output
        set_lora_reset_pin_high();

    }
    else
    {
    
         set_lora_reset_pin_low();
         //RESET_IOPORT->BSRR=RESET_PIN;

    }
    return ;
}

void SX1276Write( uint8_t addr, uint8_t data )
{
    //SX1276WriteBuffer( addr, &data, 1 );

	SpiWriteRegister (addr, data);
}

void SX1276Read( uint8_t addr, uint8_t *data )
{
    //SX1276ReadBuffer( addr, data, 1 );
    *data=SpiReadRegister(addr);

}


void SX1276WriteBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

    //NSS = 0;
//    NSS_SET_LOW();
//    LoRa_SPI_ExchangeByte( addr | 0x80 );
//    for( i = 0; i < size; i++ )
//    {
//        LoRa_SPI_ExchangeByte( buffer[i] );
//    }
//
//    //NSS = 1;
//    NSS_SET_HIGH();
    SPIWriteBuffer(addr, buffer,size );
}

void SX1276ReadBuffer( uint8_t addr, uint8_t *buffer, uint8_t size )
{
    uint8_t i;

//    //NSS = 0;
//    NSS_SET_LOW();
//
//    LoRa_SPI_ExchangeByte( addr & 0x7F );
//
//    for( i = 0; i < size; i++ )
//    {
//        buffer[i] = LoRa_SPI_ExchangeByte( 0 );
//    }
//
//    //NSS = 1;
//    NSS_SET_HIGH();
    SPIReadBuffer(addr, buffer,size );
}

void SX1276WriteFifo( uint8_t *buffer, uint8_t size )
{
    SX1276WriteBuffer( 0, buffer, size );
}

void SX1276ReadFifo( uint8_t *buffer, uint8_t size )
{
    SX1276ReadBuffer( 0, buffer, size );
}

uint8_t SX1276ReadDio0( void )
{
    return dio0_read_state();
}

uint8_t SX1276ReadDio1( void )
{
    return 0;
}

uint8_t SX1276ReadDio2( void )
{
    return 0;
}

uint8_t SX1276ReadDio3( void )
{
	 return 0;
}

uint8_t SX1276ReadDio4( void )
{
	 return 0;
}

uint8_t SX1276ReadDio5( void )
{
	 return 0;
}


void SX1276WriteRxTx( uint8_t txEnable )
{
//    if( txEnable != 0 )
//    {
//        IoePinOn( FEM_CTX_PIN );
//        IoePinOff( FEM_CPS_PIN );
//    }
//    else
//    {
//        IoePinOff( FEM_CTX_PIN );
//        IoePinOn( FEM_CPS_PIN );
//    }
}




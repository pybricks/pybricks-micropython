/**
*  \file ninja/spi.c
*    
*  \brief This contains function definitions required to interact with the SPI0 controller of the SoC which is connected to the ADC.
* 
*  \author Tobias Schießl, ev3ninja
*/

/* Include statements */
#include "spi.h"
#include "gpio.h"
#include "pininfo.h"
#include "systick.h"

/* Macro defintions */
/**
* \brief The clock rate at which the SPI0 controller should run
**/ 
#define   SPI0_CLOCK  150000000UL
/**
* \brief The delay between 2 ADC convertions 
**/ 
#define   ADC_TIME    8UL // in µS
/**
* \brief The clock rate at which the ADC runs
**/ 
#define   ADC_CLOCK   ((1000000UL * 16UL) / ADC_TIME)
/**
* \brief The maximum convertion speed
**/ 
#define   CNVSPD      ((SPI0_CLOCK / ADC_CLOCK) - 1)
/* SPI0 GPIO pins */
/**
* \brief The offset of the first GPIO pin which can provide SPI functionality
**/ 
#define   SPI0_OFFSET (GPIO_PIN(9, 0) + 5)
/**
* \brief The GPIO pin which provided the Master-Output-Slave-Input functionality for SPI0
**/ 
#define   SPI0_MOSI   (SPI0_OFFSET + 0)
/**
* \brief The GPIO pin which provided the Master-Input-Slave-Output functionality for SPI0
**/ 
#define   SPI0_MISO   (SPI0_OFFSET + 1)
/**
* \brief The GPIO pin which provided the Clock functionality for SPI0
**/ 
#define   SPI0_SCL    (SPI0_OFFSET + 2)
/**
* \brief The GPIO pin which provided the Chip Select functionality for SPI0
**/ 
#define   SPI0_CS     (SPI0_OFFSET + 3)
/* SPI Register addresses */
/**
* \brief The base address of the SPI0 control register
**/ 
#define   SPI_BASE    ((volatile void*)0x01C41000)
/**
* \brief The SPI0 global configuration register 0
**/ 
#define   SPIGCR0     (*((volatile unsigned int*)(SPI_BASE + 0x00)))
/**
* \brief The SPI0 global configuration register 1
**/ 
#define   SPIGCR1     (*((volatile unsigned int*)(SPI_BASE + 0x04)))
/**
* \brief The SPI0 interrupt register
**/ 
#define   SPIINT0     (*((volatile unsigned int*)(SPI_BASE + 0x08)))
/**
* \brief The SPI0 pin control register 0
**/ 
#define   SPIPC0      (*((volatile unsigned int*)(SPI_BASE + 0x14)))
/**
* \brief The SPI0 data transmit register 0
**/ 
#define   SPIDAT0     (*((volatile unsigned int*)(SPI_BASE + 0x38)))
/**
* \brief The SPI0 data transmit register 1
**/ 
#define   SPIDAT1     (*((volatile unsigned int*)(SPI_BASE + 0x3C)))
/**
* \brief The SPI0 receive buffer register
**/ 
#define   SPIBUF      (*((volatile unsigned int*)(SPI_BASE + 0x40)))
/**
* \brief The SPI0 delay register
**/ 
#define   SPIDELAY    (*((volatile unsigned int*)(SPI_BASE + 0x48)))
/**
* \brief The SPI0 default chip select register
**/ 
#define   SPIDEF      (*((volatile unsigned int*)(SPI_BASE + 0x4C)))
/**
* \brief The SPI0 data format register 0
**/ 
#define   SPIFMT0     (*((volatile unsigned int*)(SPI_BASE + 0x50)))
/* Macros to check if data has already been sent or received */
/**
* \brief Check if the transmit data buffer is full
**/ 
#define   SPITxFULL   (SPIBUF & 0x20000000)
/**
* \brief Check if the receive data buffer is empty
**/ 
#define   SPIRxEMPTY  (SPIBUF & 0x80000000)

/**
* \brief Send data via the SPI0 controller and get an updated value received by the SPI0 controller
* 
* Since the SPI0 controller is configured for chip select 3 only, all data sent and received will be to or from the ADC.
* According to the documentation of the ADC, the selected channel (0 to 15) will be probed and returned on frame n+2. Therefore, the command is sent 3 times and only the third value
* received is returned to the caller of this function.
*
* \param data - The data to send (since this is data to the ADC, check the ADC documentation for possible commands that can be sent)
*
* \return The value received from the ADC after sending the data 3 times
**/
unsigned short spi_update (unsigned short data) {
    // TODO: Maybe there is a better approach than this? (according to the ADC documentation page 30, the selected channel is red in frame n+2 - therefore we get the correct value 2 requests later)
    unsigned short dataRet = 0;
    for (int i = 0; i < 3; ++i) {
        while (SPITxFULL);
        SPIDAT0 = (unsigned long)data;
        while (SPIRxEMPTY);
        dataRet = ((unsigned short)(SPIBUF & 0x0000FFFF));
    }
    return dataRet;
}

/**
* \brief Initialize the SPI0 controller and the required GPIO pins
* 
* For the GPIO pins, the SPI functionality will be set instead of the GPIO functionality.
*
* \return none
**/
void spi_init (void) {
	gpio_init_pin(SPI0_MOSI); // ADCMOSI
	gpio_init_pin(SPI0_MISO); // ADCMISO
	gpio_init_pin(SPI0_SCL);  // ADCCLK
	gpio_init_pin(SPI0_CS);   // ADCCS

	SPIGCR0  = 0x00000001;    // enable
	SPIGCR1  = 0x00000003;    // Master enable
	SPIPC0   = 0x00000E08;
	SPIDAT1  = 0x00000000;    // Format 0 is selected
	SPIFMT0  = 0x00010010 | ((CNVSPD << 8) & 0xFF00);
	SPIDELAY = 0x0A0A0A0A;    // Delays = 10
	SPIINT0  = 0x00000000;    // Interrupts disabled
	SPIDEF   = 0x00000008;
	SPIGCR1  = 0x01000003;    // Enable bit
}

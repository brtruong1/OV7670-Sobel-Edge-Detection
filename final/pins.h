#include "../driverlib/gpio.h"


#ifndef __PINS_H
#define __PINS_H

// Camera Interface
#define GPIO_Pin_All               ((uint8_t)0xFF)  //!< All pins selected 

#define RRST    	GPIO_PIN_0 			// PORT A

#define WEN     	GPIO_PIN_7 			// PORT M

#define VSYNC   	GPIO_PIN_4 			// PORT P	Input
#define RCLK    	GPIO_PIN_5 			// PORT P

#define RED_LED   	GPIO_PIN_0			// PORT N
#define GREEN_LED 	GPIO_PIN_1			// PORT N

#define GPIO_DATA	GPIO_Pin_All		// PORT K Input Camera

#define VSYNC_PORT 		GPIO_PORTC_BASE
#define WEN_PORT		GPIO_PORTM_BASE
#define RRST_PORT		GPIO_PORTA_BASE
#define RCLK_PORT 		GPIO_PORTP_BASE
#define DATA_PORT 		GPIO_PORTK_BASE



// Leds
#define LEDS_PORT		GPIO_PORTN_BASE

// I2C Interface
#define PERIPH_I2C		SYSCTL_PERIPH_I2C2
#define	PER_PORT_I2C	SYSCTL_PERIPH_GPION
#define	PORT_I2C		GPIO_PORTN_BASE
#define I2C_UNIT		I2C2_BASE
#define GPIO_I2CSCL		GPIO_PN5_I2C2SCL
#define GPIO_I2CSDA		GPIO_PN4_I2C2SDA


						//
#define SCL     		GPIO_PIN_5 			// PORT N	I2C
#define SDA     		GPIO_PIN_4 			// PORT N	Input	I2C2

#endif

// vim: set sw=4 et: //
/*



void ConfigureBoard(void)
{
    // Enable and configure the GPIO port
    //ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA | SYSCTL_PERIPH_GPIOB | SYSCTL_PERIPH_GPIOC | SYSCTL_PERIPH_GPIOD | SYSCTL_PERIPH_GPIOE | SYSCTL_PERIPH_GPIOF);
	//ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);

	//ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C1);
    // PORT A
    ROM_GPIOPinTypeGPIOOutput(RRST_PORT, RRST);
    ROM_GPIOPinWrite(RRST_PORT, RRST, 0x00);
    GPIOPadConfigSet(RRST_PORT,  RRST, GPIO_STRENGTH_12MA, GPIO_PIN_TYPE_STD);

    // PORT P
    ROM_GPIOPinTypeGPIOInput(VSYNC_PORT, VSYNC);
    GPIOPadConfigSet(VSYNC_PORT,  VSYNC, GPIO_STRENGTH_12MA, GPIO_PIN_TYPE_STD);

    ROM_GPIOPinTypeGPIOOutput(RCLK_PORT, RCLK);
    ROM_GPIOPinWrite(RCLK_PORT, RCLK, 0x00);
    GPIOPadConfigSet(RCLK_PORT,  RCLK, GPIO_STRENGTH_12MA, GPIO_PIN_TYPE_STD);

    // PORT M
    ROM_GPIOPinTypeGPIOOutput(WEN_PORT, WEN);    //he set outputs: rrst, rclk, wen, 
    ROM_GPIOPinWrite(WEN_PORT, WEN, 0x00);       //he set inputs: 
    GPIOPadConfigSet(WEN_PORT, WEN, GPIO_STRENGTH_12MA , GPIO_PIN_TYPE_STD);

    // PORT K
    ROM_GPIOPinTypeGPIOInput(DATA_PORT, GPIO_DATA);
    GPIOPadConfigSet(DATA_PORT, GPIO_DATA, GPIO_STRENGTH_12MA , GPIO_PIN_TYPE_STD);

    // PORT N
    //GPIOPinTypeGPIOOutput(LEDS_PORT, RED_LED|GREEN_LED);
    //GPIOPinWrite(LEDS_PORT, RED_LED|GREEN_LED, 0x00);
    //GPIOPadConfigSet(LEDS_PORT,  RED_LED|GREEN_LED, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);
}


void ConfigurePWM(void)
{
//	unsigned long ulPeriod;

	//Configure PWM Clock to match system
//	SysCtlPWMClockSet(SYSCTL_PWMDIV_1);

    //Enable the peripherals used by this program.
//	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);

//	ulPeriod = SysCtlClockGet() / 1200; //PWM frequency
	//Configure PWM Options
//	GPIOPinConfigure(GPIO_PF2_M1PWM6);
//	GPIOPinTypePWM(GPIO_PORTF_BASE, GPIO_PIN_2);

	//Configure PWM Options
	//PWM_GEN_2 Covers M1PWM4 and M1PWM5
	//PWM_GEN_3 Covers M1PWM6 and M1PWM7 See page 207 4/11/13 DriverLib doc
//	PWMGenConfigure(PWM1_BASE, PWM_GEN_3, PWM_GEN_MODE_UP_DOWN | PWM_GEN_MODE_NO_SYNC);

	//Set the Period (expressed in clock ticks)
//	PWMGenPeriodSet(PWM1_BASE, PWM_GEN_3, ulPeriod);

	//Set PWM duty-50% (Period /2)
//	PWMPulseWidthSet(PWM1_BASE, PWM_OUT_6,ulPeriod/2);

	// Enable the PWM generator
//	PWMGenEnable(PWM1_BASE, PWM_GEN_3);

    // Turn on the Output pins
//    PWMOutputState(PWM1_BASE, PWM_OUT_6_BIT, true);
}
//*****************************************************************************
//
// Configure the UART and its pins for the camera
//
//*****************************************************************************



*/
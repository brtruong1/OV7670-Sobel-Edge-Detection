#include <stdint.h>
	#include "../inc/BSP.h"
	#include "../inc/Profile.h"
	#include "Texas.h"
	#include "../inc/CortexM.h"
	//#include "os.h"
	#include "FreeRTOS.h"
	#include "task.h"
	#include <math.h>
	//#include "queue.h"
	#include "semphr.h"
	//#include "timers.h"

	#include <stdbool.h>
	#include "../inc/hw_memmap.h"
	#include "../driverlib/sysctl.h"
	#include "../driverlib/i2c.h"
	#include "../driverlib/debug.h"
	#include "../driverlib/gpio.h"
	#include "../driverlib/pin_map.h"
	#include "ov7670reg.h"
	#include "../inc/hw_types.h"
	#include "../inc/hw_gpio.h"
	#include "../driverlib/pwm.h"

	uint8_t slaveAddr = 0x42;
	void I2C0_Init(void);
	void ov7670_set(uint8_t memAddr, uint8_t data);
	uint8_t ov7670_get(uint8_t memAddr);
	uint8_t ov7670_init(void);

	void CreateImage();
	void DisplayImage();
	void EdgeDetection();
	void dummy(void){};
	void CameraPins_Init(void);
	void ov7670_capture(void);
	void ov7670_rrst(void);
	void ov7670_image(void);
	void ov7670_wrst(void);
		
	//array for image
	uint8_t image[128][128];
	//uint8_t edgeImage[80][50];
	int width = 128;
	int height = 128;
    //convolution arrays for the sobel edge detection
	int gx[3][3] = {
					{-1, 0, 1},
					{-2, 0, 2},
					{-1, 0, 1}
			};
	int gy[3][3] = {
					{-1, -2, -1},
					{0, 0, 0},
					{1, 2, 1}
			};

//Create buttton states to toggle between regular image and edge detected
enum State{
	Regular,
	Edge
};
//task for capture, button, and display edge/normal based on buttonState

void ImageCaptureTask(void *arguments);
void ButtonTask(void *arguments); 
void DisplayTask(void *arguments);
enum State buttonState = Regular;
uint8_t current;
static uint8_t prev2 = 0;
TaskHandle_t buttonTaskHandle = NULL;
SemaphoreHandle_t frameReady = NULL; //mut ex


void ImageCaptureTask(void *arguments){
    frameReady = xSemaphoreCreateMutex();               //Create muxtex sema; function sets initial value to 1
    while(1){
        xSemaphoreTake(frameReady, (TickType_t) 10);    //take semaphore
        //modifying critical reasource
        vTaskPrioritySet(NULL, 3);                      //Set own priority high to not get interrupted
        ov7670_wrst();                                  //resets write pointer of the camera's fifo
        ov7670_capture();                               //writes pixels to the camera's fifo
        ov7670_rrst();                                  //resets the read pointer of the camera's fifo
        ov7670_image();                                 //writes the pixel data from the fifo to the image array
        xSemaphoreGive(frameReady);                     //Gives sempahore once the full frame is complete
        vTaskPrioritySet(NULL, 1);                      //Set priority of ImageCaptureTask back to 1
        vTaskPrioritySet(buttonTaskHandle, 3);          //Set the priority of ButtonTask high
    }
}


void ButtonTask(void *arguments){
    while(1){
        current = BSP_Button2_Input();                  //Recieve the button input from user
	    if ((current == 0) && (prev2 != 0)){            //this section ensures that the state only changes once per press
		    if (buttonState == Regular){ 
		    	buttonState = Edge;
		    }
	    else if (buttonState == Edge) {
		    buttonState = Regular;
		    }
        }
        prev2 = current;
        vTaskPrioritySet(NULL, 1);                      //Set priority of ButtonTask back to 1
    }
}

void DisplayTask(void *arguments){
    if(frameReady != NULL){                                     
        while(1){
            if (buttonState == Regular){
                xSemaphoreTake(frameReady, (TickType_t) 10);        //taking semaphore
                vTaskPrioritySet(NULL, 3);                          //Set own priority high to not get interrupted
                DisplayImage();                                     //Display normal image
                xSemaphoreGive(frameReady);                         //Give semaphore back
                vTaskPrioritySet(NULL, 1);                          //Revert priority level
                vTaskPrioritySet(buttonTaskHandle, 3);              //Set the priority of ButtonTask high
            }
            else if (buttonState == Edge){
                xSemaphoreTake(frameReady, (TickType_t) 10);
                vTaskPrioritySet(NULL, 3);
                EdgeDetection();                                    //Display edge detected image
                xSemaphoreGive(frameReady);
                vTaskPrioritySet(NULL, 1);
                vTaskPrioritySet(buttonTaskHandle, 3);
            }
        }   
    }
}


int main(void){
	
    //Task 0 is initializing everything
	I2C0_Init(); 
	BSP_Delay1ms(500);
	CameraPins_Init();
	BSP_Delay1ms(500);
	ov7670_init();
	BSP_Delay1ms(500);
	BSP_LCD_Init();
	BSP_Delay1ms(500);
    BSP_Button2_Init();
	BSP_LCD_FillScreen(BSP_LCD_Color565(0,0,0));



    //frameReady = xSemaphoreCreateMutex();
    //xSemaphoreTake( SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait );   OS_Wait
    //xSemaphoreGive( SemaphoreHandle_t xSemaphore );  OS_Signal             
    xTaskCreate(ImageCaptureTask, "Task1", 128, NULL, 1, NULL);
	xTaskCreate(ButtonTask, "Task2", 128, NULL, 1, &buttonTaskHandle);
	xTaskCreate(DisplayTask, "Task3", 128, NULL, 1, NULL);
    vTaskStartScheduler();  //start the FreeRTOS roundrobin scheduling for the 3 tasks
}

	//display image array
	void DisplayImage(void) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint8_t gray = image[x][y];
            BSP_LCD_DrawPixel(x, y, BSP_LCD_Color565(gray, gray, gray));
        }
    }
}
    //sobel edge detection algorithm adapted from
    //https://medium.com/@twinnroshan/understanding-and-implementing-edge-detection-in-c-with-sobel-operator-31159f26587c
	void EdgeDetection() {
			float sumx, sumy;
			uint8_t color;
			for (int i = 1; i < height - 1; i++) {
					for (int j = 1; j < width - 1; j++) {
							sumx = 0.0;
							sumy = 0.0;
							for (int p = -1; p <= 1; p++) {
									for (int q = -1; q <= 1; q++) {
											sumx += (image[i + p][j + q] * gx[p + 1][q + 1]);
											sumy += (image[i + p][j + q] * gy[p + 1][q + 1]);
									}
							}
							//edgeImage[i][j] = (unsigned char)sqrt(sumx * sumx + sumy * sumy);
							color = (unsigned char)sqrt(sumx * sumx + sumy * sumy);
							BSP_LCD_DrawPixel(i, j, BSP_LCD_Color565(color, color, color));
					}
			}
	}

///////////////////////////////////////////////////////////////////////////////////////////////
//this section is to setup the I2C and OV7670 Camera
	//
	// Function: I2C0_Init
	// Description: Initializes I2C0 module on TM4C123G for Master mode
    // Adapted from:
	// https://controllerstech.com/tivac-i2c-communication/
	void I2C0_Init(void)
	{
			// Enable I2C0 and GPIOB peripherals
			SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C0);
			SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

			// Wait until peripherals are ready
			while(!SysCtlPeripheralReady(SYSCTL_PERIPH_I2C0));
			while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOB));

			// Configure PB2 as I2C SCL and PB3 as I2C SDA
			//GPIOPinConfigure(GPIO_PB2_I2C0SCL);
			//GPIOPinConfigure(GPIO_PB3_I2C0SDA);
		    GPIOPinConfigure(0x00010803);
			GPIOPinConfigure(0x00010C03);

			GPIOPinTypeI2CSCL(GPIO_PORTB_BASE, GPIO_PIN_2);  // Configure PB2 -> SCL
			GPIOPinTypeI2C(GPIO_PORTB_BASE, GPIO_PIN_3);  // Configure PB3 -> SDA

			// Initialize I2C master with system clock
			// false = standard mode (100kHz), true = fast mode (400kHz)
			I2CMasterInitExpClk(I2C0_BASE, SysCtlClockGet(), false);

			// Clear any previous I2C status
			I2CMasterIntClear(I2C0_BASE);
	}
    void CameraPins_Init(void) {

			//turn on ports A, C, D, E, F . B is already turned on in I2c init?
			// from example: ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
			SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA); 
			// Wait until peripherals are ready
			while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOA));
			SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
			while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOC));
			SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
			while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOD));
			SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
			while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOE));
			SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
			while(!SysCtlPeripheralReady(SYSCTL_PERIPH_GPIOF));
			//for D0-D7 they are inputs
			GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_0);  //D0
			GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_1);  //D1
			GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_2);  //D2
			GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_3);  //D3
			GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_1);  //D4
			GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_2);  //D5
			GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_3);  //D6
			HWREG(GPIO_PORTF_BASE + GPIO_O_LOCK) = 0x4C4F434B;
			HWREG(GPIO_PORTF_BASE + GPIO_O_CR) |= 0x02;        // Allow changes to PF1
			GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_1);  //D7
			
			
			//fifo inputs   no OE? no WRST?
			//tie OE to ground? tie WRST to 3.3V

			//ROM_GPIOPinTypeGPIOInput(VSYNC_PORT, VSYNC);
			GPIOPinTypeGPIOInput(GPIO_PORTC_BASE, GPIO_PIN_4); //VSYNC
			GPIOPadConfigSet(GPIO_PORTC_BASE,  GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);

			//fifo outputs
			//ROM_GPIOPinTypeGPIOOutput(WEN_PORT, WEN);
			GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_5);  //WR/WEN
			GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_5, 0x00);
			GPIOPadConfigSet(GPIO_PORTC_BASE, GPIO_PIN_5, GPIO_STRENGTH_2MA , GPIO_PIN_TYPE_STD);

			//ROM_GPIOPinTypeGPIOOutput(RCLK_PORT, RCLK);
			GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_6);  //RCLK
			GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, 0x00);
			GPIOPadConfigSet(GPIO_PORTC_BASE,  GPIO_PIN_6, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);

			//ROM_GPIOPinTypeGPIOOutput(RRST_PORT, RRST);
			//ROM_GPIOPinWrite(RRST_PORT, RRST, 0x00);
			//GPIOPadConfigSet(RRST_PORT,  RRST, GPIO_STRENGTH_12MA, GPIO_PIN_TYPE_STD);
			GPIOPinTypeGPIOOutput(GPIO_PORTC_BASE, GPIO_PIN_7);  //rrst
			GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, 0x00);
			GPIOPadConfigSet(GPIO_PORTC_BASE,  GPIO_PIN_7, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);
			
			
			GPIOPinTypeGPIOOutput(GPIO_PORTD_BASE, GPIO_PIN_6);  // WRST
			GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_6, GPIO_PIN_6); // inactive high
			GPIOPadConfigSet(GPIO_PORTD_BASE, GPIO_PIN_6, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD);
			//non fifo inputs


			//non fifo outputs
			//xclk is output?
			//GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_4); //XCLK
	}

    //these 8 ov7670 functions were adapted from the github below
    //https://github.com/lemariva/tiva_tm4c1294_ov7670
	void ov7670_set(uint8_t memAddr, uint8_t data)
	{
			// Send slave address with WRITE bit (R/W = 0)
			//\param ui8SlaveAddr 7-bit slave address  in i2c.c
			I2CMasterSlaveAddrSet(I2C0_BASE, slaveAddr>>1, false);

			// Send high byte of memory address
			I2CMasterDataPut(I2C0_BASE, memAddr);
			I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_START);  // Start new Write
			SysCtlDelay(1000);
			while(I2CMasterBusy(I2C0_BASE));
				
			SysCtlDelay(1500);
			
			// Send the actual data byte
			I2CMasterDataPut(I2C0_BASE, data);
			I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_BURST_SEND_FINISH);  // Finish Write
			SysCtlDelay(1000);
			while(I2CMasterBusy(I2C0_BASE));
	}

	uint8_t ov7670_get(uint8_t memAddr)
	{
			uint8_t data;

			// Send slave address with WRITE bit (R/W = 0)
			//\param ui8SlaveAddr 7-bit slave address  in i2c.c
			I2CMasterSlaveAddrSet(I2C0_BASE, slaveAddr>>1, false);

			// Send high byte of memory address
			I2CMasterDataPut(I2C0_BASE, memAddr);
			I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_SEND);  // Start new Write
			SysCtlDelay(1000);
			while(I2CMasterBusy(I2C0_BASE));
			SysCtlDelay(1500);
			
			// Repeated START + Slave address with READ bit (R/W = 1)
			I2CMasterSlaveAddrSet(I2C0_BASE, (slaveAddr + 1)>>1, true);
			I2CMasterControl(I2C0_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);  // Read 1 Byte
		    SysCtlDelay(1000);
			while(I2CMasterBusy(I2C0_BASE));
			SysCtlDelay(1500);
			// Read the received byte
			data = (uint8_t)I2CMasterDataGet(I2C0_BASE);
			return data;
	}


	

	uint8_t ov7670_init(void)
	{
		uint8_t a;
		a = ov7670_get(REG_PID);
		//tprintf("  ID Received 0x%x \r\n",a);
			if (a != 0x76) {
					return 0;
			}

			ov7670_set(REG_COM7, 0x80); /* reset to default values */
			BSP_Delay1ms(100);
			ov7670_set(REG_CLKRC, 0x80);


			ov7670_set(REG_COM7, 0x00);  // YUV output
            ov7670_set(REG_TSLB, 0x04);  // YUYV order
            ov7670_set(REG_COM13, 0x88); // enable UV auto adjust, gamma-ish color matrix


			//ov7670_set(REG_RGB444, 0x00); /* disable RGB444 */
			//ov7670_set(REG_COM15, 0xD0); /* set RGB565 */



			/* not even sure what all these do, gonna check the oscilloscope and go
			 * from there... */
			ov7670_set(REG_HSTART, 0x16);
			ov7670_set(REG_HSTOP, 0x04);
			ov7670_set(REG_HREF, 0x24);
			ov7670_set(REG_VSTART, 0x02);
			ov7670_set(REG_VSTOP, 0x7a);
			ov7670_set(REG_VREF, 0x0a);
			ov7670_set(REG_COM10, 0x02);

		a = ov7670_get(REG_COM10);
		//tprintf("  COM10 Received 0x%x \r\n",a);
			if (a != 0x02) {
					return 0;
			}

			ov7670_set(REG_COM3, 0x04);
			ov7670_set(REG_MVFP, 0x3f);

			// 80 x 60  divide by 8
			ov7670_set(REG_COM3, 0x04);    // enable DCW
			//ov7670_set(REG_COM14, 0x1b); // divide by 8
			//ov7670_set(0x72, 0x33); // downsample by 8
			//ov7670_set(0x73, 0xf3); // divide by 8

			/* 160x120, i think */
			ov7670_set(REG_COM14, 0x1a); // divide by 4
			ov7670_set(0x72, 0x22); // downsample by 4
		  ov7670_set(0x73, 0xf2); // divide by 4

			/* 320x240: */ 
			//ov7670_set(REG_COM14, 0x19); //
			//ov7670_set(0x72, 0x11);
			//ov7670_set(0x73, 0xf1); //divide by 2
			//640x 480  //divide by 1
			// test pattern
		 // ov7670_set(0x70, 0xf0);
		 // ov7670_set(0x71, 0xf0);

			// COLOR SETTING
			ov7670_set(0x4f, 0x80);
			ov7670_set(0x50, 0x80);   
			ov7670_set(0x51, 0x00);
			ov7670_set(0x52, 0x22);
			ov7670_set(0x53, 0x5e);
			ov7670_set(0x54, 0x80);
			ov7670_set(0x56, 0x40);
			ov7670_set(0x58, 0x9e);
			ov7670_set(0x59, 0x88);
			ov7670_set(0x5a, 0x88);
			ov7670_set(0x5b, 0x44);
			ov7670_set(0x5c, 0x67);
			ov7670_set(0x5d, 0x49);
			ov7670_set(0x5e, 0x0e);    
			ov7670_set(0x69, 0x00);
			ov7670_set(0x6a, 0x40);
			ov7670_set(0x6b, 0x0a);
			ov7670_set(0x6c, 0x0a);
			ov7670_set(0x6d, 0x55);
			ov7670_set(0x6e, 0x11);
			ov7670_set(0x6f, 0x9f);     

			ov7670_set(0xb0, 0x84);

			return 1;
	}
	//GPIOPinTypeGPIOInput(GPIO_PORTD_BASE, GPIO_PIN_0);  //D0
	//GPIOPinTypeGPIOInput(GPIO_PORTC_BASE, GPIO_PIN_4); //VSYNC 
	//GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_1);  //D7
void ov7670_capture(void)
{
    
    // Capture frame 
	while (GPIOPinRead(GPIO_PORTC_BASE, GPIO_PIN_4));		// VSYNC wait for an old frame to end
	while (!GPIOPinRead(GPIO_PORTC_BASE, GPIO_PIN_4));	// wait for an old frame to start
	GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_5, GPIO_PIN_5);			//WEN enable writing to fifo
    //80 MHz so 12.5ns period
    //SysCtrlDelay is 3 clock cycles so 3  x 12.5ns = 37.5ns
    //run 4 since 37.5ns * 4 = 150ns
    SysCtlDelay(4);
	//wait(delay_120ns);
	while (GPIOPinRead(GPIO_PORTC_BASE, GPIO_PIN_4));	// VSYNC wait for the current frame to end
	GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_5, 0x00);		//WEN disable writing to fifo
    //run 1 since 37.5ns 
    SysCtlDelay(1);
	//wait(delay_25ns);
    
	GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, GPIO_PIN_7);   //rrst
	//while ((P1IN & VSYNC));     // wait for an old frame to end
    //while (!(P1IN & VSYNC));    // wait for a new frame to start
    //P1OUT |= WEN;               // enable writing to fifo
    //while ((P1IN & VSYNC));     // wait for the current frame to end
    //P1OUT &= ~WEN;              // disable writing to fifo
    //wait();
    //P1OUT |= RRST;
}

	// Reset the al422 read pointer 
	void ov7670_rrst(void)
	{
		// Since RRST (P1.3) has a debouncing cap, let's go really slowly
			 // here... 
		GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, 0x00);  //RRST
			//run 4 since 37.5ns * 4 = 150ns
			SysCtlDelay(10);
		//wait(delay_120ns);
		GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_PIN_6);  //RCLK
		SysCtlDelay(1);
		//wait(delay_25ns);
		GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_7, GPIO_PIN_7);  //RRST
		GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, 0x00); //RCLK
			// Since RRST (P1.3) has a debouncing cap, let's go really slowly
			 // here... 
			//P1OUT &= ~RRST;
			//__delay_cycles(500);
			//P1OUT |= RCLK;
			//__delay_cycles(100);
			//P1OUT |= RRST;
			//P1OUT &= ~RCLK;
	}

	void ov7670_wrst(void)
{
    // WRST is active low
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_6, 0x00);
    SysCtlDelay(10);

    // release write reset
    GPIOPinWrite(GPIO_PORTD_BASE, GPIO_PIN_6, GPIO_PIN_6);
    SysCtlDelay(10);
}
	
	
	// Read one byte from the al422 
uint8_t ov7670_read(void)
{
    uint8_t val = 0x00;

	GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, GPIO_PIN_6);  //RCLK
	SysCtlDelay(1);
	//wait(delay_25ns);
	//val = GPIOPinRead(DATA_PORT, GPIO_DATA);  //DATA D0-D7
    
    /*
    
    PD0: D0 
    PD1: D1
    PD2: D2
    PD3: D3
    PE1: D4
    PE2: D5
    PE3: D6
    PF1: D7
    */
    uint8_t pd0, pd1, pd2, pd3, pe1, pe2, pe3, pf1;
    //say in portd we have 0b10101111
    //GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_1) gives 0b00000010
    pd0 = GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_0); 
    pd1 = GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_1);
    pd2 = GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_2);
    pd3 = GPIOPinRead(GPIO_PORTD_BASE, GPIO_PIN_3);
    //val has 0b00001111
    // GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_1) gives 0b00000010
    //port e's start at bit 4 so shift that left 
    pe1 = GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_1) << 3;
    pe2 = GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_2) << 3;
    pe3 = GPIOPinRead(GPIO_PORTE_BASE, GPIO_PIN_3) << 3; 
    //GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_1) gives 0b00000010
    pf1 = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_1) << 6;
    val = pd0 | pd1 | pd2 | pd3 | pe1 | pe2 | pe3 | pf1;

    

	GPIOPinWrite(GPIO_PORTC_BASE, GPIO_PIN_6, 0x00);  //RCLK
	SysCtlDelay(10);
	//wait(delay_25ns);
   // wait(delay_25ns);
    //P1OUT |= RCLK;
    //val = P2IN;
    ////__delay_cycles(10);
    //P1OUT &= ~RCLK;
    //__delay_cycles(10);

    return val;
}



void ov7670_image(void)
{
    uint8_t b0, b1, b2, b3;
    for (int y = 0; y < 120; y++) {
        for (int x = 0; x < 160; x += 2) {

            b0 = ov7670_read();
            b1 = ov7670_read();
					
            b2 = ov7670_read();
					
            b3 = ov7670_read();
					
            if (x < width && y < height) {
                image[x][y]     = b0;
                image[x + 1][y] = b2;
            }
        }
    }
}



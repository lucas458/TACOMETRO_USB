/*
 * File:   main.c
 * Author: Lucas
 *
 * Created on 17 de Janeiro de 2025, 16:14
 * 
 * XC8 (v2.46)
 */


// CONFIG1
#pragma config FOSC		= INTOSC  // Oscillator Selection (INTOSC oscillator: I/O function on CLKIN pin)
#pragma config WDTE		= OFF     // Watchdog Timer Enable (WDT disabled)
#pragma config PWRTE	= ON      // Power-up Timer Enable (PWRT enabled)
#pragma config MCLRE	= OFF     // MCLR Pin Function Select (MCLR/VPP pin function is digital input)
#pragma config CP		= OFF     // Flash Program Memory Code Protection (Program memory code protection is disabled)
#pragma config CPD		= OFF     // Data Memory Code Protection (Data memory code protection is disabled)
#pragma config BOREN	= ON      // Brown-out Reset Enable (Brown-out Reset enabled)
#pragma config CLKOUTEN = OFF	  // Clock Out Enable (CLKOUT function is disabled. I/O or oscillator function on the CLKOUT pin)
#pragma config IESO		= ON      // Internal/External Switchover (Internal/External Switchover mode is enabled)
#pragma config FCMEN	= ON      // Fail-Safe Clock Monitor Enable (Fail-Safe Clock Monitor is enabled)

// CONFIG2
#pragma config WRT		= OFF     // Flash Memory Self-Write Protection (Write protection off)
#pragma config PLLEN	= ON      // PLL Enable (4x PLL enabled)
#pragma config STVREN	= ON      // Stack Overflow/Underflow Reset Enable (Stack Overflow or Underflow will cause a Reset)
#pragma config BORV		= LO      // Brown-out Reset Voltage Selection (Brown-out Reset Voltage (Vbor), low trip point selected.)
#pragma config LVP		= ON      // Low-Voltage Programming Enable (Low-voltage programming enabled)

// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.

#define _XTAL_FREQ	32000000
#include <xc.h>



volatile struct Buffer_s{
	uint8_t data[6];
	uint8_t index;
} Buffer;

/*
 data[0] = Display 0 Data
 data[1] = Display 1 Data
 data[2] = Display 2 Data
 data[3] = Amount of LEDs (0 to 20)
 data[4] = Brightness (0 to 15)
 data[5] = '\r' or '\n'
 */



const uint8_t initSequence[][2] = {
	{0xC, 0x1}, // Shutdown Register Format:		Normal Operation
	{0x9, 0x0}, // Decode-Mode Register Examples:	No decode for digits 7–0
	{0xA, 0xF}, // Intensity Register Format:		31/32 (max on)
	{0xB, 0x5}, // Scan-Limit Register Format:		Display digits 0-5
	{0xF, 0x0}, // Display-Test Register Format:	Normal Operation
	// Self-Test (ALL SEGMENTS ON)
	{1, 0xFF},{2, 0xFF},{3, 0xFF},{4, 0xFF},{5, 0xFF},{6, 0xFF},{7, 0xFF},{8, 0xFF}
};






/****** MAX7219 -- Driver ******/
#define MAX7219_CS	LATAbits.LATA2


void max7219_write(uint8_t address, uint8_t data){
	MAX7219_CS = 0;
		SSPBUF = address;while (SSPSTATbits.BF) NOP();
		SSPBUF = data;while (SSPSTATbits.BF) NOP();
	MAX7219_CS = 1;
}








void main(){
	OSCCONbits.IRCF = 0b1110;
	while ( !OSCSTATbits.HFIOFR );
	
	ANSELA = 0x00; // Digital I/O. Pin is assigned to port or digital special function.
	APFCON = 0x84; // Rx(RA4) | Tx(RA5)
	
	MAX7219_CS = 1;
	TRISA = 0xF8;
	
	
	// USART Async: BRGH=1; BRG16=1 | 115900 (115200) Error 0.64% at Fosc=32MHz
	TXSTA	= 0x24;
	RCSTA	= 0x90;
	BAUDCON = 0x48;
	SPBRGH	= 0;
	SPBRGL	= 68;
	
	
	// SPI Master: Fosc / 4 | SPI Mode: 0
	SSPSTAT  = 0x00;
	SSP1CON1 = 0x20;
	
	
	// MAX7219 INIT SEQUENCE
	for (uint8_t i = 0; i < sizeof(initSequence)/2; i++){
		max7219_write(initSequence[i][0], initSequence[i][1]);
	}
	
	__delay_ms(1000);
	
	// End: Selt-Test (ALL SEGMENTS OFF)
	for (uint8_t i = 1; i <= 8; i++){
		max7219_write(i, 0x00);
	}
	
	
	
	// INTERRUPTS
	INTCONbits.PEIE = 1;
	PIR1bits.RCIF   = 0;
	PIE1bits.RCIE   = 1;
	ei();
	
	
	while (1){
		
		if ( Buffer.index >= 6 ){
			Buffer.index = 0;
			
			if ( Buffer.data[5] == '\r' || Buffer.data[5] == '\n' ){
				di();
				
				// --- Set Displays ---
				max7219_write(1, Buffer.data[0]); // Digit 0
				max7219_write(2, Buffer.data[1]); // Digit 1
				max7219_write(3, Buffer.data[2]); // Digit 2
				
				
				// --- Set LEDs ---
				uint8_t currentLedIndex = 0;
				
				if (Buffer.data[3] > 20 ){
					Buffer.data[3] = 20;
				}
				
				for (uint8_t i = 0; i < 3; i++){ // 3x LEDs Groups 8+8+4 = 20
					uint8_t tempData = 0x00;

					for (uint8_t j = 0; j < 8; j++, currentLedIndex++){

						if ( currentLedIndex < Buffer.data[3] ){
							tempData |= 1 << (7 - j);
						}
					}
					
					max7219_write(4+i, tempData);
					
				}
				
				
				// --- Set Brightness ---
				max7219_write(0x0A, Buffer.data[4] & 0xF);
				
				ei();
			}
			
		}
		
	}
	
}




void __interrupt() ISR(){
	
	if ( RCIE && RCIF ){
		uint8_t data = RCREG;
		
		if ( Buffer.index < sizeof(Buffer.data) ){
			Buffer.data[Buffer.index] = data;
			Buffer.index++;
		}
		
	}
	
}
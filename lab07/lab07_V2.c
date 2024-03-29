#include "msp430g2553.h"
 
#define S1 0x0001
#define S2 0x0002
#define S3 0x0003
#define S4 0x0004
#define level_MAX 5

/* button press variables */ 
int buttonNumber = 0;
int Time = 0;
//int previousTime = 4096;
int endOfSequence = 0;
int startOfSequence = 0;
int RSTCnt = 1;
int counter = 0;
int start = 0;
int game = 0;
int game_start = 0;
int wrong = 1;
int press = 0;
int win = 0;
int LED_flag = 1;

/* button pattern variables */
char pattern[level_MAX]; // define pattern
int level = 0; 
int pointer = 0; // pointer to loop through the pattern
char newPattern[6]; // new pattern to be changed later in reset mode
int index = 0;
int firstPress = 0;

/* buzzer variables */
float winSeq[] = {1000000/277.18,1000000/261.63,
				  1000000/246.94,1000000/233.08}; // buzzer pattern for win
float loseSeq[] = {1000000/523.251,1000000/391.995,1000000/523.251,
				   1000000/659.255,1000000/783.991}; // buzzer pattern for lose
float periods[] = {1000000/261.63,1000000/293.66,
				   1000000/329.63,1000000/349.23}; // frequencies

/* LED variables */
int PERIOD = 240;
int step = 8;
int intensity;
int led_buzzer_pointer = 0;
int GLOBAL = 0x1F;
int seed;
int led1r;
int led1g;
int led1b;
int led2r;
int led2g;
int led2b;
int led3r;
int led3g;
int led3b;
int led4r;
int led4g;
int led4b;

/* setup functions */
void button_init(void);
void buzzer_init(void);
void led_init(void);
void WDT_init(void);
void USCI_init(void);
int rand32(int seed);

/* game functions */
void game_over(void);
void game_win(void);
void LED_on(int led1r, int led1g, int led1b,
			int led2r, int led2g, int led2b,
			int led3r, int led3g, int led3b,
			int led4r, int led4g, int led4b);
 
int main (void) {
	seed = 12;
	WDT_init();
	buzzer_init();
	button_init();
	led_init();
	USCI_init();
	LED_on(0,0,0,0,0,0,0,0,0,0,0,0);
	//__bis_SR_register(LPM0_bits | GIE);
	
	LED_on(0xFF,0,0,0,0,0,0,0,0,0,0,0);
	
}

int rand32(int seed) {
	static char lfsr[5]; // value for linear feedback shift register
	static unsigned bit3; // third bit
	static unsigned bit5; // fifth bit
	static int output = 0; // integer value of the output
	if (output == 0) {
		if (seed != 0) { // calculate the values for lfsr when seed is provided
			lfsr[4] = seed%2;
			lfsr[3] = (seed/2)%2;
			lfsr[2] = (seed/4)%2;
			lfsr[1] = (seed/8)%2;
			lfsr[0] = (seed/16)%2;
		}
		else { // return 0 if no seed is set
			return 0;
		}
	}

		bit5 = lfsr[4]; // get bit5 and bit3 for xor function
		bit3 = lfsr[2];
		lfsr[4] = lfsr[3]; // shifting lfsr 1 bit to the right
		lfsr[3] = lfsr[2];
		lfsr[2] = lfsr[1];
		lfsr[1] = lfsr[0];
		lfsr[0] = bit5 ^ bit3; // xor function
		output = lfsr[0]*16 + lfsr[1]*8 + lfsr[2]*4 + lfsr[3]*2 + lfsr[4]; // convert to decimal output
		if (output <= 8) {
			output = 1;
		} 
		else if (output > 8 && output <= 16) {
			output = 2;
		}
		else if (output > 16 && output <= 24) {
			output = 3;
		}
		else {
			output = 4;
		}
	return output;
}

/* set P2.0, P2.2, P2.3, P2.4 to be button inputs. */
void button_init() {
	// P1.2 is S1 and P1.3 is S2.
	P2DIR &= ~BIT0; 	// Set P2.0 as input
	P2OUT |= BIT0; 
	P2REN |= BIT0; 		// keep pin high until pressed
	P2IES |= BIT0; 		// Enable Interrupt 
	P2IFG &= ~BIT0; 	// Clear the interrupt flag
	P2IE |= BIT0; 		// Enable interrupts on port 1 

	P2DIR &= ~BIT2; 	// set P2.2 as input
	P2OUT |= BIT2; 
	P2REN |= BIT2; 
	P2IES |= BIT2; 
	P2IFG &= ~BIT2; 
	P2IE |= BIT2; 

	P2DIR &= ~BIT3; 	// set P2.3 as input
	P2OUT |= BIT3; 
	P2REN |= BIT3; 
	P2IES |= BIT3; 
	P2IFG &= ~BIT3; 
	P2IE |= BIT3; 

	P2DIR &= ~BIT4; 	// set P2.4 as input
	P2OUT |= BIT4; 
	P2REN |= BIT4; 
	P2IES |= BIT4; 
	P2IFG &= ~BIT4; 
	P2IE |= BIT4; 
}

/* TA1 setup for LED and buzzer. P2.1. */
void buzzer_init() {
  P2DIR |= BIT1;
  P2SEL |= BIT1; 			// Set P2.1 as TA1.1 PWM output.
  P2DIR |= BIT5; 		
  P2SEL |= BIT5; 			// Set BIT2 as TA1.2 PWM output.

  TA1CCR0 = 0;	 			// not playing anything initially	
  TA1CCR1 = 220; 			// divde by 2, 50%: loudest
  TA1CCR2 = 0; 
  TA1CCTL1 = OUTMOD_6;
  TA1CCTL2 = OUTMOD_6;
  TA1CTL = TASSEL_2 + MC_1; // SMCLK + upmode
}

/* set SPI LED outputs. */
void led_init() {
	P1SEL |= BIT2 + BIT4; 			// pin1.2 and pin1.4 are selected for PWM
	P1SEL2 |= BIT2 + BIT4;
}

/* initialize watchdog timer and interrupt */
void WDT_init() {
	WDTCTL = WDTPW | WDTHOLD; //WDT password + Stop WDT + detect RST button falling edge
	IE1 |= WDTIE; // Enable the WDT interrupt
	WDTCTL = WDT_MDLY_32;
}

void USCI_init() {
	UCA0CTL0 |= UCCKPH + UCSYNC + UCMST + UCMSB; // clock phase selected; Sync-mode-SPI-mode; master select; most significant bit first select.
	UCA0CTL1 |= UCSWRST + UCSSEL_1; // sw-reset enabled; clock source from ACLK
	UCB0BR0 |= BIT1; // data speed divide by 2
	UCA0CTL1 &= ~UCSWRST; // start USCI
}

void LED_on(int led1r, int led1g, int led1b, int led2r, int led2g, int led2b, int led3r, int led3g, int led3b, int led4r, int led4g, int led4b) {
	// send start frame
	while (!(IFG2 & UCA0TXIFG));
	UCA0TXBUF = 0x00;
	while (!(IFG2 & UCA0TXIFG));
	UCA0TXBUF = 0x00;
	while (!(IFG2 & UCA0TXIFG));
	UCA0TXBUF = 0x00;
	while (!(IFG2 & UCA0TXIFG));
	UCA0TXBUF = 0x00;
	// LED 1
	while (!(IFG2 & UCA0TXIFG)); // wait until transmission is complete
	UCA0TXBUF = 0xE0 + GLOBAL; // LED MSB bits(111) + Global bits (TODO: set global bits; 1111)
	while (!(IFG2 & UCA0TXIFG)); // wait until transmission is complete
	UCA0TXBUF = led1b; // set PWM for blue LED
	while (!(IFG2 & UCA0TXIFG)); // wait until transmission is complete
	UCA0TXBUF = led1g; // set PWM for green LED
	while (!(IFG2 & UCA0TXIFG)); // wait until transmission is complete
	UCA0TXBUF = led1r; // set PWM for red LED
	// LED 2
	while (!(IFG2 & UCA0TXIFG)); // wait until transmission is complete
	UCA0TXBUF = 0xE0 + GLOBAL; // LED MSB bits(111) + Global bits (TODO: set global bits; 1111)
	while (!(IFG2 & UCA0TXIFG)); // wait until transmission is complete
	UCA0TXBUF = led2b; // set PWM for blue LED
	while (!(IFG2 & UCA0TXIFG)); // wait until transmission is complete
	UCA0TXBUF = led2g; // set PWM for green LED
	while (!(IFG2 & UCA0TXIFG)); // wait until transmission is complete
	UCA0TXBUF = led2r; // set PWM for red LED
	// LED 3
	while (!(IFG2 & UCA0TXIFG)); // wait until transmission is complete
	UCA0TXBUF = 0xE0 + GLOBAL; // LED MSB bits(111) + Global bits (TODO: set global bits; 1111)
	while (!(IFG2 & UCA0TXIFG)); // wait until transmission is complete
	UCA0TXBUF = led3b; // set PWM for blue LED
	while (!(IFG2 & UCA0TXIFG)); // wait until transmission is complete
	UCA0TXBUF = led3g; // set PWM for green LED
	while (!(IFG2 & UCA0TXIFG)); // wait until transmission is complete
	UCA0TXBUF = led3r; // set PWM for red LED
	// LED 4
	while (!(IFG2 & UCA0TXIFG)); // wait until transmission is complete
	UCA0TXBUF = 0xE0 + GLOBAL; // LED MSB bits(111) + Global bits (TODO: set global bits; 1111)
	while (!(IFG2 & UCA0TXIFG)); // wait until transmission is complete
	UCA0TXBUF = led4b; // set PWM for blue LED
	while (!(IFG2 & UCA0TXIFG)); // wait until transmission is complete
	UCA0TXBUF = led4g; // set PWM for green LED
	while (!(IFG2 & UCA0TXIFG)); // wait until transmission is complete
	UCA0TXBUF = led4r; // set PWM for red LED
	// end frame
	while (!(IFG2 & UCA0TXIFG)); // wait until transmission is complete
	UCA0TXBUF = 0xFF;
	while (!(IFG2 & UCA0TXIFG)); // wait until transmission is complete
	UCA0TXBUF = 0xFF;
	while (!(IFG2 & UCA0TXIFG)); // wait until transmission is complete
	UCA0TXBUF = 0xFF;
	while (!(IFG2 & UCA0TXIFG)); // wait until transmission is complete
	UCA0TXBUF = 0xFF;
}

/* game over function */
void game_over() {
	//for (int i=0;i<sizeof(loseSeq)/sizeof*(loseSeq)-1;i++) {
		TA1CCR0 = loseSeq[0];
		//__delay_cycles(100000);
	//}
	// LED red
	LED_on(0xFF,0,0,0xFF,0,0,0xFF,0,0,0xFF,0,0);
}

void game_win() {
	//for (int i=0;i<sizeof(winSeq)/sizeof*(winSeq)-1;i++) {
		TA1CCR0 = winSeq[1];
		//__delay_cycles(100000);
	//}
	// LED green
	LED_on(0,0xFF,0,0,0xFF,0,0,0xFF,0,0,0xFF,0);
}

#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void) {
	/* interrupt routine for button P2.0 */
 	if (P2IFG & BIT0) {
	 		P2IE &= ~BIT0; 				// Disable Button interrupt
	 		P2IFG &= ~BIT0; 			// Clear the interrupt flag
 		if (P2IES & BIT0) { 		// Falling edge detected
 			firstPress = 1;
 	  		buttonNumber = S1;  	// record the button number for pattern checking
 	  		//LED_on(0,0,0xFF,0,0,0,0,0,0,0,0,0);
 	  		//LED_on(0,0,0,0,0,0,0,0,0,0,0,0);
 	  		/* check pattern correctness */
 	  		if (pattern[pointer] == buttonNumber) {
		 		pointer ++;			// if correct, go to next pattern in the sequence
		 		startOfSequence = 1;
		 		press = 1;
		 		LED_on(0,0,0xFF,0,0,0,0,0,0,0,0,0);
		 		LED_on(0,0,0,0,0,0,0,0,0,0,0,0);
		 	}
		 	/* incorrect press */
		 	else {
				pointer = 0; 		// reset pointer 	
				P1OUT &= ~BIT0; 	// check if entered else statement	
				firstPress = 0;
				wrong = 1;
		 	}
		 	/* start the sequence again if the entire sequence is pressed correctly */
		 	if (pointer > level-1) {
		 		pointer = 0;		// go to the start of pattern
		 		endOfSequence = 1;	// set end of sequence flag
		 	}
 		} else {
 			// turn off rising edge

 		}
 		//P2IES ^= BIT0; // Toggle edge detect
 	}
 	/* interrupt routine for button P2.2 */
 	 	else if (P2IFG & BIT2) { // add other port 1 interrupts
 	 		P2IE &= ~BIT2; 				// Disable Button interrupt
 	 		P2IFG &= ~BIT2; 			// Clear the interrupt flag
 	 		if (P2IES & BIT2) { 		// Falling edge detected
 	 			firstPress = 1;
 		  		buttonNumber = S2; 		// record the button number for pattern checking
			 		//LED_on(0,0,0,0,0,0xFF,0,0,0,0,0,0);
			 		//LED_on(0,0,0,0,0,0,0,0,0,0,0,0);
 		  		/* check pattern correctness */
 		  		if (pattern[pointer] == buttonNumber) {
 			 		pointer ++;			// go to next pattern
 			 		startOfSequence = 1;// set start of sequence flag
 			 		press = 1;
 			 		LED_on(0,0,0,0,0,0xFF,0,0,0,0,0,0);
 			 		LED_on(0,0,0,0,0,0,0,0,0,0,0,0);
 			 	}
 			 	/* reset everything for incorrect press */
 			 	else {
 					pointer = 0; 		// reset pointer
 					firstPress = 0;
 					wrong = 1;
 			 	}
 			 	/* if the entire sequence is pressed correctly */
 			 	if (pointer > level-1) {
 			 		pointer = 0;
 			 		endOfSequence = 1;
 			 	}
 	 		}
 	 	}
 	 	/* interrupt routine for button P2.3 */
 	 	else if (P2IFG & BIT3) { // add other port 1 interrupts
 	 		P2IE &= ~BIT3; 				// Disable Button interrupt
 	 		P2IFG &= ~BIT3; 			// Clear the interrupt flag
 	 		if (P2IES & BIT3) { 		// Falling edge detected
 	 			firstPress = 1;
 		  		buttonNumber = S3; 		// record the button number for pattern checking
			 		//LED_on(0,0,0,0,0,0,0,0,0xFF,0,0,0);
			 		//LED_on(0,0,0,0,0,0,0,0,0,0,0,0);
 		  		/* check pattern correctness */
 		  		if (pattern[pointer] == buttonNumber) {
 			 		pointer ++;			// go to next pattern
 			 		startOfSequence = 1;// set start of sequence flag
 			 		press = 1;
 			 		LED_on(0,0,0,0,0,0,0,0,0xFF,0,0,0);
 			 		LED_on(0,0,0,0,0,0,0,0,0,0,0,0);
 			 	}
 			 	/* reset everything for incorrect press */
 			 	else {
 					pointer = 0; 		// reset pointer
 					firstPress = 0;
 					wrong = 1;
 			 	}
 			 	/* start the sequence again if the entire sequence is pressed correctly */
 			 	if (pointer > level-1) {
 			 		pointer = 0;
 			 		endOfSequence = 1;
 			 	}
 	 		}
 	 	}
 	 	/* interrupt routine for button P2.4 */
 	 	else if (P2IFG & BIT4) { // add other port 1 interrupts
 	 		P2IE &= ~BIT4; 				// Disable Button interrupt
 	 		P2IFG &= ~BIT4; 			// Clear the interrupt flag
 	 		if (P2IES & BIT4) { 		// Falling edge detected
				buttonNumber = S4; 		// record the button number for pattern checking
				press = 1;
					//LED_on(0,0,0,0,0,0,0,0,0,0,0,0xFF);
					//LED_on(0,0,0,0,0,0,0,0,0,0,0,0);
				if (firstPress) {
					wrong = 0;
					win = 0;
					game = 1;
					game_start = 0;
					firstPress = 0;
				} else {
					/* check pattern correctness */
					if (pattern[pointer] == buttonNumber) {
						pointer ++;			// go to next pattern
						startOfSequence = 1;// set start of sequence flag
						press = 1;
						LED_on(0,0,0,0,0,0,0,0,0,0,0,0xFF);
						LED_on(0,0,0,0,0,0,0,0,0,0,0,0);
					}
					/* reset everything for incorrect press */
					else {
						pointer = 0; 		// reset pointer
						firstPress = 1;
						wrong = 1;
						LED_flag = 1;
					}
					/* start the sequence again if the entire sequence is pressed correctly */
					if (pointer > level) {
						pointer = 1;
						level++;
						game_start = 0;
					}
 	 			}
 			}	
 		}
}
      
/* WDT interrupt routine for update LED and buzzer, restart and reset pattern */
#pragma vector = WDT_VECTOR
__interrupt void wdt_isr(void) {
 	P2IFG &= ~BIT0; // Clear the button interrupt flag
 	P2IE |= BIT0; 	// Re-enable interrupt for the button on P1.3
 	P2IFG &= ~BIT2;
 	P2IE |= BIT2;
 	P2IFG &= ~BIT3;
 	P2IE |= BIT3;
 	P2IFG &= ~BIT4;
 	P2IE |= BIT4;

 	if (wrong) {
 		// pressed wrong
 		// flash game over pattern
 		if (LED_flag) {
	 		game_over();	
	 	}
 		// reset certain flags
	 	start = 0; 			// reset start flag
	 	LED_flag = 0;
		level = 1;
		game = 0;
		game_start = 0;
		counter = 0;
		firstPress = 1;
 		//LED_flag = 1;
 	} 
 	else if (win) {
 		// win
 		// flash game win pattern
 		if (LED_flag) {
 			game_win();
 		}
	 	// reset certain flags
	 	start = 0; 			// reset start flag
	 	LED_flag = 0;
		level = 1;
		game = 0;
		game_start = 0;
		firstPress = 1;
 	}
	
	while (game) {
		// set pattern 
		if (level <= level_MAX) {
			if (!game_start) {
				pattern[level] = rand32(seed);
				// play pattern
				int i = 1;
				while (i < level) {
					if (pattern[i-1] == 1) {
						LED_on(0xFF,0,0xFF,0,0,0,0,0,0,0,0,0);
						LED_on(0,0,0,0,0,0,0,0,0,0,0,0);
						TA1CCR0 = periods[i];
					} else if (pattern[i-1] == 2) {
						LED_on(0,0,0,0xFF,0,0xFF,0,0,0,0,0,0);
						LED_on(0,0,0,0,0,0,0,0,0,0,0,0);
						TA1CCR0 = periods[i];						
					} else if (pattern[i-1] == 3) {
						LED_on(0,0,0,0,0,0,0xFF,0,0xFF,0,0,0);
						LED_on(0,0,0,0,0,0,0,0,0,0,0,0);
						TA1CCR0 = periods[i];						
					} else if (pattern[i-1] == 4) {
						LED_on(0,0,0,0,0,0,0,0,0,0xFF,0,0xFF);
						LED_on(0,0,0,0,0,0,0,0,0,0,0,0);
						TA1CCR0 = periods[i];
					}
					i++;

				}
				// game start flag
				game_start = 1;
				// reset timer
				counter = 0;
			}
			if (game_start) {
				// increment counter
				counter ++;
				// reset timer if button pressed
				if (press) {
					counter = 0;
					press = 0;
				}
				if (counter > 63) {
					wrong = 1;
					counter = 0;
				}
			}
		} else {
			// win the game if max level is reached
			game = 0;
			win = 1;
		}	
 	}
}

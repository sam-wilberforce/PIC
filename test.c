#include <htc.h>                         //PIC hardware mapping
#define _XTAL_FREQ   500000                //Used by the compiler for the delay_ms(x) macro

#define DOWN         0
#define UP           1

#define SWITCH       PORTAbits.RA2

#define LED0         LATCbits.LATC0
#define LED1         LATCbits.LATC1
#define LED2         LATCbits.LATC2
#define LED3         LATCbits.LATC3

#define PULL_UPS  //if this is uncommented, the trace under JP5 can be cut with no ill effects

//config bits that are part-specific for the PIC18F14K22
__CONFIG(1, FOSC_IRC & PLLEN_OFF & FCMEN_OFF);
__CONFIG(2, PWRTEN_OFF & BOREN_OFF & WDTEN_OFF);
__CONFIG(3, HFOFST_OFF & MCLRE_OFF);
__CONFIG(4, STVREN_ON & LVP_OFF & DEBUG_ON);
__CONFIG(5, CP0_OFF & CP1_OFF & CPB_OFF & CPD_OFF);
__CONFIG(6, WRT0_OFF & WRT1_OFF & WRTC_OFF & WRTB_OFF & WRTD_OFF);
__CONFIG(7, EBTR0_OFF & EBTR1_OFF & EBTRB_OFF);

//prototype
void setupp(void);
void wait_plz(void);

//globals
unsigned char _prev_switch; //need to add this for the PIC18 since there is no specific interrupt-on-change NEGATIVE....
signed int MY_FLAG = 0;

void main(void) {
    setupp();
    while (1) {
        if (MY_FLAG == UP) {
            wait_plz();
            LED0 ^= 1;
            if (MY_FLAG == UP) { //these checks are to stop pattern finishing
                wait_plz(); //after switch is pressed
                LED1 ^= 1;
            }
            if (MY_FLAG == UP) {
                wait_plz();
                LED2 ^= 1;
            }
            if (MY_FLAG == UP) {
                wait_plz();
                LED3 ^= 1;
            }
        } else LATC = 0;
    }
}

void interrupt ISR(void) {
    if (INTCONbits.RABIF) { //SW1 was just pressed
        INTCONbits.RABIF = 0; //must clear the flag in software
        __delay_ms(5); //debounce by waiting and seeing if still held down
        if (SWITCH == DOWN && _prev_switch == UP) {
            MY_FLAG ^= 1;
        }
        if (SWITCH == DOWN) {
            _prev_switch = DOWN;
        } else
            _prev_switch = UP;
    }

    if (INTCONbits.T0IF) //TMR1 overflowed
        INTCONbits.T0IF = 0; //must clear flag
}

void setupp(void) {
    OSCCON = 0b00100010; //500KHz clock speed
    TRISC = 0; //all LED pins are outputs

    TRISAbits.TRISA2 = 1; //switch input
    TRISAbits.TRISA4 = 1; //Potentiamtor is connected to RA4...set as input

    ANSELbits.ANS2 = 0; //digital for switch
    ANSELbits.ANS3 = 1; //analog input

    LATC = 0; //start with no LEDs lit

    ADCON0 = 0b00001101; //select RA4 as source of ADC, which is AN3, and enable the module
    ADCON2 = 0b00000001; //left justified - FOSC/8 speed


    //by using the internal resistors, you can save cost by eleminating an external pull-up/down resistor
#ifdef PULL_UPS
    WPUA2 = 1; //enable the weak pull-up for the switch
    nRABPU = 0; //enable the global weak pull-up bit
    //this bit is active HIGH, meaning it must be cleared for it to be enabled
#endif

    //setup TIMER0 as the delay
    T0CON = 0b11000111; //8bit timer - enable - 1:256 prescaler
    INTCONbits.TMR0IE = 1; //enable the TMR0 rollover interrupt

    //setup interrupt on change for the switch
    INTCONbits.RABIE = 1; //enable interrupt on change global
    IOCAbits.IOCA2 = 1; //when SW1 is pressed/released, enter the ISR

    RCONbits.IPEN = 0; //disable interrupt priorites
    INTCONbits.GIE = 1; //enable global interupts
}

void wait_plz(void) {
    __delay_us(5); //wait for ADC charging cap to settle
    GO = 1;
    while (GO) continue; //wait for conversion to be finished
    __delay_ms(5);
    while (ADRESH-- != 0)
        __delay_ms(2);
}
#include <xc.h>
#include <stdint.h>

// --- MICROCONTROLLER CONFIGURATION ---
#pragma config FOSC = HS    // High Speed Oscillator (20 MHz)
#pragma config WDTE = OFF   // Watchdog Timer
#pragma config PWRTE = OFF  // Power-up Timer
#pragma config BOREN = OFF  // Brown-out Reset
#pragma config LVP = OFF    // Low-Voltage Programming
#pragma config CPD = OFF    // Data EE Read Protect
#pragma config WRT = OFF    // Flash Program Memory Write Enable
#pragma config CP = OFF     // Code Protection

#define _XTAL_FREQ 20000000

void setup();
void adc_init();
unsigned int adc_read_int();
void send_output_data(unsigned int val);
float filter_float(float new_sample);

//Fc = 10hz Fs = 316 Hz Ordem 2, butterworth, passa baixa iir;
const float b0 = 0.008644;
const float b1 = 0.017288;
const float b2 = 0.008644;
const float a1 = -1.7203135;
const float a2 = 0.7548894;

float x1 = 0.0, x2 = 0.0;
float y1 = 0.0, y2 = 0.0;

void setup() {
    TRISAbits.TRISA0 = 1; // RA0 as input (analog)
    TRISB = 0;            // PORTB as output (DAC)
    TRISC = 0;            // PORTC as output (DAC and debug)
}

void adc_init(){
    ADCON0 = 0b10000001;
    ADCON1 = 0b10001110;
}

unsigned int adc_read_int(){
    ADCON0bits.GO_nDONE = 1;
    while (ADCON0bits.GO_nDONE);
    return ((unsigned int)ADRESH << 8) | ADRESL;
}

void send_output_data(unsigned int val){
    PORTB = val & 0xFF;
    PORTC = (PORTC & 0xFC) | ((val >> 8) & 0x03);
}

float filter_float(float new_sample){
    float output;
    
    output = b0 * new_sample + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    
    x2 = x1;
    x1 = new_sample;
    y2 = y1;
    y1 = output;
    
    return output;
}

void main(void) {
    setup();
    adc_init();
    
    x1 = 0;
    x2 = 0;
    y1 = 0;
    y2 = 0;
    
    __delay_us(50);
    
    while (1) {
        PORTCbits.RC2 = 1;
        float adc_val_float = (float)adc_read_int();
        float filtered_val_float = filter_float(adc_val_float);
       
        if (filtered_val_float < 0.0) {
            filtered_val_float = 0.0;
        } else if (filtered_val_float > 1023.0) {
            filtered_val_float = 1023.0;
        }

        unsigned int output_val = (unsigned int)(filtered_val_float);
        
        send_output_data(output_val);
        
        PORTCbits.RC2 = 0;
    }
}
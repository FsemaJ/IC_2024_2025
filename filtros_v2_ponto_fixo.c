#include <xc.h>

// --- CONFIGURACAO DO MICROCONTROLADOR ---
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
unsigned int adc_read();
void send_output_data(unsigned int val);
long filter_q(long input);

#define Q_SHIFT 10


// Fc = 87Hz Fs = 1030Hz Q_SHIFT = 2¹º = 1024 Ordem 2, butterworth, passa baixa iir

const int b_0 = 52; 
const int b_1 = 104;
const int b_2 = 52;
const int a_1 = -1301;
const int a_2 = 484;

long x_1 = 0, x_2 = 0;
long y_1 = 0, y_2 = 0;

void setup() {
    TRISAbits.TRISA0 = 1; // RA0 as input (analog)
    TRISB = 0;            // PORTB as output (DAC)
    TRISC = 0;            // PORTC as output (DAC and debug)
}

void adc_init(){
    ADCON0 = 0b10000001;
    ADCON1 = 0b10001110;
}

unsigned int adc_read(){
    ADCON0bits.GO_nDONE = 1;
    while (ADCON0bits.GO_nDONE);
    return ((unsigned int)ADRESH << 8) | ADRESL;
}

void send_output_data(unsigned int val){
    PORTB = val & 0xFF;
    PORTC = (PORTC & 0xFC) | ((val >> 8) & 0x03);
}

// --- FUNCAO DE FILTRO IIR OTIMIZADA E ROBUSTA ---
long filter_q(long input){
    long sum_b = 0;
    long sum_a = 0;
    long output = 0;
    
    sum_b += (long)b_0 * input;
    sum_b += (long)b_1 * x_1;
    sum_b += (long)b_2 * x_2;
    
    sum_a += (long)a_1 * y_1;
    sum_a += (long)a_2 * y_2;
    
    output = (sum_b - sum_a) >> Q_SHIFT;
    
    x_2 = x_1;
    x_1 = input; 
    y_2 = y_1;
    y_1 = output;

    return output;
}

void main(void) {
    
    setup();
    adc_init();
    
    __delay_us(50);
    
    while(1){
        
        PORTCbits.RC2 = 1;
        
        unsigned int adc_val = adc_read();
        long adc_q = (long)adc_val << Q_SHIFT;
        
        long filtered_q = filter_q(adc_q);
        
        if (filtered_q < 0) {
            filtered_q = 0;
        }
        
        unsigned int output_val = (unsigned int)(filtered_q >> Q_SHIFT);
        
        if (output_val > 1023) {
            output_val = 1023;
        }
        
        send_output_data(output_val);
        
        PORTCbits.RC2 = 0;
        
    }
    return;
}
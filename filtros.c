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

// --- PROTÓTIPOS DE FUNÇÃO ---
void setup();
void adc_init();
unsigned int adc_read_int();
float adc_read_float();
void send_output_data(unsigned int val);
float filter_float(float new_sample);

// --- COEFICIENTES DO FILTRO (10Hz, 316Hz) EM FLOAT ---
const float b0 = 0.000980;
const float b1 = 0.001960;
const float b2 = 0.000980;
const float a1 = -1.931750;
const float a2 = 0.935670;

// --- VARIAVEIS DE ESTADO ---
float x1 = 0.0, x2 = 0.0;
float y1 = 0.0, y2 = 0.0;

// --- CONFIGURACAO E INICIALIZACAO ---
void setup() {
    TRISAbits.TRISA0 = 1;
    TRISB = 0;
    TRISCbits.TRISC0 = 0;
    TRISCbits.TRISC1 = 0;
    TRISCbits.TRISC2 = 0;
}

void adc_init(){
    ADCON0 = 0b10000001;
    ADCON1 = 0b10000000;
}

unsigned int adc_read_int(){
    ADCON0bits.GO_nDONE = 1;
    while (ADCON0bits.GO_nDONE);
    return ((unsigned int)ADRESH << 8) | ADRESL;
}

float adc_read_float(){
    return (float)adc_read_int();
}

void send_output_data(unsigned int val){
    PORTB = val & 0xFF;
    PORTCbits.RC0 = (val >> 8) & 0x01;
    PORTCbits.RC1 = (val >> 9) & 0x01;
}

// --- FUNCAO DO FILTRO DIGITAL (com float) ---
float filter_float(float new_sample){
    float output;
    output = b0 * new_sample + b1 * x1 + b2 * x2 - a1 * y1 - a2 * y2;
    x2 = x1;
    x1 = new_sample;
    y2 = y1;
    y1 = output;
    return output;
}

// --- LOOP PRINCIPAL ---
void main(void) {
    setup();
    adc_init();
    
    __delay_us(50);
    while (1) {
        PORTCbits.RC2 = 1;
        float adc_val_float = adc_read_float();
        float filtered_val_float = filter_float(adc_val_float);
        
        if (filtered_val_float < 0.0) {
            filtered_val_float = 0.0;
        }

        unsigned int output_val = (unsigned int)(filtered_val_float);
        
        if (output_val > 1023) {
            output_val = 1023;
        }
        send_output_data(output_val);
        
        PORTCbits.RC2 = 0;
    }
}
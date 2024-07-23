#define F_CPU 16000000UL
#include <util/delay.h>
#include <avr/io.h>
#include <stdint.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include "ADC/ADC.h"
#include "LCD 8bits/LCD8b.h"
#include "Uart/Uart.h"

/************************ VARIABLES ***********************/
volatile uint16_t adc_value5 = 0;       // Almacena el valor del ADC del canal 5
volatile uint16_t adc_value6 = 0;       // Almacena el valor del ADC del canal 6
volatile uint8_t current_channel = 5;   // Canal inicial
uint8_t inicio = 1;
volatile char buffer[16];
int voltage5_int, voltage6_int;
volatile int contador = 0;              // Contador para el UART


void iniciar_conversion_adc(uint8_t canal);
void IntCont();
void mostrarLCD();
void enviarValoresUART();


/************************ ENVÍO A UART *******************/
void enviarValoresUART() {
	UART_send_string("\n");
	// Crear las cadenas de texto para los valores de V1 y V2
	snprintf(buffer, 16, "V1: %d.%02dV\r\n", voltage5_int / 100, voltage5_int % 100);
	UART_send_string(buffer);

	snprintf(buffer, 16, "V2: %d.%02dV\r\n", voltage6_int / 100, voltage6_int % 100);
	UART_send_string(buffer);

	snprintf(buffer, 16, "S3: %d\r\n", contador);
	UART_send_string(buffer);
}

/************************ FUNCIONES DEL CONTADOR *******************/
void IntCont() {
	// Leer datos de UART para incrementar o decrementar el contador
	if (UART_available()) {
		char comando = UART_receive();
		if (comando == '+') {
			contador++;
			enviarValoresUART(); // Enviar los valores a la consola a través de UART
			} else if (comando == '-') {
			contador--;
			enviarValoresUART(); // Enviar los valores a la consola a través de UART
			} else {
				UART_send_string("\n El comando ingresado no se reconoce \n");
			}
	}
}

/************************ MOSTRAR EN LCD *******************/
void mostrarLCD() {
	// Convertir los valores del ADC a voltaje (asumiendo referencia de 5V y resolución de 10 bits)
	voltage5_int = (int)(adc_value5 * (5.0 / 1023.0) * 100); // Multiplicar por 100 para mantener dos decimales
	voltage6_int = (int)(adc_value6 * (5.0 / 1023.0) * 100);

	// Mostrar identificadores y voltajes en la LCD
	// V1
	LCD_Set_Cursor(2, 1);
	LCD_Write_String("V1 ");
	LCD_Set_Cursor(2, 2); // Posiciona el cursor para mostrar el voltaje de V1
	snprintf(buffer, 16, "%d.%02dV", voltage5_int / 100, voltage5_int % 100);
	LCD_Write_String(buffer);

	// V2
	LCD_Set_Cursor(8, 1);
	LCD_Write_String("V2 ");
	LCD_Set_Cursor(8, 2); // Posiciona el cursor para mostrar el voltaje de V2
	snprintf(buffer, 16, "%d.%02dV", voltage6_int / 100, voltage6_int % 100);
	LCD_Write_String(buffer);

	// Mostrar el valor del contador en la LCD
	LCD_Set_Cursor(14, 1);
	LCD_Write_String("S3 ");
	LCD_Set_Cursor(14, 2); // Reposiciona el cursor para mostrar el contador
	LCD_Write_String("        "); // Escribe espacios en blanco para borrar cualquier carácter residual
	LCD_Set_Cursor(14, 2);
	snprintf(buffer, 16, "%d", contador);
	LCD_Write_String(buffer);
}

/************************ CONVERSIÓN ADC *******************/
void iniciar_conversion_adc(uint8_t canal) {
	if (canal == 5) {
		ADC5();
		current_channel = 5;
		} else {
		ADC6();
		current_channel = 6;
	}
	habilitar_conversion(); // Iniciar la conversión
}

/************************ INTERRUPCIONES *******************/
// Rutina de interrupción del ADC
ISR(ADC_vect) {
	if (current_channel == 5) {
		adc_value5 = ADC; // Leer el valor del canal 5
		iniciar_conversion_adc(6); // Cambiar al canal 6
		} else {
		adc_value6 = ADC; // Leer el valor del canal 6
		iniciar_conversion_adc(5); // Cambiar al canal 5
	}
}

// Rutina de interrupción del UART
ISR(USART_RX_vect) {
	uart_buffer[uart_head] = UDR0; // Leer el carácter recibido del registro UDR0
	uart_head = (uart_head + 1) % sizeof(uart_buffer); // Actualizar el índice de la cabeza del buffer
	uart_buffer[uart_head] = '\0'; // Terminador de cadena
}

/***************** PROGRAMA PRINCIPAL **********************/
int main(void) {
	// Inicializar LCD, ADC y UART
	initLCD8bits();
	ADC_init(128);
	UART_init(9600);
	sei(); // Habilitar interrupciones globales

	
	// Iniciar la conversión en el canal 5
	iniciar_conversion_adc(5);
	while (1) {
	if( inicio == 1){
		_delay_ms(500); // Esperar un tiempo para permitir la actualización del ADC
		UART_send_string("\0  \r\n");
		UART_send_string(" 1. Ingrese '-' para decrementar el contador.\r\n");
		UART_send_string(" 2. Ingrese '+' para incrementar el contador.\r\n");	
		inicio = 0;

		
	} else{
	
		_delay_ms(500); // Esperar un tiempo para permitir la actualización del ADC
		IntCont(); // Verificar y procesar los comandos UART
		mostrarLCD(); // Mostrar los valores en la LCD
	}
	}
}

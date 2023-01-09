#define F_CPU 1000000UL
#define BAUDRATE (F_CPU/(16*4800L))-1
#define INT_LEAK (PIND&(1<<PD2))
#define INT_LB (PIND&(1<<PD3))
#define WAIT 1000000UL

#include <avr/io.h>
#include <string.h>
#include <util/delay.h>
#include <avr/interrupt.h>

int stop_key = 0;

void PIN_Init(void){
	DDRA |= ((1<<0) | (1<<1));
	PORTA &= ~(1<<0);
	PORTA |= (1<<1);
	DDRD &= ~((1<<2) | (1<<3));
	PORTD |= ((1<<2) | (1<<3));
}

ISR(USART_RX_vect){
	stop_key = UDR;
}

void UART_Init(unsigned int baud){
	UBRRH = (unsigned char)(baud>>8);
	UBRRL = (unsigned char)baud;
	UCSRB = (1<<RXEN)|(1<<TXEN);
	UCSRC = (1<<USBS)|(3<<UCSZ0);
	UCSRB |= (1<<RXCIE);
	sei();
}

void UART_Transmit(unsigned char data){
	while (!(UCSRA & (1<<UDRE)));
	UDR = data;
}

unsigned char UART_Receive(void){
	while (!(UCSRA & (1<<RXC)));
	return UDR;
}

void USART_Flush( void )
{
	while ( UCSRA & (1<<RXC) ) {
		unsigned char dummy = UDR;
	}
}

void send_message(char* message){
	int length = strlen(message);
	for (int i = 0; i < length; i++) {
		UART_Transmit(message[i]);
	};
	UART_Transmit('\n');
}

int main(void)
{
	PIN_Init();
	UART_Init(BAUDRATE);
	
	char* response[2] = {"Leak is detected!", "No leak!"};
	int i = 0;
	int counter = WAIT;
	
    while (1) 
    {
		PORTA ^= ((1<<0) | (1<<1));
		if (stop_key == '1' || counter <= 0) {
			send_message(response[i]);
			i++;
			if (i > 1) {
				i = 0;
			}
			if (counter <= 0) {
				send_message("counter is less than zero..");
				counter = WAIT;
			}
			
		}
		counter--;
    }
}


#define F_CPU 1000000UL
#define BAUDRATE (F_CPU/(16*4800L))-1
#define WAIT 200

#include <avr/io.h>
#include <string.h>
#include <util/delay.h>
#include <avr/interrupt.h>

int stop_key = 0;
int leak = 0;
int counter;
int INT_counter = 0;

ISR(INT0_vect){
	stop_key = 0;
	counter = WAIT;
	if (~PIND & (1<<2)) {
		leak = 1;
	}
	else {
		leak = 0;
	}
	MCUCR ^= (1<<0);
	_delay_ms(500);
}

ISR(INT1_vect){
	leak = 0;
}

ISR(USART_RX_vect){
	stop_key = UDR;
}

void PIN_Init(void){
	GIMSK |= (1<<6);
	MCUCR &= ~(1<<0);
	MCUCR |= (1<<1);
	SREG |= (1<<7);
	DDRD &= ~(1<<1);
	PORTB |= (1<<1);
	DDRA |= ((1<<0) | (1<<1));
	PORTA &= ~(1<<0);
	PORTA |= (1<<1);
	DDRD &= ~((1<<2) | (1<<3));
	PORTD |= ((1<<2) | (1<<3));
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

void send_message(char* message){
	int length = strlen(message);
	for (int i = 0; i < length; i++) {
		UART_Transmit(message[i]);
	};
	UART_Transmit('\n');
}

void BT_Transmit(int state){
//	_delay_ms(2000);

}

int main(void)
{
	PIN_Init();
	UART_Init(BAUDRATE);
	char* response[2] = {"No leak!", "Leak is detected!"};
	while (1){
	//	PORTA ^= ((1<<0) | (1<<1));
		if (leak == 0 || leak == 1){
			if (stop_key != '1' && counter == WAIT) {
				send_message(response[leak]);
				if (counter <= 0) {
					counter = WAIT;
				}
			}
			if (stop_key == '1') {
				leak = -1;
			}
			counter--;
		}			
	}
}


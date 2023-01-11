#define F_CPU 1000000UL
#define BAUDRATE (F_CPU/(16*4800L))-1
#define WAIT 100

#include <avr/io.h>
#include <string.h>
#include <util/delay.h>
#include <avr/interrupt.h>

int stop_key = 0;
int lb = 0;
int leak;
int counter;


void PIN_Init(void){
	GIMSK |= 1<<INT0;
	MCUCR |= (1<<ISC01);
	MCUCR &= ~(1<<ISC00);
	GIMSK |= 1<<INT1;
	MCUCR |= (1<<ISC11) | (1<<ISC10);		
	DDRD &= ~((1<<DDD3) | (1<<DDD2));
	PORTD |= ((1<<PORTD3) | (1<<PORTD2));	
	DDRA |= ((1<<DDA1) | (1<<DDA0));
	PORTA &= ~((1<<PORTA1) | (1<<PORTA0));
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
void BT_ON(void){
	_delay_ms(1000);;
	PORTA |= ((1<<PORTA1) | (1<<PORTA0));
	
}

void BT_OFF(void){
	PORTA &= ~((1<<PORTA1) | (1<<PORTA0));
}

ISR(INT0_vect){
	stop_key = 0;
	counter = WAIT;
	if (~PIND & (1<<2)) {
		leak = 1;
		MCUCR |= 1<<ISC00;
	}
	else {
		leak = 0;
		MCUCR &= ~(1<<ISC00);
	}
	BT_ON();
}

ISR(INT1_vect){
	stop_key = 0;
	counter = WAIT;
	lb = 1;
	BT_ON();
}

ISR(USART_RX_vect){
	stop_key = UDR;
}

int main(void)
{
	PIN_Init();
	UART_Init(BAUDRATE);
	char* response[2] = {"No leak!", "Leak is detected!"};
	leak = 0;
	BT_ON();
	while (1){
		if (leak == 0 || leak == 1 || lb == 1){
			if (stop_key != '1' && counter == WAIT) {
				if (leak == 0 || leak == 1) {
					send_message(response[leak]);
				}
				if (lb == 1) {
					send_message("Low battery!");
				}
				if (counter <= 0) {
					counter = WAIT;
				}
			}
			counter--;
			if (stop_key == '1') {
				BT_OFF();
				leak = -1;
				lb = 0;
//				MCUCR |= 1<<SE;
//				MCUCR |= (1<<SM1) | (1<<SM0);
//				asm("sleep");
			}
		}			
	}
}


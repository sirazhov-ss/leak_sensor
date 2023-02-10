#define F_CPU 1000000UL
#define BAUDRATE (F_CPU/(16*4800L))-1
#define WAIT 10000
#include <avr/io.h>
#include <string.h>
#include <util/delay.h>
#include <avr/interrupt.h>

char stop_key;
int leak;
int lb;
int counter;
int send;


void PIN_Init(void){
	DDRD &= ~((1<<PD3) | (1<<PD2));
	PORTD &= ~((1<<PD3) | (1<<PD2));
	GIMSK |= 1<<INT1;
	MCUCR &= ~((1<<ISC11) | (1<<ISC10));
	sei();
	DDRA |= ((1<<PA1) | (1<<PA0));
	PORTA |= 1<<PA0;
	PORTA &= ~(1<<PA1);
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
	PORTA |= ((1<<PA1) | (1<<PA0));
	
}

void BT_OFF(void){
	PORTA &= ~((1<<PA1) | (1<<PA0));
}

int get_state(int bit){
	int state = -1;
	if (~PIND & (1<<bit)) {
		state = 1;
	}
	else {
		state = 0;
	}
	return state;
}

ISR(INT0_vect){
	MCUCR &= ~(1<<SE);
	GIMSK &= ~(1<<INT0);
	send = 1;
	stop_key = '0';
	counter = WAIT;
	BT_ON();
}

ISR(INT1_vect){
	MCUCR &= ~(1<<SE);
	GIMSK &= ~(1<<INT1);
	send = 1;
	lb = 1;
	stop_key = '0';
	counter = WAIT;
	BT_ON();
}

ISR(WDT_OVERFLOW_vect){
	if (PIND & (1<<PD2)) {
		MCUCR &= ~(1<<SE);
		WDTCR|=(1<<WDCE) | (1<<WDE);
		WDTCR &= ~((1<<WDE) | (1<<WDIE));
		send = 1;
		stop_key = '0';
		counter = WAIT;
		BT_ON();
	}
	else{
		for (int i=0; i<4; i++){
			PORTA ^= 1<<PA0;
			_delay_ms(200);
		}
	}
	asm("wdr");
}

ISR(USART_RX_vect){
	stop_key = UDR;
}

int main(void)
{
	PIN_Init();
	UART_Init(BAUDRATE);
	char* response[2] = {"No leak!", "Leak is detected!"};
	_delay_ms(500);
	lb = get_state(3);
	_delay_ms(500);
	leak = get_state(2);
	_delay_ms(500);
	PORTA |= 1<<PA1;
	stop_key = '0';
	counter = WAIT;
	send = 1;
	while (1){
		if (send == 1) {
			leak = get_state(2);
			if (stop_key != '1' && counter == WAIT) {
				if (lb == 1) {
					send_message("Low battery!");
				}
				send_message(response[leak]);
			}
			if (counter <= 0) {
					counter = WAIT;
			}
			else {
			counter--;
			}
			if (stop_key == '1') {
				BT_OFF();
				leak = -1;
				send = 0;
				lb = 0;
				if (~PIND & (1<<PD2)) {
					MCUCR |= 1<<SE;
					MCUCR|=(1<<SM1) | (1<<SM0);
					WDTCR|=(1<<WDCE) | (1<<WDE);
					WDTCR = (1<<WDIE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0);
				}
				else {
					MCUCR |= 1<<SE;
					MCUCR|=(1<<SM1) | (1<<SM0);
					GIMSK|= 1<<INT0;
					MCUCR &= ~((1<<ISC01) | (1<< ISC00));
				}
			}
		}
		asm("sleep");
	}
}
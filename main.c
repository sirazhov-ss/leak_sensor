#define F_CPU 1000000UL
//#define BAUDRATE (F_CPU/(16*4800L))-1
#define BAUDRATE (F_CPU/(16*4800L))
#define WAIT 10000
#define WAIT2 2
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
	DDRA |= ((1<<PA1) | (1<<PA0));
	PORTD &= ~((1<<PD3) | (1<<PD2));
	PORTA &= ~(1<<PA1);
	PORTA |= 1<<PA0;
}

void UART_Init(unsigned int baud){
	UBRRH = (unsigned char)(baud>>8);
	UBRRL = (unsigned char)baud;
	UCSRB = (1<<RXEN)|(1<<TXEN);
	UCSRC = (1<<USBS)|(3<<UCSZ0);
	UCSRB |= (1<<RXCIE);
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

ISR(USART_RX_vect){
	stop_key = UDR;
}

ISR(INT0_vect){
	MCUCR &= ~(1<<SE);
	WDTCR|=(1<<WDCE) | (1<<WDE);
	WDTCR &= ~((1<<WDE) | (1<<WDIE));
	GIMSK &= ~(1<<INT0);
	GIMSK &= ~(1<<INT1);
	send = 1;
	stop_key = '0';
	counter = WAIT;
	BT_ON();
}

ISR(INT1_vect){
	MCUCR &= ~(1<<SE);
	WDTCR|=(1<<WDCE) | (1<<WDE);
	WDTCR &= ~((1<<WDE) | (1<<WDIE));
	GIMSK &= ~(1<<INT0);
	GIMSK &= ~(1<<INT1);
	send = 1;
	stop_key = '0';
	counter = WAIT;
	BT_ON();
}

ISR(WDT_OVERFLOW_vect){
	if ((PIND & (1<<PD2) && leak == 1) || (~PIND & (1<<PD2) && leak == 0) || (PIND & (1<<PD3) && lb == 1) || (~PIND & (1<<PD3) && lb == 0)) {
		MCUCR &= ~(1<<SE);
		WDTCR|=(1<<WDCE) | (1<<WDE);
		WDTCR &= ~((1<<WDE) | (1<<WDIE));
		GIMSK &= ~(1<<INT0);
		GIMSK &= ~(1<<INT1);
		send = 1;
		stop_key = '0';
		counter = WAIT;
		BT_ON();
	}
	if (~PIND & (1<<PD2) && PIND & (1<<PD3)){
		for (int i=0; i<4; i++){
			PORTA ^= 1<<PA0;
			_delay_ms(200);
		}
	}
	if (PIND & (1<<PD2) && ~PIND & (1<<PD3)){
		for (int i=0; i<8; i++){
			PORTA ^= 1<<PA0;
			_delay_ms(200);
		}
	}
	if (~PIND & (1<<PD2) && ~PIND & (1<<PD3)){
		for (int i=0; i<16; i++){
			PORTA ^= 1<<PA0;
			_delay_ms(200);
		}
	}
	asm("wdr");
}

int main(void)
{
	PIN_Init();
	UART_Init(BAUDRATE);
	sei();
	char* response[4] = {"No leak! & GB", "Leak is detected! & GB", "No leak! & LB", "Leak is detected! & LB"};
	_delay_ms(500);
	PORTA |= 1<<PA1;
	_delay_ms(1000);
	stop_key = '0';
	counter = WAIT;
	send = 1;
	int counter2 = WAIT2;
	while (1){
		if (send == 1) {
			leak = get_state(2);
			lb = get_state(3);	
			if (stop_key != '1' && counter == WAIT && counter2 == WAIT2) {
				if (leak == 0 && lb == 0) {
					send_message(response[0]);
				}
				if (leak == 1 && lb == 0) {
					send_message(response[1]);
				}
				if (leak == 0 && lb == 1) {
					send_message(response[2]);
				}
				if (leak == 1 && lb == 1) {
					send_message(response[3]);
				}
			}
			if (counter <= 0) {
				counter = WAIT;
				if (counter2 <= 0) {
					counter2 = WAIT2;
				}
				else {
					counter2--;
				}
			}
			else {
				counter--;
			}
			if (stop_key == '1') {
				BT_OFF();
				send = 0;
				if (PIND & (1<<PD2) && PIND & (1<<PD3)){
					GIMSK |= 1<<INT1;
					MCUCR &= ~((1<<ISC11) | (1<<ISC10));
					GIMSK |= 1<<INT0;
					MCUCR &= ~((1<<ISC01) | (1<<ISC00));
					MCUCR |= 1<<SE;
					MCUCR |= (1<<SM0);
				}
				if (~PIND & (1<<PD2) && PIND & (1<<PD3)) {
					GIMSK |= 1<<INT1;
					MCUCR &= ~((1<<ISC11) | (1<<ISC10));
					WDTCR|=(1<<WDCE) | (1<<WDE);
					WDTCR = (1<<WDIE) | (1<<WDP2) | (1<<WDP1) | (1<<WDP0);
					MCUCR |= 1<<SE;
					MCUCR |= (1<<SM0);
				}
				if (PIND & (1<<PD2) && ~PIND & (1<<PD3)) {
					GIMSK |= 1<<INT0;
					MCUCR &= ~((1<<ISC01) | (1<<ISC00));
					WDTCR|=(1<<WDCE) | (1<<WDE);
					WDTCR = (1<<WDIE) | (1<<WDP3);
					MCUCR |= 1<<SE;
					MCUCR |= (1<<SM0);
				}
				if (~PIND & (1<<PD2) && ~PIND & (1<<PD3)) {
					WDTCR|=(1<<WDCE) | (1<<WDE);
					WDTCR = (1<<WDIE) | (1<<WDP3) | (1<<WDP0);
					MCUCR |= 1<<SE;
					MCUCR |= (1<<SM0);
				}
				
			}
		}
		asm("sleep");
	}
}
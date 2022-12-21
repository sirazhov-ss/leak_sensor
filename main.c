#define F_CPU 1000000UL
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>



int main(void)
{
	DDRA |= ((1<<0) | (1<<1));
	PORTA &= ~(1<<0);
	PORTA |= (1<<1);
	DDRD &= ~((1<<2) | (1<<3));
	PORTD |= ((1<<2) | (1<<3));
	//

    while (1) 
    {
		PORTA ^= ((1<<0) | (1<<1));
		_delay_ms(500);
    }
}


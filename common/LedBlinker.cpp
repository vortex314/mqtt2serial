//_______________________________________________________________________________________________________________
//
#include <LedBlinker.h> 

LedBlinker::LedBlinker(Thread& thr,uint32_t pin, uint32_t delay)
	: Actor(thr),_pin(pin),blinkTimer(thr,delay,true),timerHandler(3,"timerHandler"),blinkSlow(3,"blinkSlow") {

	blinkTimer >> ([&](const TimerMsg tm) {
    digitalWrite(_pin, _on);
		_on = _on ? 0 : 1 ;
	});

	_pin = pin;
	blinkSlow.async(thread(),[&](bool flag) {
		if ( flag ) blinkTimer.interval(500);
		else blinkTimer.interval(100);
	});
}
void LedBlinker::init() {
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, 1);
}

void LedBlinker::delay(uint32_t d) {
	blinkTimer.interval(d);
}
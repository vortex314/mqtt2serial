//_______________________________________________________________________________________________________________
//
#include <Button.h>

Button *Button::_button = 0;

Button::Button(Thread &thr, uint32_t pin) : Actor(thr), ValueFlow<bool>(), _pin(pin)
{
    _button = this;
};

void Button::newValue(bool b)
{
    if (b != _lastState)
    {
        _lastState = b;
        on(b);
    }
}

void IRAM_ATTR Button::isrButton()
{
    if (_button)
        _button->newValue(digitalRead(_button->_pin) == 0);
}
void Button::init()
{
    pinMode(_pin, INPUT_PULLUP);
    attachInterrupt(_pin, isrButton, CHANGE);
};

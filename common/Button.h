//_______________________________________________________________________________________________________________
//
#ifndef _BUTTON_H_
#define _BUTTON_H_
#include <limero.h>
#ifndef IRAM_ATTR
#define IRAM_ATTR 
#endif
class Button : public Actor, public ValueFlow<bool>
{
    uint32_t _pin;
    bool _pinOldValue;
    static Button *_button;
    static Button *_button2;
    bool _lastState = false;

public:
    Button(Thread &thr, uint32_t pin) ;
    void newValue(bool b);
    static void IRAM_ATTR isrButton();
    void init();
};
#endif

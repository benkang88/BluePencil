#ifndef Button_h
#define Button_h
#include "Arduino.h"
enum button_state {S0,S1,S2,S3,S4}; // S0 is unpressed, S1 is unstable press, S2 is short press, S3 is long press, S4 is unstable unpress
class Button {
  public:
    uint32_t S2_start_time;
    uint32_t button_change_time;
    uint32_t last_press;
    uint32_t debounce_duration;
    uint32_t long_press_duration;
    uint8_t pin;
    uint8_t flag;
    uint8_t button_pressed;
    button_state state;
    Button(int p, int long_press=1000);
    int getButtonPress();
    button_state getState();
    void read();
    int update();
};
#endif
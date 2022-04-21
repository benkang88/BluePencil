#include "Arduino.h"
#include "Button.h"

Button::Button(int p, int long_press) {
  flag = 0;  
  state = S0;
  pin = p;
  S2_start_time = millis(); //init
  button_change_time = millis(); //init
  debounce_duration = 10;
  long_press_duration = long_press;
  button_pressed = 0;
  last_press = 0;
}

int Button::getButtonPress() {
  return button_pressed;
}

button_state Button::getState() {
  return state;
}

void Button::read() {
  uint8_t button_val = digitalRead(pin);  
  button_pressed = !button_val;
}

int Button::update() {
  read();
  flag = 0;
  //your code here!<------!!!!
  switch (state) {
    case S0:
      if (button_pressed == 1) {
        state = S1;
        button_change_time = millis();
      }
      break;
    case S1:
      if (button_pressed == 0) {
        state = S0;
        button_change_time = millis();
      }
      else if (button_pressed == 1 && millis() - button_change_time >= debounce_duration) {
        state = S2;
        S2_start_time = millis();
      }
      break;
    case S2:
      if (button_pressed == 1 && millis() - S2_start_time >= long_press_duration) {
        state = S3;
      }
      else if (button_pressed == 0) {
        state = S4;
        button_change_time = millis();
      }
      break;
    case S3:
      if (button_pressed == 0) {
        state = S4;
        button_change_time = millis();
      }
      break;
    case S4:
      if (button_pressed == 0 && millis() - button_change_time >= debounce_duration) {
        state = S0;
        if (millis() - S2_start_time >= long_press_duration + debounce_duration) {
          flag = 2;
        }
        else {
          flag = 1;
        }
      }
      else if (button_pressed == 1) {
        if (millis() - S2_start_time < long_press_duration) {
          state = S2;
        }
        else {
          state = S3;
        }
        button_change_time = millis();
      }
    break;
  }
  return flag;
}
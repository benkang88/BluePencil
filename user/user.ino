#include <WiFi.h> //Connect to WiFi Network
#include <SPI.h>
#include <TFT_eSPI.h>
#include <mpu6050_esp32.h>
#include<math.h>
#include<string.h>

TFT_eSPI tft = TFT_eSPI();
const int SCREEN_HEIGHT = 160;
const int SCREEN_WIDTH = 128;
const int BUTTON_PIN = 45;
const int LOOP_PERIOD = 40;

MPU6050 imu; //imu object called, appropriately, imu

char network[] = "MIT";  //SSID for .08 Lab
char wifi_password[] = ""; //Password for 6.08 Lab
char POST_URL[] = "http://608dev-2.net/sandbox/sc/team39/login.py";

//Some constants and some resources:
const int RESPONSE_TIMEOUT = 6000; //ms to wait for response from host
const uint16_t IN_BUFFER_SIZE = 1000; //size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 1000; //size of buffer to hold HTTP response
char request[IN_BUFFER_SIZE];
char old_response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request
char response[OUT_BUFFER_SIZE]; //char array buffer to hold HTTP request


//pins for LCD and AUDIO CONTROL
uint8_t LCD_CONTROL = 21;
uint8_t AUDIO_TRANSDUCER = 14;

//PWM Channels. The LCD will still be controlled by channel 0, we'll use channel 1 for audio generation
uint8_t LCD_PWM = 0;
uint8_t AUDIO_PWM = 1;

uint32_t primary_timer;

int old_val;

//used to get x,y values from IMU accelerometer!
void get_angle(float* x, float* y) {
  imu.readAccelData(imu.accelCount);
  *x = imu.accelCount[0] * imu.aRes;
  *y = imu.accelCount[1] * imu.aRes;
}

void lookup(char* query, char* response, int response_size) {
  char request_buffer[200];
  //CHANGE WHERE THIS IS TARGETED! IT SHOULD TARGET YOUR SERVER SCRIPT
  //CHANGE WHERE THIS IS TARGETED! IT SHOULD TARGET YOUR SERVER SCRIPT
  //CHANGE WHERE THIS IS TARGETED! IT SHOULD TARGET YOUR SERVER SCRIPT
  sprintf(request_buffer, "GET /sandbox/sc/erives/ex05/wiki_interfacer.py?subject=%s HTTP/1.1\r\n", query);
  strcat(request_buffer, "Host: 608dev-2.net\r\n");
  strcat(request_buffer, "\r\n"); //new line from header to body

  do_http_request("608dev-2.net", request_buffer, response, response_size, RESPONSE_TIMEOUT, true);
}

class StringGetter {
    char alphabet[50] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char query_string[50] = {0};
    int char_index;
    int state;
    uint32_t scroll_timer;
    const int scroll_threshold = 150;
    const float angle_threshold = 0.3;
  public:
    StringGetter() {
      state = 0;
      char_index = 0;
      scroll_timer = millis();
    }
    void update(float angle, int button, char* output) {
      int alphabet_length = 36;
      if(state == 0){
        // printf("angle: %f\nbutton: %d\noutput: %s\nquery_string: %s\n\n", angle, button, output, query_string);
        if(button == 1){
          char temp[100];
          sprintf(temp, "%c", alphabet[char_index]);
          strcat(query_string, temp);
          strcpy(output, query_string);
          char_index = 0;
        }else if(button == 2){
          state = 1;
        }else if(millis() - scroll_timer > scroll_threshold){
          if(angle > angle_threshold){
              char_index = (char_index + 1) % alphabet_length;
              scroll_timer = millis();
          }else if(angle < -angle_threshold){
              char_index = (char_index + alphabet_length - 1) % alphabet_length;
              scroll_timer = millis();
          }
          sprintf(output, "%s%c", query_string, alphabet[char_index]);
        }else{
          sprintf(output, "%s%c", query_string, alphabet[char_index]);
        }
      }
    }
    boolean is_done() {
      Serial.printf("the value of the state is: %d\n", state);
      return (state == 1);
    }
};


char old_username[100]; //for detecting changes
char old_password[100]; //for detecting changes
char username[100]; //for password string
char password[100]; //for username string

StringGetter username_getter; //wikipedia object
StringGetter password_getter; 

//START - indicates both strings are empty
//USERNAME - indicates currently getting username
//PASSWORD - indicates currently getting password
//DONE - indicates we are done with both
enum login_status {START, USERNAME, PASSWORD, DONE}; 
login_status login_state;

Button button(BUTTON_PIN); //button object!

void setup() {
  Serial.begin(115200); //for debugging if needed.
  WiFi.begin(network, wifi_password); //attempt to connect to wifi
  uint8_t count = 0; //count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count < 12) {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected()) { //if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.printf("%d:%d:%d:%d (%s) (%s)\n", WiFi.localIP()[3], WiFi.localIP()[2],
                  WiFi.localIP()[1], WiFi.localIP()[0],
                  WiFi.macAddress().c_str() , WiFi.SSID().c_str());    delay(500);
  } else { //if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }
  if (imu.setupIMU(1)) {
    Serial.println("IMU Connected!");
  } else {
    Serial.println("IMU Not Connected :/");
    Serial.println("Restarting");
    ESP.restart(); // restart the ESP (proper way)
  }
  tft.init();
  tft.setRotation(2);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK); //set color of font to green foreground, black background
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  primary_timer = millis();

  //set up the LCD PWM and set it to 
  pinMode(LCD_CONTROL, OUTPUT);
  ledcSetup(LCD_PWM, 100, 12);//12 bits of PWM precision
  ledcWrite(LCD_PWM, 1000); //0 is a 0% duty cycle for the PFET...increase if you'd like to dim the LCD.
  ledcAttachPin(LCD_CONTROL, LCD_PWM);

  //initialize password and username to zero
  sprintf(username, "");
  sprintf(password, "");
  sprintf(old_username, "");
  sprintf(old_password, "");

  login_state = START;
}

void loop() {
  float x, y;
  get_angle(&x, &y); //get angle values
  Serial.printf("x: %f, y: %f\n", x, y);
  int bv = button.update(); //get button value
  Serial.printf("Current login_state is: %d\n", login_state);
  if(login_state == START){
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0, 1);
    tft.printf("Username: %s\nPassword:%s\n", username, password);
    login_state = USERNAME;
  }else if(login_state == USERNAME){
    username_getter.update(y, bv, username);
    if(username_getter.is_done()){
      login_state = PASSWORD;
    }
  }else if(login_state == PASSWORD){
    password_getter.update(y, bv, password);
    if(password_getter.is_done()){
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0, 1);
      tft.printf("Sending data to server!");
      login_state = DONE;
    }
  }else{
    //state is DONE
    char body[1000]; //for body
    sprintf(body, "username=%s&password=%s", username, password);
    int body_len = strlen(body); //calculate body length (for header reporting)
    sprintf(request, POST_URL);
    strcat(request, "Host: 608dev-2.net\r\n");
    strcat(request, "Content-Type: application/x-www-form-urlencoded\r\n");
    sprintf(request + strlen(request), "Content-Length: %d\r\n", body_len); //append string formatted to end of request buffer
    strcat(request, "\r\n"); //new line from header to body
    strcat(request, body); //body
    strcat(request, "\r\n"); //new line
    do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, true);
  }
  if (strcmp(username, old_username) != 0 || strcmp(password, old_password)) {//only draw if changed!
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(0, 0, 1);
    tft.printf("Username: %s\nPassword:%s\n", username, password);
  }
  strcpy(old_username, username);
  strcpy(old_password, password);
  // memset(old_response, 0, sizeof(old_response));
  // strcat(old_response, response);
  while (millis() - primary_timer < LOOP_PERIOD); //wait for primary timer to increment
  primary_timer = millis();
}



/*----------------------------------
  char_append Function:
  Arguments:
     char* buff: pointer to character array which we will append a
     char c:
     uint16_t buff_size: size of buffer buff

  Return value:
     boolean: True if character appended, False if not appended (indicating buffer full)
*/
uint8_t char_append(char* buff, char c, uint16_t buff_size) {
  int len = strlen(buff);
  if (len > buff_size) return false;
  buff[len] = c;
  buff[len + 1] = '\0';
  return true;
}

/*----------------------------------
   do_http_request Function:
   Arguments:
      char* host: null-terminated char-array containing host to connect to
      char* request: null-terminated char-arry containing properly formatted HTTP request
      char* response: char-array used as output for function to contain response
      uint16_t response_size: size of response buffer (in bytes)
      uint16_t response_timeout: duration we'll wait (in ms) for a response from server
      uint8_t serial: used for printing debug information to terminal (true prints, false doesn't)
   Return value:
      void (none)
*/
void do_http_request(char* host, char* request, char* response, uint16_t response_size, uint16_t response_timeout, uint8_t serial) {
  WiFiClient client; //instantiate a client object
  if (client.connect(host, 80)) { //try to connect to host on port 80
    if (serial) Serial.print(request);//Can do one-line if statements in C without curly braces
    client.print(request);
    memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected()) { //while we remain connected read out data coming back
      client.readBytesUntil('\n', response, response_size);
      if (serial) Serial.println(response);
      if (strcmp(response, "\r") == 0) { //found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis() - count > response_timeout) break;
    }
    memset(response, 0, response_size);
    count = millis();
    while (client.available()) { //read out remaining text (body of response)
      char_append(response, client.read(), OUT_BUFFER_SIZE);
    }
    if (serial) Serial.println(response);
    client.stop();
    if (serial) Serial.println("-----------");
  } else {
    if (serial) Serial.println("connection failed :/");
    if (serial) Serial.println("wait 0.5 sec...");
    client.stop();
  }
}

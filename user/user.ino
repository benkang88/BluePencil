#include <WiFi.h> //Connect to WiFi Network
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <mpu6050_esp32.h>
#include <math.h>
#include <string.h>
#include <ArduinoJson.h>
#include "Button.h"

TFT_eSPI tft = TFT_eSPI();
const int SCREEN_HEIGHT = 160;
const int SCREEN_WIDTH = 128;
const int BUTTON1_PIN = 45;
const int BUTTON2_PIN = 39;
const int BUTTON3_PIN = 38;
const int BUTTON4_PIN = 34;
const int LOOP_PERIOD = 40;

MPU6050 imu; // imu object called, appropriately, imu

char network[] = "MIT";    // SSID for .08 Lab
char wifi_password[] = ""; // Password for 6.08 Lab
// char network[] = "EECS_Labs";  //SSID for .08 Lab
// char wifi_password[] = ""; //Password for 6.08 Lab
char POST_URL[] = "http://608dev-2.net/sandbox/sc/team39/login.py";

// Prefix to POST request:
const char PREFIX[] = "{\"wifiAccessPoints\": [";                 // beginning of json body
const char SUFFIX[] = "]}";                                       // suffix to POST request
const char API_KEY[] = "AIzaSyAQ9SzqkHhV-Gjv-71LohsypXUH447GWX8"; // don't change this and don't share this

char *SERVER = "googleapis.com"; // Server URL

WiFiClientSecure client; // global WiFiClient Secure object
WiFiClient client2;      // global WiFiClient Secure object

// Some constants and some resources:
const int RESPONSE_TIMEOUT = 6000;     // ms to wait for response from host
const uint16_t IN_BUFFER_SIZE = 1000;  // size of buffer to hold HTTP request
const uint16_t OUT_BUFFER_SIZE = 1000; // size of buffer to hold HTTP response
char request[IN_BUFFER_SIZE];
char old_response[OUT_BUFFER_SIZE]; // char array buffer to hold HTTP request
char response[OUT_BUFFER_SIZE];     // char array buffer to hold HTTP request

// pins for LCD and AUDIO CONTROL
uint8_t LCD_CONTROL = 21;
uint8_t AUDIO_TRANSDUCER = 14;

// PWM Channels. The LCD will still be controlled by channel 0, we'll use channel 1 for audio generation
uint8_t LCD_PWM = 0;
uint8_t AUDIO_PWM = 1;

const int HEIGHT = 159;
const int WIDTH = 127;

uint32_t primary_timer;

int old_val;

// used to get x,y values from IMU accelerometer!
void get_angle(float *x, float *y)
{
  imu.readAccelData(imu.accelCount);
  *x = imu.accelCount[0] * imu.aRes;
  *y = imu.accelCount[1] * imu.aRes;
}

class StringGetter
{
  char alphabet[50] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  char query_string[50] = "";
  int char_index;
  int state;
  uint32_t scroll_timer;
  const int scroll_threshold = 150;
  const float angle_threshold = 0.3;

public:
  StringGetter()
  {
    state = 0;
    char_index = 0;
    scroll_timer = millis();
  }
  void update(float angle, int button, char *output, bool update)
  {
    int alphabet_length = 36;
    if (state == 0)
    {
      // printf("angle: %f\nbutton: %d\noutput: %s\nquery_string: %s\n\n", angle, button, output, query_string);
      if (button == 1)
      {
        char temp[100];
        sprintf(temp, "%c", alphabet[char_index]);
        strcat(query_string, temp);
        strcpy(output, query_string);
        char_index = 0;
      }
      else if (button == 2)
      {
        state = 1;
        strcpy(output, query_string);
      }
      else if (millis() - scroll_timer > scroll_threshold)
      {
        if (angle > angle_threshold)
        {
          char_index = (char_index + 1) % alphabet_length;
          scroll_timer = millis();
        }
        else if (angle < -angle_threshold)
        {
          char_index = (char_index + alphabet_length - 1) % alphabet_length;
          scroll_timer = millis();
        }
        if (update)
        {
          sprintf(output, "%s%c", query_string, alphabet[char_index]);
        }
        else
        {
          strcpy(output, query_string);
        }
      }
      else
      {
        if (update)
        {
          sprintf(output, "%s%c", query_string, alphabet[char_index]);
        }
        else
        {
          strcpy(output, query_string);
        }
      }
    }
  }
  boolean is_done()
  {
    return (state == 1);
  }

  void reset()
  {
    // reset
    state = 0;
    char_index = 0;
    sprintf(query_string, "");
  }
};

char old_username[100]; // for detecting changes
char old_password[100]; // for detecting changes
char username[100];     // for password string
char password[100];     // for username string

StringGetter username_getter; // wikipedia object
StringGetter password_getter;

// START - indicates both strings are empty
// USERNAME - indicates currently getting username
// PASSWORD - indicates currently getting password
// POST - indicates currently making post request
// POST_RESULT - displays result of making post requset - loops back to start if necessary
// DONE - indicates we are done with both

uint32_t post_result_timer = 0;
const uint32_t post_result_threshold = 2000;
enum login_status
{
  START,
  USERNAME,
  PASSWORD,
  POST,
  POST_RESULT,
  DONE
};
login_status login_state;

enum checkout_status
{
  SEARCH,
  MAP,
  SELECTED,
  DISPLAY_CODE,
  EXPIRED
};
checkout_status checkout_state;
enum borrow_status
{
  BORROWING,
  NOT_BORROWING
};
borrow_status borrow_state;
uint32_t station_search_timer;
uint32_t fetch_code_timer;
uint32_t check_borrow_timer;
uint32_t borrow_start;
uint32_t borrow_timer;
uint32_t borrow_duration;
uint32_t temp_message_timer;
uint32_t location_post_timer;
const uint32_t fetch_code_wait = 2000;   // time to wait while geting code request
const uint32_t code_status_wait = 1000;  // time to wait between consecutive checks of code status
const uint32_t expired_code_wait = 3000; // amount of time to display code expiration message
const uint32_t location_post_wait = 5000;
const uint32_t check_borrow_wait = 2000;
const uint32_t borrow_wait = 2000;
char unlock_code[10];
char code_status[100];
const uint32_t station_search_period = 15000; // send out GET request for nearby stations every 5 seconds
char selected_station[100];

float longitude;
float latitude;

enum system_status
{
  STARTUP,
  LOGIN,
  WELCOME,
  SELECT,
  CHECKOUT,
  BORROW_VIEW,
  USER_STATS,
  CREDITS
};
system_status system_state;

uint32_t startup_time = 3000; // time in ms to display startup sequence
uint32_t startup_timer;
bool startup = false;

uint32_t welcome_time = 3000;
uint32_t welcome_timer;
bool welcome = false;

int old_system_select;
int system_select;
const int NUM_SETTINGS = 5;
char select_options[NUM_SETTINGS][100] = {"Find stations", "Borrow Status", "User stats", "Credits", "Logout"};

Button button1(BUTTON1_PIN); // button object!
Button button2(BUTTON2_PIN);
Button button3(BUTTON3_PIN);
Button button4(BUTTON4_PIN);

const int max_nearby_stations = 5;
char nearby_stations[max_nearby_stations][100];
int num_nearby_stations = 0;
float distances[max_nearby_stations];
float locs[max_nearby_stations][2];
int station_select = 0;
int search_select = 0;

void location_post()
{
  Serial.printf("POSTING LOCATION!!");
  char body[1000]; // for body
  sprintf(body, "username=%s&lat=%f&lon=%f", username, latitude, longitude);
  int len = strlen(body);
  request[0] = '\0'; // set 0th byte to null
  int offset = sprintf(request + offset, "POST %s?%s  HTTP/1.1\r\n", "http://608dev-2.net/sandbox/sc/team39/user_location.py", body);
  offset += sprintf(request + offset, "Host: 608dev-2.net\r\n");
  offset += sprintf(request + offset, "Content-Type: application/x-www-form-urlencoded\r\n");
  offset += sprintf(request + offset, "cache-control: no-cache\r\n");
  offset += sprintf(request + offset, "Content-Length: %d\r\n\r\n", len);
  offset += sprintf(request + offset, "%s\r\n", body);
  Serial.printf("Request: %s\n\n\n\n", request);
  do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
  Serial.printf("Response: %s\n", response);
}

void clear_nearby_stations()
{
  for (int i = 0; i < max_nearby_stations; i++)
  {
    sprintf(nearby_stations[i], "");
  }
}

void update_nearby_stations()
{
  clear_nearby_stations();
  /*Serial.println('latitude');
  Serial.println(latitude);
  Serial.println('longitude');
  Serial.println(longitude);*/
  sprintf(request, "GET http://608dev-2.net/sandbox/sc/team39/get_nearest_locations.py?lat=%f%&lon=%f&radius=100  HTTP/1.1\r\n", latitude, longitude);
  // sprintf(request, "GET http://608dev-2.net/sandbox/sc/team39/get_nearest_locations.py?lat=%f%&lon=%f&radius=%f  HTTP/1.1\r\n", -71.095, 42.359, 100);
  // Serial.printf("%s", request);
  strcat(request, "Host: 608dev-2.net\r\n"); // add more to the end
  strcat(request, "\r\n");
  sprintf(response, "");
  do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
  sprintf(nearby_stations[0], "STATION");
  int i = 0;
  num_nearby_stations = 0;
  char *cur_val = strtok(response, "'");
  while (floor(i / 6) < max_nearby_stations && cur_val != NULL)
  {
    if (i % 6 == 1)
    {
      strcpy(nearby_stations[(int)floor(i / 6)], cur_val);
      num_nearby_stations++;
    }
    else if (i % 6 == 3)
    {
      locs[(int)floor(i / 6)][0] = atof(cur_val);
    }
    else if (i % 6 == 5)
    {
      locs[(int)floor(i / 6)][1] = atof(cur_val);
    }
    i++;
    cur_val = strtok(NULL, "'");
  }
  Serial.println(num_nearby_stations);
  for (int i = 0; i < num_nearby_stations; i++)
  {
    Serial.printf("%s\n", nearby_stations[i]);
  }
  if (num_nearby_stations == 0)
  {
    station_select = 0;
  }
  else
  {
    station_select %= num_nearby_stations;
  }
}

void display_nearby_stations()
{
  // float temp_locs[5][2] = {{42.359, -71.092}, {42.359, -71.095}, {42.361, -71.093}, {42.356, -71.098}, {42.36, -71.089}};
  // char* temp_nearby_stations[5] = {"Infinite Corridor/Killian", "Student Center", "Vassar Academic Buildings", "Simmons/Briggs", "East Campus"};
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0, 2);
  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  tft.println("BluePencils\n");
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.print(nearby_stations[station_select]);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(60, 85, 1);
  tft.print(".");
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  for (int i = 0; i < max_nearby_stations; i++)
  {
    if (i == station_select)
      continue;
    int x = 60 + ceil(61 * (locs[i][0] - latitude) / 0.01);
    int y = 85 + ceil(61 * (locs[i][1] - longitude) / 0.01);
    Serial.println(x, y);
    tft.setCursor(x, y, 1);
    tft.print(".");
  }
  int x = 60 + ceil(61 * (locs[station_select][0] - latitude) / 0.01);
  int y = 85 + ceil(61 * (locs[station_select][1] - longitude) / 0.01);
  tft.setCursor(x, y, 1);
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.print(".");
}

void display_station_select()
{
  // char* temp_nearby_stations[5] = {"Infinite Corridor/Killian", "Student Center", "Vassar Academic Buildings", "Simmons/Briggs", "East Campus"};
  // Serial.printf("Displaying nearby stations!\n");
  // Serial.printf("station_select: %d\n", station_select);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0, 2);
  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  tft.println("BluePencils\n");
  tft.setTextColor(TFT_RED, TFT_BLACK);
  tft.printf("%s\n\n", nearby_stations[station_select]);
  if (search_select == 0)
  {
    tft.print("[*] SELECT\n    BACK");
  }
  else
  {
    tft.print("    SELECT\n[*] BACK");
  }
}

void get_unlock_code(char selected_station[])
{
  sprintf(request, "GET http://608dev-2.net/sandbox/sc/team39/get_code.py?user=%s&station=%s  HTTP/1.1\r\n", username, selected_station);
  strcat(request, "Host: 608dev-2.net\r\n"); // add more to the end
  strcat(request, "\r\n");
  sprintf(response, "");
  do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
  Serial.printf("response is: %s\n", response);
  sprintf(unlock_code, response);
}

/**
 * updates code status
 * */
void update_code_status()
{
  sprintf(request, "GET http://608dev-2.net/sandbox/sc/team39/code_status.py?user=%s&first=%c&second=%c&third=%c  HTTP/1.1\r\n", username, unlock_code[0], unlock_code[1], unlock_code[2]);
  strcat(request, "Host: 608dev-2.net\r\n"); // add more to the end
  strcat(request, "\r\n");
  sprintf(response, "");
  do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
  printf("response is: %s\n", response);
  sprintf(code_status, response);
  Serial.printf("Code status is: %s\n", code_status);
  Serial.printf("Code status length: %d\n", strlen(code_status));
  for (int i = 0; i < strlen(code_status); i++)
  {
    Serial.printf("Character %d: %d\n", i, code_status[i]);
  }
}

void username_password_post()
{
  char body[1000]; // for body
  sprintf(body, "username=%s&password=%s", username, password);
  int len = strlen(body);
  request[0] = '\0'; // set 0th byte to null
  int offset = sprintf(request + offset, "POST %s?%s  HTTP/1.1\r\n", POST_URL, body);
  offset += sprintf(request + offset, "Host: 608dev-2.net\r\n");
  offset += sprintf(request + offset, "Content-Type: application/x-www-form-urlencoded\r\n");
  offset += sprintf(request + offset, "cache-control: no-cache\r\n");
  offset += sprintf(request + offset, "Content-Length: %d\r\n\r\n", len);
  offset += sprintf(request + offset, "%s\r\n", body);
  Serial.printf("Request: %s\n\n\n\n", request);
  do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
  Serial.printf("Response: %s\n", response);
}

uint32_t check_stats_timer;
const uint32_t check_stats_wait = 3000;
char user_stats[500];
void update_user_stats()
{
  sprintf(request, "GET http://608dev-2.net/sandbox/sc/team39/get_stats.py?user=%s  HTTP/1.1\r\n", username);
  strcat(request, "Host: 608dev-2.net\r\n"); // add more to the end
  strcat(request, "\r\n");
  sprintf(response, "");
  do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
  printf("response is: %s\n", response);
  sprintf(user_stats, response);
}

char borrow_status[100] = "NO";
void update_borrow_status()
{
  sprintf(request, "GET http://608dev-2.net/sandbox/sc/team39/return_pencil.py?user=%s  HTTP/1.1\r\n", username);
  strcat(request, "Host: 608dev-2.net\r\n"); // add more to the end
  strcat(request, "\r\n");
  sprintf(response, "");
  do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
  printf("response is: %s\n", response);
  sprintf(borrow_status, response);
}

void display_user_stats()
{
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0, 2);
  tft.printf("%s\nPress any button to exit.\n", user_stats);
}

void display_credits()
{
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0, 2);
  tft.println("Designed by Ben Kang, Allen Ding, Ezra Erives, Sid Muppalla, and Sebastian Zhu in Cambridge, Massachusetts.\n\nPress any button to exit.\n");
}

void setup()
{
  Serial.begin(115200);               // for debugging if needed.
  WiFi.begin(network, wifi_password); // attempt to connect to wifi
  uint8_t count = 0;                  // count used for Wifi check times
  Serial.print("Attempting to connect to ");
  Serial.println(network);
  while (WiFi.status() != WL_CONNECTED && count < 12)
  {
    delay(500);
    Serial.print(".");
    count++;
  }
  delay(2000);
  if (WiFi.isConnected())
  { // if we connected then print our IP, Mac, and SSID we're on
    Serial.println("CONNECTED!");
    Serial.printf("%d:%d:%d:%d (%s) (%s)\n", WiFi.localIP()[3], WiFi.localIP()[2],
                  WiFi.localIP()[1], WiFi.localIP()[0],
                  WiFi.macAddress().c_str(), WiFi.SSID().c_str());
    delay(500);
  }
  else
  { // if we failed to connect just Try again.
    Serial.println("Failed to Connect :/  Going to restart");
    Serial.println(WiFi.status());
    ESP.restart(); // restart the ESP (proper way)
  }
  if (imu.setupIMU(1))
  {
    Serial.println("IMU Connected!");
  }
  else
  {
    Serial.println("IMU Not Connected :/");
    Serial.println("Restarting");
    ESP.restart(); // restart the ESP (proper way)
  }
  tft.init();
  tft.setRotation(2);
  tft.setTextSize(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_BLUE, TFT_BLACK); // set color of font to green foreground, black background
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);
  pinMode(BUTTON4_PIN, INPUT_PULLUP);
  primary_timer = millis();

  // set up the LCD PWM and set it to
  pinMode(LCD_CONTROL, OUTPUT);
  ledcSetup(LCD_PWM, 100, 12); // 12 bits of PWM precision
  ledcWrite(LCD_PWM, 1000);    // 0 is a 0% duty cycle for the PFET...increase if you'd like to dim the LCD.
  ledcAttachPin(LCD_CONTROL, LCD_PWM);

  // initialize password and username to zero
  sprintf(username, "");
  sprintf(password, "");
  sprintf(old_username, "");
  sprintf(old_password, "");

  login_state = START;
  checkout_state = SEARCH;
  system_state = STARTUP;
  borrow_state = NOT_BORROWING;

  station_search_timer = 0;
  startup_timer = millis();
  check_borrow_timer = millis();
  borrow_timer = millis();
  check_stats_timer = millis();
  location_post_timer = millis();
}

void loop()
{
  int bv1 = button1.update(); // get button value
  int bv2 = button2.update();
  int bv3 = button3.update();
  int bv4 = button4.update();
  int bv = 0;
  if (bv1 > 0)
    bv = 1;
  else if (bv2 > 0)
    bv = 2;
  // Serial.printf("Current button value is: %d\n", bv);

  if (system_state != STARTUP && system_state != LOGIN && millis() - location_post_timer > location_post_wait)
  {
    location_post();
    update_nearby_stations();
    location_post_timer = millis();
  }

  if (system_state == STARTUP)
  {
    if (!startup)
    {
      startup = true;
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0, 2);
      tft.println("BluePencilsTM\n");
      tft.print("Loading...");
    }
    if (millis() - startup_timer > startup_time)
    {
      system_state = LOGIN;
    }
  }

  if (system_state == LOGIN)
  {
    float x, y;
    get_angle(&x, &y); // get angle values

    if (login_state == START)
    {
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0, 2);
      tft.setTextColor(TFT_BLUE, TFT_BLACK);
      tft.println("BluePencil");
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.printf("Username:%s", username);
      login_state = USERNAME;
    }

    else if (login_state == USERNAME)
    {
      username_getter.update(y, bv, username, millis() % 1000 < 500);
      if (username_getter.is_done())
      {
        login_state = PASSWORD;
        username_getter.reset();
      }
    }

    else if (login_state == PASSWORD)
    {
      password_getter.update(y, bv, password, millis() % 1000 < 500);
      if (password_getter.is_done())
      {
        // tft.fillScreen(TFT_BLACK);
        // tft.setCursor(0, 0, 1);
        // tft.printf("Sending data to server!");
        login_state = POST;
        password_getter.reset();
      }
    }

    else if (login_state == POST)
    {
      // state is DONE
      username_password_post();
      // transition to POST_RESULT state and display post result
      login_state = POST_RESULT;
      post_result_timer = millis();
      // tft.fillScreen(TFT_BLACK);
      // tft.setCursor(0, 0, 1);
      // tft.printf("%s\n", response);
      // if response is bad (wrong password) --> login_state = USERNAME, reset username and password strings
      // else if response is good, (welcome back or welcome new user) --> login_state = DONE
    }

    else if (login_state == POST_RESULT)
    {
      if (millis() - post_result_timer > post_result_threshold)
      {
        const char bad_response[] = "Incorrect password!\n";
        // Serial.printf("bad_resp len: %d\n", strlen(bad_response));
        // Serial.printf("resp len: %d\n", strlen(response));
        // Serial.printf("second to last char of response: %d\n", response[18]);
        // Serial.printf("last character of response: %d\n", response[19]);
        if (strcmp(response, bad_response) == 0)
        {
          // reset username and password
          sprintf(username, "");
          sprintf(password, "");
          // return to done state
          login_state = START;
          Serial.printf("bad password\n!");
        }
        else
        {
          login_state = DONE;
          Serial.printf("good response!");
        }
      }
    }

    else if (login_state == DONE)
    {
      system_state = WELCOME;
      welcome_timer = millis();
    }
    if (strcmp(username, old_username) != 0 || strcmp(password, old_password) != 0)
    { // only draw if changed!
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0, 2);
      if (login_state == USERNAME)
      {
        tft.setTextColor(TFT_BLUE, TFT_BLACK);
        tft.println("BluePencil\n");
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.printf("Username:\n%s", username);
      }
      else if (login_state == PASSWORD)
      {
        tft.setTextColor(TFT_BLUE, TFT_BLACK);
        tft.println("BluePencils\n");
        tft.printf("Username:\n%s\n", username);
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.printf("Password:\n%s\n", password);
      }
      else
      {
        tft.setTextColor(TFT_BLUE, TFT_BLACK);
        tft.println("BluePencils\n");
        tft.printf("Username:\n%s\n", username);
        tft.printf("Password:\n%s\n", password);
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.println("Processing...");
      }
    }
    strcpy(old_username, username);
    strcpy(old_password, password);
    // memset(old_response, 0, sizeof(old_response));
    // strcat(old_response, response);
  }

  else if (system_state == WELCOME)
  {
    if (!welcome)
    {
      welcome = true;
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_BLUE, TFT_BLACK);
      tft.setCursor(0, 0, 2);
      tft.println("BluePencil\n");
      tft.printf("Welcome to BluePencils, %s!", username);
    }
    if (millis() - welcome_timer > welcome_time)
    {
      system_state = SELECT;
      old_system_select = -1;
      system_select = 0;
    }
  }

  else if (system_state == SELECT)
  {
    if (old_system_select != system_select)
    {
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_BLUE, TFT_BLACK);
      tft.setCursor(0, 0, 2);
      tft.println("BluePencils\n");
      tft.printf("User: %s\n\n", username);
      tft.setTextColor(TFT_RED, TFT_BLACK);
      for (int i = 0; i < NUM_SETTINGS; i++)
      {
        if (system_select == i)
        {
          tft.print("[*] ");
        }
        else
        {
          tft.print("    ");
        }
        tft.printf("%s\n", select_options[i]);
      }
    }

    old_system_select = system_select;
    if (bv > 0)
    {
      if (system_select == 0)
      {
        system_state = CHECKOUT;
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0, 2);
        tft.setTextColor(TFT_BLUE, TFT_BLACK);
        tft.println("BluePencils\n");
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.print("Fetching nearby stations...");
      }
      else if (system_select == 1)
      {
        system_state = BORROW_VIEW;
      }
      else if (system_select == 2)
      {
        system_state = USER_STATS;
        update_user_stats();
        display_user_stats();
      }
      else if (system_select == 3)
      {
        system_state = CREDITS;
        display_credits();
      }
      else if (system_select == 4)
      {
        login_state = START;
        system_state = LOGIN;
        welcome = false;
        strcpy(old_username, "");
        strcpy(username, "");
        strcpy(old_password, "");
        strcpy(password, "");
      }
    }
    else if (bv3 > 0)
    {
      system_select = (system_select + 1) % NUM_SETTINGS;
    }
    else if (bv4 > 0)
    {
      system_select = (system_select + NUM_SETTINGS - 1) % NUM_SETTINGS;
    }
  }

  else if (system_state == CHECKOUT)
  {
    // Serial.printf("Current button value is: %d\n", bv);
    if (checkout_state == SEARCH)
    {
      if (millis() - station_search_timer > station_search_period)
      {
        Serial.printf("doing GET stuff now at %d\n", millis());
        get_latitude_longitude(&latitude, &longitude);
        update_nearby_stations();
        // num_nearby_stations = 5;
        display_nearby_stations();
        station_search_timer = millis();
      }
      // Serial.printf("Current button value is: %d\n", bv);
      if (bv3 > 0)
      {
        station_select = (num_nearby_stations == 0) ? 0 : (station_select + 1) % num_nearby_stations;
        display_nearby_stations();
      }
      else if (bv4 > 0)
      {
        station_select = (num_nearby_stations == 0) ? 0 : (station_select + num_nearby_stations - 1) % num_nearby_stations;
        display_nearby_stations();
      }
      else if (bv == 1)
      {
        display_station_select();
        checkout_state = MAP;
      }
      else if (bv == 2)
      {
        system_state = SELECT;
        old_system_select = -1;
        system_select = 0;
      }
    }

    else if (checkout_state == MAP)
    {
      if (bv3 > 0 || bv4 > 0)
      {
        search_select = (search_select + 1) % 2;
        display_station_select();
      }
      else if (bv > 0)
      {
        if (search_select == 0)
        {
          sprintf(selected_station, "%s", nearby_stations[station_select]);
          Serial.printf("station is now: %s\n", selected_station);
          tft.fillScreen(TFT_BLACK);
          tft.setCursor(0, 0, 2);
          tft.setTextColor(TFT_BLUE, TFT_BLACK);
          tft.println("BluePencils\n");
          tft.setTextColor(TFT_RED, TFT_BLACK);
          tft.printf("Fetching code for station: %s", selected_station);
          get_unlock_code(selected_station);
          fetch_code_timer = millis();
          checkout_state = SELECTED;
        }
        else
        {
          search_select = 0;
          checkout_state = SEARCH;
          display_nearby_stations();
        }
      }
    }
    else if (checkout_state == SELECTED)
    {
      if (millis() - fetch_code_timer > fetch_code_wait)
      {
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0, 2);
        tft.setTextColor(TFT_BLUE, TFT_BLACK);
        tft.println("BluePencils\n");
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.printf("Unlock code is: %s\n", unlock_code);
        checkout_state = DISPLAY_CODE;
        temp_message_timer = millis();
      }
    }
    else if (checkout_state == DISPLAY_CODE)
    {
      if (millis() - temp_message_timer > code_status_wait)
      {
        update_code_status();
        temp_message_timer = millis();
      }
      if (strcmp(code_status, "USED\n") == 0)
      {
        // tft.printf(code was successfully used --> move state to start tracking the trip
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0, 2);
        tft.setTextColor(TFT_BLUE, TFT_BLACK);
        tft.println("BluePencils\n");
        tft.printf("Code entered successfully!");
        checkout_state = SEARCH;
        system_state = BORROW_VIEW;
        sprintf(borrow_status, "YES\n");
        system_state = BORROW_VIEW;
        borrow_state = BORROWING;
        borrow_start = millis();
      }
      else if (strcmp(code_status, "EXPIRED\n") == 0)
      {
        // tft.printf your code has expired --> return to SELECT station
        tft.fillScreen(TFT_BLACK);
        tft.setCursor(0, 0, 2);
        tft.setTextColor(TFT_BLUE, TFT_BLACK);
        tft.println("BluePencils\n");
        tft.printf("Code has expired!");
        checkout_state = EXPIRED;
        temp_message_timer = millis();
      }
    }
    else if (checkout_state == EXPIRED)
    {
      // remain in this state until expired_wait_seconds have passed
      if (millis() - temp_message_timer > expired_code_wait)
      {
        checkout_state = SEARCH;
      }
    }
  }
  else if (system_state == BORROW_VIEW)
  {
    if (borrow_state == BORROWING)
    {
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0, 2);
      const int num_seconds = (millis() - borrow_start) / 1000;
      tft.printf("Currently borrowing a pencil!\nCurrent borrow duration: %d seconds.\n", num_seconds);
    }
    else if (borrow_state == NOT_BORROWING)
    {
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0, 2);
      tft.printf("Not currently borrowing a pencil!");
    }
    if (bv1 > 0 || bv2 > 0 || bv3 > 0 || bv4 > 0)
    {
      system_state = SELECT;
      old_system_select = -1;
      system_select = 0;
    }
  }
  else if (system_state == USER_STATS)
  {
    // when looking for a closer station
    if (millis() - check_stats_timer > check_stats_wait)
    {
      update_user_stats();
      display_user_stats();
      check_stats_timer = millis();
    }
    if (bv1 > 0 || bv2 > 0 || bv3 > 0 || bv4 > 0)
    {
      system_state = SELECT;
      old_system_select = -1;
      system_select = 0;
    }
  }

  else if (system_state == CREDITS)
  {
    if (bv1 > 0 || bv2 > 0 || bv3 > 0 || bv4 > 0)
    {
      system_state = SELECT;
      old_system_select = -1;
      system_select = 0;
    }
  }

  // monitor for borrowing session to end
  if (millis() - check_borrow_timer > check_borrow_wait)
  {
    check_borrow_timer = millis();
    update_borrow_status();
    if (strcmp(borrow_status, "YES\n"))
    {
      // borrow has ended
      borrow_state = NOT_BORROWING;
    }
    else
    {
      borrow_state = BORROWING;
    }
  }

  while (millis() - primary_timer < LOOP_PERIOD)
    ; // wait for primary timer to increment

  primary_timer = millis();
}

/*----------------------------------
   wifi_object_builder: generates a json-compatible entry for use with Google's geolocation API
   Arguments:
    * `char* object_string`: a char pointer to a location that can be used to build a c-string with a fully-contained JSON-compatible entry for one WiFi access point
    *  `uint32_t os_len`: a variable informing the function how much  space is available in the buffer
    * `uint8_t channel`: a value indicating the channel of WiFi operation (1 to 14)
    * `int signal_strength`: the value in dBm of the Access point
    * `uint8_t* mac_address`: a pointer to the six long array of `uint8_t` values that specifies the MAC address for the access point in question.

      Return value:
      the length of the object built. If not entry is written,
*/
int wifi_object_builder(char *object_string, uint32_t os_len, uint8_t channel, int signal_strength, uint8_t *mac_address)
{
  char new_object_string[300];
  sprintf(new_object_string, "{\"macAddress\": \"%02x:%02x:%02x:%02x:%02x:%02x\",\"signalStrength\": %d,\"age\": 0,\"channel\": %d}", mac_address[0], mac_address[1], mac_address[2], mac_address[3], mac_address[4], mac_address[5], signal_strength, channel);
  if (strlen(new_object_string) <= os_len)
  {
    strcpy(object_string, new_object_string);
    return strlen(new_object_string);
  }
  else
  {
    return 0;
  }
}

void get_latitude_longitude(float *latitude, float *longitude)
{
  char json_body[1000]; // for body
  char pruned_response[OUT_BUFFER_SIZE];
  const uint16_t JSON_BODY_SIZE = 3000;
  const int MAX_APS = 5;
  int offset = sprintf(json_body, "%s", PREFIX);
  int n = WiFi.scanNetworks(); // run a new scan. could also modify to use original scan from setup so quicker (though older info)
  // Serial.println("scan done");
  if (n == 0)
  {
    Serial.println("no networks found");
  }
  else
  {
    int max_aps = max(min(MAX_APS, n), 1);
    for (int i = 0; i < max_aps; ++i)
    {                                                                                                                           // for each valid access point
      uint8_t *mac = WiFi.BSSID(i);                                                                                             // get the MAC Address
      offset += wifi_object_builder(json_body + offset, JSON_BODY_SIZE - offset, WiFi.channel(i), WiFi.RSSI(i), WiFi.BSSID(i)); // generate the query
      if (i != max_aps - 1)
      {
        offset += sprintf(json_body + offset, ","); // add comma between entries except trailing.
      }
    }
    sprintf(json_body + offset, "%s", SUFFIX);
    // Serial.println(json_body);
    int len = strlen(json_body);
    // Make a HTTP request:
    // Serial.println("SENDING REQUEST");
    request[0] = '\0'; // set 0th byte to null
    offset = 0;        // reset offset variable for sprintf-ing
    offset += sprintf(request + offset, "POST https://www.googleapis.com/geolocation/v1/geolocate?key=%s  HTTP/1.1\r\n", API_KEY);
    offset += sprintf(request + offset, "Host: googleapis.com\r\n");
    offset += sprintf(request + offset, "Content-Type: application/json\r\n");
    offset += sprintf(request + offset, "cache-control: no-cache\r\n");
    offset += sprintf(request + offset, "Content-Length: %d\r\n\r\n", len);
    offset += sprintf(request + offset, "%s\r\n", json_body);
    do_https_request(SERVER, request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
    // Serial.println("-----------");
    // Serial.println(response);
    // Serial.println("-----------");
    int left_paren = '{';
    int right_paren = '}';
    char *left_loc = strchr(response, left_paren);
    char *right_loc = strrchr(response, right_paren);
    size_t length = right_loc - left_loc + 1;
    memcpy(pruned_response, left_loc, length);
    // Serial.println(pruned_response);
    // For Part Two of Lab04B, you should start working here. Create a DynamicJsonDoc of a reasonable size (few hundred bytes at least...)
    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, pruned_response);
    // Test if parsing succeeds.
    if (error)
    {
      Serial.print(F("deserializeJson() failed: "));
      Serial.println(error.f_str());
      return;
    }
    *latitude = doc["location"]["lat"];
    *longitude = doc["location"]["lng"];
    // add blank line!
    // submit to function that performs GET.  It will return output using response_buffer char array
    // Serial.printf("Current Location: \n Lat: %f \n Lon: %f \n", *latitude, *longitude);
  }
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
uint8_t char_append(char *buff, char c, uint16_t buff_size)
{
  int len = strlen(buff);
  if (len > buff_size)
    return false;
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
void do_http_request(char *host, char *request, char *response, uint16_t response_size, uint16_t response_timeout, uint8_t serial)
{
  WiFiClient client; // instantiate a client object
  if (client.connect(host, 80))
  { // try to connect to host on port 80
    if (serial)
      Serial.print(request); // Can do one-line if statements in C without curly braces
    client.print(request);
    memset(response, 0, response_size); // Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected())
    { // while we remain connected read out data coming back
      client.readBytesUntil('\n', response, response_size);
      if (serial)
        Serial.println(response);
      if (strcmp(response, "\r") == 0)
      { // found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis() - count > response_timeout)
        break;
    }
    memset(response, 0, response_size);
    count = millis();
    while (client.available())
    { // read out remaining text (body of response)
      char_append(response, client.read(), OUT_BUFFER_SIZE);
    }
    if (serial)
      Serial.println(response);
    client.stop();
    if (serial)
      Serial.println("-----------");
  }
  else
  {
    if (serial)
      Serial.println("connection failed :/");
    if (serial)
      Serial.println("wait 0.5 sec...");
    client.stop();
  }
}

/*https communication requires certificates to be known ahead of time so each entity will trust one another.
   This is a cert for google's generic servers below, allowing us to connect over https with their resources
*/
const char *CA_CERT =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDdTCCAl2gAwIBAgILBAAAAAABFUtaw5QwDQYJKoZIhvcNAQEFBQAwVzELMAkG\n"
    "A1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24gbnYtc2ExEDAOBgNVBAsTB1Jv\n"
    "b3QgQ0ExGzAZBgNVBAMTEkdsb2JhbFNpZ24gUm9vdCBDQTAeFw05ODA5MDExMjAw\n"
    "MDBaFw0yODAxMjgxMjAwMDBaMFcxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9i\n"
    "YWxTaWduIG52LXNhMRAwDgYDVQQLEwdSb290IENBMRswGQYDVQQDExJHbG9iYWxT\n"
    "aWduIFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDaDuaZ\n"
    "jc6j40+Kfvvxi4Mla+pIH/EqsLmVEQS98GPR4mdmzxzdzxtIK+6NiY6arymAZavp\n"
    "xy0Sy6scTHAHoT0KMM0VjU/43dSMUBUc71DuxC73/OlS8pF94G3VNTCOXkNz8kHp\n"
    "1Wrjsok6Vjk4bwY8iGlbKk3Fp1S4bInMm/k8yuX9ifUSPJJ4ltbcdG6TRGHRjcdG\n"
    "snUOhugZitVtbNV4FpWi6cgKOOvyJBNPc1STE4U6G7weNLWLBYy5d4ux2x8gkasJ\n"
    "U26Qzns3dLlwR5EiUWMWea6xrkEmCMgZK9FGqkjWZCrXgzT/LCrBbBlDSgeF59N8\n"
    "9iFo7+ryUp9/k5DPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8E\n"
    "BTADAQH/MB0GA1UdDgQWBBRge2YaRQ2XyolQL30EzTSo//z9SzANBgkqhkiG9w0B\n"
    "AQUFAAOCAQEA1nPnfE920I2/7LqivjTFKDK1fPxsnCwrvQmeU79rXqoRSLblCKOz\n"
    "yj1hTdNGCbM+w6DjY1Ub8rrvrTnhQ7k4o+YviiY776BQVvnGCv04zcQLcFGUl5gE\n"
    "38NflNUVyRRBnMRddWQVDf9VMOyGj/8N7yy5Y0b2qvzfvGn9LhJIZJrglfCm7ymP\n"
    "AbEVtQwdpf5pLGkkeB6zpxxxYu7KyJesF12KwvhHhm4qxFYxldBniYUr+WymXUad\n"
    "DKqC5JlR3XC321Y9YeRq4VzW9v493kHMB65jUr9TU/Qr6cf9tveCX4XSQRjbgbME\n"
    "HMUfpIBvFSDJ3gyICh3WZlXi/EjJKSZp4A==\n"
    "-----END CERTIFICATE-----\n";

/*----------------------------------
   do_https_request Function. Similar to do_http_request, but supports https requests
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
void do_https_request(char *host, char *request, char *response, uint16_t response_size, uint16_t response_timeout, uint8_t serial)
{
  client.setHandshakeTimeout(30);
  client.setCACert(CA_CERT); // set cert for https
  if (client.connect(host, 443, 4000))
  { // try to connect to host on port 443
    if (serial)
      Serial.print(request); // Can do one-line if statements in C without curly braces
    client.print(request);
    response[0] = '\0';
    // memset(response, 0, response_size); //Null out (0 is the value of the null terminator '\0') entire buffer
    uint32_t count = millis();
    while (client.connected())
    { // while we remain connected read out data coming back
      client.readBytesUntil('\n', response, response_size);
      if (serial)
        Serial.println(response);
      if (strcmp(response, "\r") == 0)
      { // found a blank line!
        break;
      }
      memset(response, 0, response_size);
      if (millis() - count > response_timeout)
        break;
    }
    memset(response, 0, response_size);
    count = millis();
    while (client.available())
    { // read out remaining text (body of response)
      char_append(response, client.read(), OUT_BUFFER_SIZE);
    }
    if (serial)
      Serial.println(response);
    client.stop();
    if (serial)
      Serial.println("-----------");
  }
  else
  {
    if (serial)
      Serial.println("connection failed :/");
    if (serial)
      Serial.println("wait 0.5 sec...");
    client.stop();
  }
}

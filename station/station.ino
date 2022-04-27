#include <WiFi.h> //Connect to WiFi Network
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <mpu6050_esp32.h>
#include <ArduinoJson.h>
#include <math.h>
#include <string.h>
#include "Button.h"

TFT_eSPI tft = TFT_eSPI();
const int SCREEN_HEIGHT = 160;
const int SCREEN_WIDTH = 128;
const int BUTTON_PIN1 = 45;
const int BUTTON_PIN2 = 39;
const int BUTTON_PIN3 = 34;
const int LOOP_PERIOD = 40;
const int POST_LOCATION_PERIOD = 60000;
const int ROTATION_PERIOD = 450;
float lat;
float lon;

MPU6050 imu; // imu object called, appropriately, imu

// char network[] = "MIT";    // SSID for .08 Lab
// char wifi_password[] = ""; // Password for 6.08 Lab
char network[] = "MIT";  //SSID for .08 Lab
char wifi_password[] = ""; //Password for 6.08 Lab
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
const char STATION_NAME[] = "STATION";
char request[IN_BUFFER_SIZE];
char old_response[OUT_BUFFER_SIZE]; // char array buffer to hold HTTP request
char response[OUT_BUFFER_SIZE];     // char array buffer to hold HTTP request

// pins for LCD and AUDIO CONTROL
uint8_t LCD_CONTROL = 21;
uint8_t AUDIO_TRANSDUCER = 14;
uint8_t MOTOR1 = 11; 
uint8_t MOTOR2 = 10;

// PWM Channels. The LCD will still be controlled by channel 0, we'll use channel 1 for audio generation
uint8_t LCD_PWM = 0;
uint8_t AUDIO_PWM = 1;
// uint8_t MOTOR_PWM = 2;

uint32_t primary_timer;
uint32_t rotation_timer;

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
  char query_string[50] = {0};
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
  void update(float angle, int button, char *output)
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
        sprintf(output, "%s%c", query_string, alphabet[char_index]);
      }
      else
      {
        sprintf(output, "%s%c", query_string, alphabet[char_index]);
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

enum system_status
{
  LOGIN,
  STATION_FINDER,
  USER_STATS
};
system_status system_state;

Button button1(BUTTON_PIN1); // button object!
Button button2(BUTTON_PIN2);
Button button3(BUTTON_PIN3);

enum station_status
{
  IDLE,
  WAITING_FOR_CODE_DIGIT_1,
  WAITING_FOR_CODE_DIGIT_2,
  WAITING_FOR_CODE_DIGIT_3,
  CHECK_CODE,
  UNLOCKED
};
station_status station_state;
int post_location_timer;
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
  tft.setTextColor(TFT_GREEN, TFT_BLACK); // set color of font to green foreground, black background
  pinMode(BUTTON_PIN1, INPUT_PULLUP);
  pinMode(BUTTON_PIN2, INPUT_PULLUP);
  pinMode(BUTTON_PIN3, INPUT_PULLUP);
  primary_timer = millis();

  // set up the LCD PWM and set it to
  pinMode(LCD_CONTROL, OUTPUT);
  ledcSetup(LCD_PWM, 100, 12); // 12 bits of PWM precision
  ledcWrite(LCD_PWM, 1000);    // 0 is a 0% duty cycle for the PFET...increase if you'd like to dim the LCD.
  ledcAttachPin(LCD_CONTROL, LCD_PWM);
  pinMode(MOTOR1, OUTPUT);
  pinMode(MOTOR2, OUTPUT);


  station_state = WAITING_FOR_CODE_DIGIT_1;
  post_location_timer = millis();
  lat = 0;
  lon = 0;
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0, 1);
  tft.printf("Enter code: ");
}

bool valid_code = false;
int code_digit_1;
int code_digit_2;
int code_digit_3;

void post_location(float lat, float lon)
{
  char body[1000]; // for body
  sprintf(body, "station=%s&lat=%f&lon=%f", STATION_NAME, lat, lon);
  int len = strlen(body);
  request[0] = '\0'; // set 0th byte to null
  int offset = sprintf(request + offset, "POST %s?%s  HTTP/1.1\r\n", "http://608dev-2.net/sandbox/sc/team39/get_nearest_locations.py", body);
  offset += sprintf(request + offset, "Host: 608dev-2.net\r\n");
  offset += sprintf(request + offset, "Content-Type: application/x-www-form-urlencoded\r\n");
  offset += sprintf(request + offset, "cache-control: no-cache\r\n");
  offset += sprintf(request + offset, "Content-Length: %d\r\n\r\n", len);
  offset += sprintf(request + offset, "%s\r\n", body);
  Serial.printf("Request: %s\n\n\n\n", request);
  sprintf(response, "");
  do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
  Serial.printf("Response: %s", response);
}

bool check_input_code(int dig1, int dig2, int dig3)
{
  sprintf(request, "GET http://608dev-2.net/sandbox/sc/team39/code_checker.py?station=%s&first=%d%&second=%d&third=%d  HTTP/1.1\r\n", STATION_NAME, dig1, dig2, dig3);
  Serial.printf("%s", request);
  strcat(request, "Host: 608dev-2.net\r\n"); // add more to the end
  strcat(request, "\r\n");
  sprintf(response, "");
  do_http_request("608dev-2.net", request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
  response[3] = '\0';
  return (strcmp("YES", response) == 0) ? true : false;
}

void loop()
{
  if (millis() - post_location_timer > POST_LOCATION_PERIOD)
  {
    get_latitude_longitude(&lat, &lon);
    post_location(lat, lon);
    post_location_timer = millis();
  }
  if (station_state == WAITING_FOR_CODE_DIGIT_1)
  {
    if (button1.update() == 1)
    {
      code_digit_1 = 1;
      station_state = WAITING_FOR_CODE_DIGIT_2;
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0, 1);
      tft.printf("%d", code_digit_1);
    }
    else if (button2.update() == 1)
    {
      code_digit_1 = 2;
      station_state = WAITING_FOR_CODE_DIGIT_2;
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0, 1);
      tft.printf("%d", code_digit_1);
    }
    else if (button3.update() == 1)
    {
      code_digit_1 = 3;
      station_state = WAITING_FOR_CODE_DIGIT_2;
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0, 1);
      tft.printf("%d", code_digit_1);
    }
  }
  else if (station_state == WAITING_FOR_CODE_DIGIT_2)
  {
    if (button1.update() == 1)
    {
      code_digit_2 = 1;
      station_state = WAITING_FOR_CODE_DIGIT_3;
      tft.printf("%d", code_digit_2);
    }
    else if (button2.update() == 1)
    {
      code_digit_2 = 2;
      station_state = WAITING_FOR_CODE_DIGIT_3;
      tft.printf("%d", code_digit_2);
    }
    else if (button3.update() == 1)
    {
      code_digit_2 = 3;
      station_state = WAITING_FOR_CODE_DIGIT_3;
      tft.printf("%d", code_digit_2);
    }
  }
  else if (station_state == WAITING_FOR_CODE_DIGIT_3)
  {
    if (button1.update() == 1)
    {
      code_digit_3 = 1;
      station_state = CHECK_CODE;
      tft.printf("%d", code_digit_3);
    }
    else if (button2.update() == 1)
    {
      code_digit_3 = 2;
      station_state = CHECK_CODE;
      tft.printf("%d", code_digit_3);
    }
    else if (button3.update() == 1)
    {
      code_digit_3 = 3;
      station_state = CHECK_CODE;
      tft.printf("%d", code_digit_3);
    }
  }
  else if (station_state == CHECK_CODE)
  {
    valid_code = check_input_code(code_digit_1, code_digit_2, code_digit_3);
    if (valid_code)
    {
      Serial.println("unlock");
      station_state = UNLOCKED;
      rotation_timer = millis();
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0, 1);
      tft.printf("UNLOCKED");
      digitalWrite(MOTOR1, HIGH);
      digitalWrite(MOTOR2, LOW);
    }
    else
    {
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0, 1);
      tft.printf("BAD CODE");
      station_state = WAITING_FOR_CODE_DIGIT_1;
    }
  }
  else if (station_state == UNLOCKED)
  {
    if(millis() - rotation_timer > ROTATION_PERIOD){
      digitalWrite(MOTOR1, LOW);
      digitalWrite(MOTOR2, LOW);
      station_state = WAITING_FOR_CODE_DIGIT_1;
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0, 1);
      tft.printf("Enter code: ");
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
  Serial.println("scan done");
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
    Serial.println(json_body);
    int len = strlen(json_body);
    // Make a HTTP request:
    Serial.println("SENDING REQUEST");
    request[0] = '\0'; // set 0th byte to null
    offset = 0;        // reset offset variable for sprintf-ing
    offset += sprintf(request + offset, "POST https://www.googleapis.com/geolocation/v1/geolocate?key=%s  HTTP/1.1\r\n", API_KEY);
    offset += sprintf(request + offset, "Host: googleapis.com\r\n");
    offset += sprintf(request + offset, "Content-Type: application/json\r\n");
    offset += sprintf(request + offset, "cache-control: no-cache\r\n");
    offset += sprintf(request + offset, "Content-Length: %d\r\n\r\n", len);
    offset += sprintf(request + offset, "%s\r\n", json_body);
    do_https_request(SERVER, request, response, OUT_BUFFER_SIZE, RESPONSE_TIMEOUT, false);
    Serial.println("-----------");
    Serial.println(response);
    Serial.println("-----------");
    int left_paren = '{';
    int right_paren = '}';
    char *left_loc = strchr(response, left_paren);
    char *right_loc = strrchr(response, right_paren);
    size_t length = right_loc - left_loc + 1;
    memcpy(pruned_response, left_loc, length);
    Serial.println(pruned_response);
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
    Serial.printf("Current Location: \n Lat: %f \n Lon: %f \n", *latitude, *longitude);
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

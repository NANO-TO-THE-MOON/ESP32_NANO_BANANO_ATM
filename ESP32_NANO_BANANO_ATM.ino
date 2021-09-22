#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <ESP32Servo.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

Servo myservo;
int pos = 0;
int servoPin = 19;
int buttonPin = 22;

enum STATES { INACTIVE, WAITING, PAYING_OUT };
STATES state = INACTIVE;

JSONVar myObject;

const char* ssid = "<Your WiFi name>";
const char* password = "<Your WiFi password>";
const char* pricesApi = "https://api.coingecko.com/api/v3/simple/price?ids=banano,nano&include_market_cap=false&include_24hr_vol=false&include_24hr_change=false&include_last_updated_at=false&vs_currencies=eur";
const char* bananoWallet = "http://api-beta.banano.cc:7070/?action=account_balance&account=ban_3atm7ihei6i9ueybxnkk6dw8k7wpgobryap11hu56bt5zryciti7foryij5e";
const char* nanoWallet = "https://proxy.nanos.cc/proxy/?action=account_balance&account=nano_3atm7ihei6i9ueybxnkk6dw8k7wpgobryap11hu56bt5zryciti7foryij5e";
String response;
double priceInNano = -1.0;
int priceInBanano = -1;
int pendingNano = 0;
int previouslyPendingNano = 0;
int pendingBanano = 0;
int previouslyPendingBanano = 0;
int coinsToWithdraw = 0;
int remainingTime = 0;

/* Declare LCD object for SPI
 Adafruit_PCD8544(CLK,DIN,D/C,CE,RST); */
Adafruit_PCD8544 display = Adafruit_PCD8544(18, 23, 4, 15, 2);
int contrastValue = 45;

void setup()
{
  // Allow allocation of all timers
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50);    // standard 50 hz servo
  myservo.attach(servoPin, 500, 2400); // attaches the servo on pin 18 to the servo object
  myservo.write(0);
  Serial.begin(115200);
  delay(1000);
  
  /* Initialize the Display*/
  display.begin();

  /* Change the contrast using the following API*/
  display.setContrast(contrastValue);

  connecting_screen();
  WiFi.begin(ssid, password);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  update_pending_values();
  previouslyPendingNano = pendingNano;
  previouslyPendingBanano = pendingBanano;

  title_screen();
}

void loop()
{
  switch(state){
    case INACTIVE:
      if(digitalRead(buttonPin) == HIGH){
        update_prices();
        remainingTime = 50;
        waiting_screen();
        state = WAITING;
      }
      break;
    case WAITING:
      if(payout_done()){
        paying_out_screen();
        state = PAYING_OUT;
      } else {
        remainingTime-=1;
        if(remainingTime>0){
          waiting_screen();
        } else {
          state = INACTIVE;
          title_screen();
        }
      }
      break;
    case PAYING_OUT:
      drop_the_coins(coinsToWithdraw);
      state = INACTIVE;
      title_screen();
      break;
  }
  delay(50);
}

void connecting_screen(){
  display.clearDisplay();
  display.display();
  delay(1000);
  display.setCursor(0,5);
  display.setTextSize(1);
  display.println("Connecting to the WiFi...");
  display.display();
  delay(1000);
}

void title_screen(){
  display.clearDisplay();
  display.display();
  delay(1000);
  display.setCursor(20,1);
  display.setTextSize(2);
  display.println("NANO");
  display.setTextSize(1);
  display.setCursor(5,18);
  display.println("Euro Exchange");
  display.setCursor(0,32);
  display.println("Press N to start");
  display.display();
  delay(1000);
}

void waiting_screen(){
  display.clearDisplay();
  display.setCursor(0,1);
  display.setTextSize(1);
  display.println("Send NANO/BAN");
  display.setTextSize(1);
  display.setCursor(10,15);
  char buff[20];
  sprintf(buff, "Waiting %d", remainingTime);
  display.println(buff);
  display.setCursor(5,29);
  sprintf(buff, "1E = %0.2fN", priceInNano);
  display.println(buff);
  display.setCursor(5,39);
  sprintf(buff, "1E = %dB", priceInBanano);
  display.println(buff);
  display.display();
  delay(1000);
}

void paying_out_screen(){
  display.clearDisplay();
  display.display();
  delay(1000);
  display.setCursor(0,1);
  display.setTextSize(1);
  display.println("Paying out...");
  display.setTextSize(1);
  display.setCursor(5,20);
  display.println("Your EURO is");
  display.setCursor(5,32);
  display.println("rolling out");
  display.display();
  delay(1000);
}

void drop_the_coins(byte c){  
  for(byte i=0; i<c; i++){
    for(pos = 0; pos <= 100; pos += 1){ myservo.write(pos); delay(15); }
    for(pos = 100; pos >= 0; pos -= 1){ myservo.write(pos); delay(15); }
    delay(100);
  }
}

bool payout_done(){
  if(priceInNano>0.0 && priceInBanano>0){
    update_pending_values();
    int nanoPayment = pendingNano - previouslyPendingNano;
    int euroInNano = int(priceInNano*100.0);
    if(nanoPayment>=euroInNano){
      coinsToWithdraw = nanoPayment/euroInNano;
      return true;
    }
    int bananoPayment = pendingBanano - previouslyPendingBanano;
    if(bananoPayment>=priceInBanano){
      coinsToWithdraw = bananoPayment/priceInBanano;
      return true;
    }
  }
  return false;
}

void update_pending_values(){
  previouslyPendingNano = pendingNano;
  previouslyPendingBanano = pendingBanano;

  JSONVar value;
  String temp;
  
  if(getJSONObjectFromAPI(nanoWallet)){
    value = myObject["pending"];
    temp = value;
    temp.remove(temp.length()-28,28);
    pendingNano = temp.toInt();
  }
  
  if(getJSONObjectFromAPI(bananoWallet)){
    value = myObject["pending"];
    temp = value;
    temp.remove(temp.length()-29,29);
    pendingBanano = temp.toInt();
  }
}

void update_prices(){
  if(getJSONObjectFromAPI(pricesApi)){
    JSONVar value = myObject["nano"]["eur"];
    priceInNano = ceil((1.0/double(value))*100)/100.0;
    Serial.println(priceInNano);
    value = myObject["banano"]["eur"];
    priceInBanano = ceil(1.0/double(value));
    Serial.println(priceInBanano);
  }
}

bool getJSONObjectFromAPI(const char* link){
  response = httpGETRequest(link);
  Serial.println(response);
  myObject = JSON.parse(response);
  if (JSON.typeof(myObject) == "undefined") {
    Serial.println("Parsing input failed!");
    return false;
  }
  Serial.print("JSON object = ");
  Serial.println(myObject);
  return true;
}

String httpGETRequest(const char* serverName) {
  HTTPClient http;
  http.begin(serverName);
  int httpResponseCode = http.GET();
  String payload = "{}";
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
  return payload;
}

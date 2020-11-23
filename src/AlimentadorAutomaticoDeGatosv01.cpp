/*
 Name:        chatGroupEchoBot.ino
 Created:     14/06/2020
 Author:      Stefano Ledda <shurillu@tiscalinet.it>
 Description: an example that check for incoming messages
              1) send a message to the sender some "message related" infos
              2) if the message came from a group chat, reply the group chat  
                 with the same message (like the echoBot example)
*/
#include <Arduino.h>
#include "CTBot.h"
#include "Utilities.h" // for int64ToAscii() helper function
#include "RTClib.h"

#define FEED_CALLBACK "alimentar" // callback data sent when "Alimentar" button is pressed

CTBotInlineKeyboard myKbd;                                       // custom inline keyboard object helper
String ssid = "alface";                                          // REPLACE mySSID WITH YOUR WIFI SSID
String pass = "q1w2e3r4t5";                                      // REPLACE myPassword YOUR WIFI PASSWORD, IF ANY
String token = "1489186927:AAF7Ddk7ACbsUIGpbNQQBS_p47Dfas58HDg"; // REPLACE myToken WITH YOUR TELEGRAM BOT TOKEN
long long groupId = -1001471540689;                              //Group Id do grupo a enviar mensagem
bool manualFeed = false;                                         // variavel que armazena se está na hora de alimentar ou não
const unsigned long feedingInterval = 10800000UL;                //3h in milliseconds
const unsigned long testFeedingInterval = 30000UL;               //5s in milliseconds
unsigned long lastFeedingTime = 0;

RTC_PCF8563 rtc;
const long unixFeedingInterval = 10800L; //3h in seconds
long unixLastFeedingTime = 0L;

const int m4 = 16; //d0 pin
const int m2 = 14; //d5 pin

const int m1 = 0;  //d3 pin
const int m3 = 12; //d6 pin

const int ON = 0;
const int OFF = 1;
bool rtcOnline = false;

void setup();
void loop();
ICACHE_RAM_ATTR void buttonPressed1();
void dealWithMessage(TBMessage msg);
void feedCats();
void sendTelegramMessage(String text);
bool isTimeToFeed();
void stopMotor();
void runCW();
void runCCW();
void registerFeedTime();

CTBot myBot;

void setup()
{

  // initialize the Serial
  Serial.begin(115200);
  Serial.println("Starting TelegramBot...");

  // connect the ESP8266 to the desired access point
  myBot.wifiConnect(ssid, pass);

  // set the telegram bot token
  myBot.setTelegramToken(token);

  // check if all things are ok
  if (myBot.testConnection())
    Serial.println("\ntestConnection OK");
  else
  {
    Serial.println("\ntestConnection NOK");
    abort();
  }

  if (rtc.begin())
  {
    Serial.println("RTC is online");
    rtcOnline = true;
    sendTelegramMessage("system turned on. rtc is online.");
  }
  //interrupt para o botão
  attachInterrupt(digitalPinToInterrupt(13), buttonPressed1, RISING);

  //pin definitions for the motor leads
  pinMode(m1, OUTPUT);
  pinMode(m2, OUTPUT);
  pinMode(m3, OUTPUT);
  pinMode(m4, OUTPUT);
  stopMotor();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, OFF);

  rtc.start();

  // inline keyboard customization
  // add a query button to the first row of the inline keyboard
  myKbd.addButton("ALIMENTAR", FEED_CALLBACK, CTBotKeyboardButtonQuery);
  // add another query button to the first row of the inline keyboard
  // myKbd.addButton("LIGHT OFF", LIGHT_OFF_CALLBACK, CTBotKeyboardButtonQuery);
  // add a new empty button row
  myKbd.addRow();
  // add a URL button to the second row of the inline keyboard
  myKbd.addButton("see docs", "https://github.com/shurillu/CTBot", CTBotKeyboardButtonURL);
}

void runCW()
{
  //m0 e m2 ligados
  digitalWrite(m2, ON);
  digitalWrite(m4, ON);
}

void runCCW()
{
  //m1 e d3 ligados
  digitalWrite(m1, ON);
  digitalWrite(m3, ON);
}

void stopMotor()
{
  digitalWrite(m1, OFF);
  digitalWrite(m2, OFF);
  digitalWrite(m3, OFF);
  digitalWrite(m4, OFF);
  delay(100);
}

ICACHE_RAM_ATTR void buttonPressed1()
{
  digitalWrite(LED_BUILTIN, ON);
  // manualFeed = true;
}

void loop()
{
  // a variable to store telegram message data
  TBMessage msg;

  // check if there is a new incoming message
  if (myBot.getNewMessage(msg))
  {
    dealWithMessage(msg);
  }
  if (isTimeToFeed())
  {
    feedCats();
  }

  // if is not time to do nothing, wait some time...
  delay(2000);
}

void dealWithMessage(TBMessage msg)
{
  Serial.println(msg.text);
  // check if the message is a text message
  if (msg.messageType == CTBotMessageText)
  {
    if (msg.text == "alimentar")
    {
      feedCats();
      return;
    }
    // the user write anything else --> show The Keyboard
    myBot.sendMessage(msg.sender.id, "Inline Keyboard", myKbd);
  }
  if (msg.messageType == CTBotMessageQuery)
  {
    if (msg.callbackQueryData.equals(FEED_CALLBACK))
    {
      feedCats();
      myBot.endQuery(msg.callbackQueryID, "Gatos papando!", true);
    }
  }
}

void motorFeedingRoutine()
{
  //pull open for 1.5s
  runCCW();
  delay(1500);
  stopMotor();

  //wait open
  // delay(500); //maybe not

  //close for 250ms
  runCW();
  delay(250);
  stopMotor();

  //little pull to unstuck stuff
  runCCW();
  delay(250);
  stopMotor();

  //finish closing
  runCW();
  delay(250);
  stopMotor();
}

void feedCats()
{

  Serial.println("feeding Cats");
  sendTelegramMessage("Gatos sendo alimentados nesse momento");

  //10g for feeding. 2 feedings for 2 cats.
  motorFeedingRoutine();
  // motorFeedingRoutine(); //for now just one

  digitalWrite(LED_BUILTIN, OFF);

  registerFeedTime();
  Serial.printf("setting Millis last to : %lu \n", lastFeedingTime);
}
void sendTelegramMessage(String text)
{
  myBot.sendMessage(groupId, text);
}

void registerFeedTime()
{
  if (rtcOnline)
  {
    DateTime now = rtc.now();
    unixLastFeedingTime = now.unixtime();
    return;
  }
  lastFeedingTime = millis();
}

bool isTimeToFeed()
{
  if (manualFeed)
  {
    manualFeed = false;
    sendTelegramMessage("Manual Time to feed");
    return true;
  }

  if (rtcOnline)
  {
    return false;
    DateTime now = rtc.now();
    if (now.unixtime() - unixLastFeedingTime >= unixFeedingInterval)
    {
      digitalWrite(LED_BUILTIN, ON);
      sendTelegramMessage("RTC Time to feed but it is disabled");
      return true;
    }
  }

  if (!rtcOnline)
  {
    return false;
    unsigned long now = millis();
    if (now - lastFeedingTime >= feedingInterval)
    {
      digitalWrite(LED_BUILTIN, ON);
      sendTelegramMessage("millis Time to feed but it is disabled");
      return true;
    }
  }

  return false;
}

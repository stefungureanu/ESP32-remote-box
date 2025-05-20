#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ESP32Servo.h>

// WIFI
const char* ssid = "nu";
const char* password = "nu";

// display specs
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// server + port
WiFiServer server(80);

// GPIOs
const int buzzerPin = 14;
const int servoPin = 23;

// state
String boxState = "CLOSED";

Servo servo;

// function to make sounds
void buzz(int duration) {
  digitalWrite(buzzerPin, HIGH);
  delay(duration);
  digitalWrite(buzzerPin, LOW);
}

// function to move the servo depending on state
void moveServo() {
  if (boxState == "CLOSED") {
    servo.write(0);
  } else {
    servo.write(90);
  }
}

// text displayed
void updateDisplay() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.setTextSize(1);
  display.println("IP:");
  display.println(WiFi.localIP());
  display.println("");
  display.setTextSize(2);
  display.println(boxState);
  display.display();
}

// state change function
void toggleLock() {
  if (boxState == "CLOSED") {
    boxState = "OPENED";
    buzz(100);
    delay(100);
    buzz(100);
  } else {
    boxState = "CLOSED";
    buzz(200);
  }
  moveServo();
  updateDisplay();
}

void setup() {
  Serial.begin(115200);

  pinMode(buzzerPin, OUTPUT);
  digitalWrite(buzzerPin, LOW);

  // init display with default vcs (using a 3.7V) and addr
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Display failed"));
    while (true);
  }

  // servo init
  servo.setPeriodHertz(50);
  servo.attach(servoPin, 500, 2400);
  // move back
  servo.write(0);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Connecting to WiFi...");
  display.display();

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.println("IP: ");
  Serial.println(WiFi.localIP());

  server.begin();
  updateDisplay();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    String request = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        request += c;
        if (c == '\n') {
          // trigge toggle function
          if (request.indexOf("GET /toggle") >= 0) {
            toggleLock();
          }

          // HTML
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Connection: close");
          client.println();
          client.println(R"rawliteral(
          <!DOCTYPE html>
          <html lang="en">
          <head>
            <meta charset="UTF-8">
            <title>ESP32 Lock</title>
            <meta name="viewport" content="width=device-width, initial-scale=1">
            <style>
              body {
                font-family: Arial, sans-serif;
                text-align: center;
                padding: 50px;
                background-color: #f2f2f2;
              }
              h1 {
                color: #333;
              }
              .state {
                font-size: 1.5em;
                margin: 20px 0;
                color: #555;
              }
              .button {
                background-color: #4CAF50;
                border: none;
                color: white;
                padding: 15px 40px;
                text-align: center;
                text-decoration: none;
                display: inline-block;
                font-size: 24px;
                margin: 10px;
                cursor: pointer;
                border-radius: 10px;
                box-shadow: 0 4px 6px rgba(0,0,0,0.2);
              }
            </style>
          </head>
          <body>
            <h1>ESP32 Remote Box</h1>
            <div class="state">Current state: <b>)rawliteral" + boxState + R"rawliteral(</b></div>
            <a href="/toggle"><button class="button">TOGGLE</button></a>
          </body>
          </html>
          )rawliteral");

          client.println();

          break;
        }
      }
    }
    delay(500);
    client.stop();
  }
}

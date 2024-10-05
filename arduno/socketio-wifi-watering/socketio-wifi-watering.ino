#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>
WiFiMulti WiFiMulti;
SocketIOclient socketIO;
#define USE_SERIAL Serial
#include <time.h>

// Sensor pins
#define sensorPower 5  // chân cấp điện
#define sensorPin 34   // chân Analog lấy dữ liệu
#define replayPin 27   // chân relay

const char *sensorId = "ss1";                                      // sensorId
const char *location = "Hồ Chí Minh City";                         //location
const char *API = "http://192.168.1.6:8090/api/v1/sensor/create";  //api


// Hàm đọc giá trị từ cảm biến
int readSensor() {
  digitalWrite(sensorPower, HIGH);  // Mở cấp diện cho sensor
  delay(100);                       // Chờ khởi tạo cho điện áp ổn
  int val = analogRead(sensorPin);  // Đọc dữ liệu từ chân Ananalog
  digitalWrite(sensorPower, LOW);   // Tắt cảm biến
  return val;                       // Trả về giá trị độ ẩm
}


int status_activate;
int status_autowater;
int status_watering;


void socketIOEvent(socketIOmessageType_t type, uint8_t *payload, size_t length) {
  //message type             //data          // length data
  switch (type) {
    case sIOtype_DISCONNECT:
      USE_SERIAL.printf("[IOc] Disconnected!\n");
      break;
    case sIOtype_CONNECT:
      USE_SERIAL.printf("[IOc] Connected to url: %s\n", payload);
      // join default namespace (no auto join in Socket.IO V3)
      socketIO.send(sIOtype_CONNECT, "/");
      break;
    case sIOtype_EVENT:
      {
        char *sptr = NULL;
        int id = strtol((char *)payload, &sptr, 10);
        USE_SERIAL.printf("[IOc] get event: %s id: %d\n", payload, id);
        if (id) {
          payload = (uint8_t *)sptr;
        }
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, payload, length);
        if (error) {
          USE_SERIAL.print(F("deserializeJson() failed: "));
          USE_SERIAL.println(error.c_str());
          return;
        }

        String eventName = doc[0];
        USE_SERIAL.printf("[IOc] event name: %s\n", eventName.c_str());

        // Process different types of events
        if (eventName == "DEACTIVESENSOR") {
          // Handle OFFSENSOR event
          // Extract data from the event payload and process accordingly
          String sensorId = doc[1];  // Assuming the sensor ID is the second element in the JSON array
          USE_SERIAL.printf("[IOc] OFFSENSOR event received for sensor ID: %s\n", sensorId.c_str());
          if (strcmp(sensorId.c_str(), "OFFss1") == 0) {
            status_activate = 1;
          }
        } else if (eventName == "ACTIVESENSOR") {
          // Handle ONSENSOR event
          // Extract data from the event payload and process accordingly
          String sensorId = doc[1];  // Assuming the sensor ID is the second element in the JSON array
          USE_SERIAL.printf("[IOc] ONSENSOR event received for sensor ID: %s\n", sensorId.c_str());
          if (strcmp(sensorId.c_str(), "ONss1") == 0) {
            status_activate = 0;
          }
        } else if (eventName == "OFFAUTO") {
          // Handle ONSENSOR event
          // Extract data from the event payload and process accordingly
          String sensorId = doc[1];  // Assuming the sensor ID is the second element in the JSON array
          USE_SERIAL.printf("[IOc] ONSENSOR event received for sensor ID: %s\n", sensorId.c_str());
          if (strcmp(sensorId.c_str(), "OFFAUTOss1") == 0) {
            status_autowater = 1;
          }
        } else if (eventName == "ONAUTO") {
          // Handle ONSENSOR event
          // Extract data from the event payload and process accordingly
          String sensorId = doc[1];  // Assuming the sensor ID is the second element in the JSON array
          USE_SERIAL.printf("[IOc] ONSENSOR event received for sensor ID: %s\n", sensorId.c_str());
          if (strcmp(sensorId.c_str(), "ONAUTOss1") == 0) {
            status_autowater = 0;
          }
        } else if (eventName == "WATERING") {
          // Handle ONSENSOR event
          // Extract data from the event payload and process accordingly
          String sensorId = doc[1];  // Assuming the sensor ID is the second element in the JSON array
          USE_SERIAL.printf("[IOc] ONSENSOR event received for sensor ID: %s\n", sensorId.c_str());
          if (strcmp(sensorId.c_str(), "WATERINGss1") == 0) {
            status_watering = 0;
          }
        } else if (eventName == "STOPWATERING") {
          // Handle ONSENSOR event
          // Extract data from the event payload and process accordingly
          String sensorId = doc[1];  // Assuming the sensor ID is the second element in the JSON array
          USE_SERIAL.printf("[IOc] ONSENSOR event received for sensor ID: %s\n", sensorId.c_str());
          if (strcmp(sensorId.c_str(), "STOPWATERINGss1") == 0) {
            status_watering = 1;
          }
        }




        // Message Includes a ID for a ACK (callback)
        if (id) {
          // creat JSON message for Socket.IO (ack)
          DynamicJsonDocument docOut(1024);
          JsonArray array = docOut.to<JsonArray>();

          // add payload (parameters) for the ack (callback function)
          JsonObject param1 = array.createNestedObject();
          param1["now"] = millis();

          // JSON to String (serializion)
          String output;
          output += id;
          serializeJson(docOut, output);

          // Send event
          socketIO.send(sIOtype_ACK, output);
        }
      }
      break;
    case sIOtype_ACK:
      USE_SERIAL.printf("[IOc] get ack: %u\n", length);
      break;
    case sIOtype_ERROR:
      USE_SERIAL.printf("[IOc] get error: %u\n", length);
      break;
    case sIOtype_BINARY_EVENT:
      USE_SERIAL.printf("[IOc] get binary: %u\n", length);
      break;
    case sIOtype_BINARY_ACK:
      USE_SERIAL.printf("[IOc] get binary ack: %u\n", length);
      break;
  }
}

void setup() {
  //USE_SERIAL.begin(921600);
  USE_SERIAL.begin(115200);

  pinMode(sensorPower, OUTPUT);    // chân cấp điên sensor
  digitalWrite(sensorPower, LOW);  // mặc định tắt cấp điện cho sensor

  pinMode(replayPin, OUTPUT);    //cấu hình cho chân relay
  digitalWrite(replayPin, LOW);  // mặc định tắt không bơm relay

  status_activate = 0;
  status_autowater = 0;
  status_watering = 1;

  //Serial.setDebugOutput(true);
  USE_SERIAL.setDebugOutput(true);

  USE_SERIAL.println();
  USE_SERIAL.println();
  USE_SERIAL.println();

  for (uint8_t t = 4; t > 0; t--) {
    USE_SERIAL.printf("[SETUP] BOOT WAIT %d...\n", t);
    USE_SERIAL.flush();
    delay(1000);
  }

  // const char *ssid = "62BauCat5";
  // const char *pass = "HFR62baucat5";
  // const char *HOST = "192.168.1.19:8077";
  WiFiMulti.addAP("62BauCat5", "HFR62baucat5");

  //WiFi.disconnect();
  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
  }

  String ip = WiFi.localIP().toString();
  USE_SERIAL.printf("[SETUP] WiFi Connected %s\n", ip.c_str());

  // Cấu hình NTP với múi giờ GMT+7
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov", "asia.pool.ntp.org");
  USE_SERIAL.println("\nWaiting for time");
  while (!time(nullptr)) {
    USE_SERIAL.print(".");
    delay(1000);
  }
  USE_SERIAL.println("\nTime synchronized");

  // server address, port and URL
  socketIO.begin("192.168.1.6", 8090, "/socket.io/?EIO=4");

  // event handler
  socketIO.onEvent(socketIOEvent);
}


unsigned long lastSocketSendTime = 0;
unsigned long lastAPISendTime = 0;
unsigned long lastWateringTime = 0;
const unsigned long WateringInterval = 10000;
const unsigned long socketSendInterval = 5000;
const unsigned long APISendInterval = 20000;



void loop() {
  socketIO.loop();
  uint64_t now = millis();


  // Kiểm tra trạng thái activate trước khi gửi socket hoặc API
  if (status_activate == 0) {
    if (now - lastSocketSendTime > socketSendInterval) {
      lastSocketSendTime = now;
      int moisture = readSensor();
      // Xử lý dữ liệu và gửi socket
      // Lấy thời gian hiện tại
      time_t now = time(nullptr);
      struct tm *timeinfo = localtime(&now);

      // Tạo một JsonObject để lưu dữ liệu sensor
      DynamicJsonDocument doc(1024);
      JsonObject datasensor = doc.createNestedObject("datasensor");
      datasensor["sensorId"] = sensorId;
      datasensor["moisture"] = moisture;
      datasensor["time"] = String(timeinfo->tm_hour) + ":" + String(timeinfo->tm_min) + ":" + String(timeinfo->tm_sec);
      datasensor["day"] = String(timeinfo->tm_mday) + "/" + String(timeinfo->tm_mon + 1) + "/" + String(timeinfo->tm_year + 1900);
      // Tạo tin nhắn JSON cho Socket.IO
      DynamicJsonDocument eventDoc(1024);
      JsonArray array = eventDoc.to<JsonArray>();
      array.add("SENSORDATA");
      array.add(datasensor);
      String output;
      serializeJson(eventDoc, output);
      socketIO.sendEVENT(output);
      USE_SERIAL.println(output);
    }

    if (now - lastAPISendTime > APISendInterval) {
      lastAPISendTime = now;
      int moisture = readSensor();
      //Tạo json để gửi API về db
      String httpRequestData = "{\"sensorId\":\"" + String(sensorId) + "\", \"moisture\":\"" + String(moisture) + "\", \"location\":\"" + String(location) + "\"}";
      // Gửi dữ liệu lên API
      sendDataToAPI(httpRequestData);
    }

  }  // else {
     // In ra thông báo nếu status_activate = 1
     // USE_SERIAL.println("Data sending is deactivated.");
     // }


  // tưới auto
  if (status_autowater == 0 && status_watering == 1) {
    if (now - lastWateringTime > WateringInterval) {
      lastWateringTime = now;
      int moi = readSensor();
      Serial.printf("Moisture level: %d\n", moi);
      if (moi > 3500) {
        watering();      // tưới vì khô
        stopwatering();  // tưới xong tắt ngay
      } else {
        stopwatering();
      }  // Tắt tưới
    }
  }  // else {
     //   // In ra thông báo nếu status_autowater = 1
     //   USE_SERIAL.println("Automatic watering is deactivated.");
     // }

  //thủ công
  if (status_watering == 0) {
    USE_SERIAL.println("Watering...");
    digitalWrite(replayPin, HIGH);

  } else {
    digitalWrite(replayPin, LOW);
  }
}


// Function to activate watering
void watering() {
  digitalWrite(replayPin, HIGH);  // Turn off the relay to stop watering
  delay(5000);
  digitalWrite(replayPin, LOW);  // Turn on the relay to start watering
}

// Function to stop watering
void stopwatering() {
  digitalWrite(replayPin, LOW);  // Turn off the relay to stop watering
}

// Function to send data to an API
void sendDataToAPI(String jsonData) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(API);                                     // Specify the URL
    http.addHeader("Content-Type", "application/json");  // Specify content-type header
    int httpResponseCode = http.POST(jsonData);          // Send the POST request
    if (httpResponseCode > 0) {                          // Check the returning code
      String response = http.getString();                // Get the request response payload
      USE_SERIAL.println("HTTP Response code: " + String(httpResponseCode));
      USE_SERIAL.println("Response: " + response);
    } else {
      USE_SERIAL.print("Error on sending POST: ");
      USE_SERIAL.println(httpResponseCode);
    }
    http.end();  // Free resources
  } else {
    USE_SERIAL.println("WiFi Disconnected");
  }
}
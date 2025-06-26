#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>

const char* ssid = "AgriGuard_ESP32";
const char* password = "password123";

WebServer server(80);

#define OLED_SDA_PIN GPIO_NUM_21
#define OLED_SCL_PIN GPIO_NUM_22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define DHT_PIN GPIO_NUM_4
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

const int SOIL_MOISTURE_PIN = GPIO_NUM_18;
const int SOIL_MOISTURE_AIR_VALUE = 3000;
const int SOIL_MOISTURE_WATER_VALUE = 1000;

#define RAIN_SENSOR_PIN GPIO_NUM_19
const int RAIN_WET_THRESHOLD = LOW;

#define PUMP_RELAY_PIN GPIO_NUM_23
bool pump_state = false;

float temperature_c = 0.0;
float humidity_percent = 0.0;
int soil_moisture_raw = 0;
int soil_moisture_percent = 0;
bool is_raining = false;

String risk_level = "OPTIMAL";
String risk_message = "";

int watering_threshold_on = 35;
int watering_threshold_off = 70;

void setupOLED() {
  Wire.begin(OLED_SDA_PIN, OLED_SCL_PIN);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;) {}
  }
  display.display();
  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Agri-Guard Ready!");
  display.display();
  delay(1000);
}

void readDHT11() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (isnan(h) || isnan(t)) {
    Serial.println(F("DHT11 Error: Failed to read from DHT sensor!"));
    temperature_c = -1;
    humidity_percent = -1;
  } else {
    temperature_c = t;
    humidity_percent = h;
    Serial.print("DHT11 Reading: Temperature: ");
    Serial.print(temperature_c);
    Serial.print(" °C, Humidity: ");
    Serial.print(humidity_percent);
    Serial.println(" %");
  }
}

void readSoilMoisture() {
  soil_moisture_raw = analogRead(SOIL_MOISTURE_PIN);
  soil_moisture_percent = map(soil_moisture_raw, SOIL_MOISTURE_AIR_VALUE, SOIL_MOISTURE_WATER_VALUE, 0, 100);

  if (soil_moisture_percent < 0) soil_moisture_percent = 0;
  if (soil_moisture_percent > 100) soil_moisture_percent = 100;

  Serial.print("Soil Moisture (raw): ");
  Serial.print(soil_moisture_raw);
  Serial.print(", Soil Moisture: ");
  Serial.print(soil_moisture_percent);
  Serial.println(" %");
}

void readRainSensor() {
  int rain_status = digitalRead(RAIN_SENSOR_PIN);
  is_raining = (rain_status == RAIN_WET_THRESHOLD);

  Serial.print("Rain Sensor Status: ");
  Serial.print(rain_status == HIGH ? "DRY" : "WET");
  Serial.print(", Is Raining: ");
  Serial.println(is_raining ? "Yes" : "No");
}

void controlPump(bool state) {
  pump_state = state;
  digitalWrite(PUMP_RELAY_PIN, pump_state ? LOW : HIGH);
  Serial.print("Pump is now: ");
  Serial.println(pump_state ? "ON" : "OFF");
}

void automatedWatering() {
  if (!is_raining) {
    if (soil_moisture_percent < watering_threshold_on && !pump_state) {
      Serial.println("Automated watering: Soil too dry, turning pump ON.");
      controlPump(true);
    } else if (soil_moisture_percent >= watering_threshold_off && pump_state) {
      Serial.println("Automated watering: Soil sufficiently wet, turning pump OFF.");
      controlPump(false);
    }
  } else {
    if (pump_state) {
      Serial.println("Automated watering: Raining, turning pump OFF.");
      controlPump(false);
    }
  }
}

void assessRisk() {
  risk_level = "OPTIMAL";
  risk_message = "";

  const float HIGH_TEMP_THRESHOLD = 30.0;
  const float LOW_TEMP_THRESHOLD = 10.0;
  const float HIGH_HUMIDITY_THRESHOLD = 85.0;
  const float LOW_HUMIDITY_THRESHOLD = 40.0;
  const int DRY_SOIL_THRESHOLD = 30;
  const int WET_SOIL_THRESHOLD = 80;

  if (is_raining && humidity_percent > HIGH_HUMIDITY_THRESHOLD && temperature_c != -1) {
    risk_level = "HIGH RISK";
    risk_message = "FUNGAL BLIGHT RISK!";
  } else if (humidity_percent > HIGH_HUMIDITY_THRESHOLD && temperature_c > LOW_TEMP_THRESHOLD && temperature_c != -1) {
    risk_level = "MODERATE RISK";
    risk_message = "HUMIDITY CONCERN (FUNGUS)";
  }

  if (temperature_c > HIGH_TEMP_THRESHOLD && humidity_percent < LOW_HUMIDITY_THRESHOLD && !is_raining && temperature_c != -1) {
    risk_level = "HIGH RISK";
    risk_message = "PEST OUTBREAK RISK (DRY)!";
  }

  if (soil_moisture_percent < DRY_SOIL_THRESHOLD) {
    if (risk_level == "OPTIMAL") {
      risk_level = "NEEDS WATER";
      risk_message = "SOIL TOO DRY!";
    } else {
      risk_message += " & SOIL DRY!";
    }
  }

  if (soil_moisture_percent > WET_SOIL_THRESHOLD && is_raining) {
    if (risk_level == "OPTIMAL") {
      risk_level = "CRITICAL RISK";
      risk_message = "WATERLOGGING! ROOT ROT!";
    } else {
      risk_message += " & WATERLOGGED!";
    }
  }

  if (temperature_c > HIGH_TEMP_THRESHOLD && !is_raining && temperature_c != -1) {
    if (risk_level == "OPTIMAL") {
      risk_level = "MODERATE RISK";
      risk_message = "HIGH TEMP STRESS!";
    } else {
      risk_message += " & HEAT STRESS!";
    }
  }
  if (temperature_c < LOW_TEMP_THRESHOLD && temperature_c != -1) {
    if (risk_level == "OPTIMAL") {
      risk_level = "MODERATE RISK";
      risk_message = "LOW TEMP STRESS!";
    } else {
      risk_message += " & COLD STRESS!";
    }
  }

  if (risk_message.isEmpty() && risk_level == "OPTIMAL") {
    risk_message = "All conditions are good.";
  }

  Serial.print("Risk Level: ");
  Serial.print(risk_level);
  Serial.print(", Message: ");
  Serial.println(risk_message);
}

void updateOLED() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("Temp: ");
  if (temperature_c == -1) display.println("N/A C");
  else {
    display.print(temperature_c, 1);
    display.println(" C");
  }

  display.setCursor(0, 10);
  display.print("Hum : ");
  if (humidity_percent == -1) display.println("N/A %");
  else {
    display.print(humidity_percent, 1);
    display.println(" %");
  }

  display.setCursor(0, 20);
  display.print("Soil: ");
  display.print(soil_moisture_percent);
  display.println(" %");

  display.setCursor(0, 30);
  display.print("Rain: ");
  display.println(is_raining ? "YES" : "NO");

  display.setTextSize(2);
  display.setCursor(0, 45);

  if (risk_level == "OPTIMAL") {
    display.setTextColor(SSD1306_WHITE);
    display.println("OPTIMAL");
  } else if (risk_level == "NEEDS WATER") {
    display.setTextColor(SSD1306_WHITE);
    display.println("DRY ALERT!");
  } else if (risk_level == "MODERATE RISK") {
    display.setTextColor(SSD1306_WHITE);
    display.println("MOD. RISK!");
  } else if (risk_level == "HIGH RISK") {
    display.setTextColor(SSD1306_WHITE);
    display.println("HIGH RISK!");
  } else if (risk_level == "CRITICAL RISK") {
    display.setTextColor(SSD1306_WHITE);
    display.println("CRITICAL!");
  }

  display.display();
}

void handleGetData() {
  DynamicJsonDocument doc(256);
  doc["temp"] = (temperature_c == -1) ? "N/A" : String(temperature_c, 1);
  doc["hum"] = (humidity_percent == -1) ? "N/A" : String(humidity_percent, 1);
  doc["soil"] = soil_moisture_percent;
  doc["rain"] = is_raining ? "YES" : "NO";
  doc["riskLevel"] = risk_level;
  doc["riskMessage"] = risk_message;
  doc["pumpState"] = pump_state ? "ON" : "OFF";
  doc["waterOnThreshold"] = watering_threshold_on;
  doc["waterOffThreshold"] = watering_threshold_off;

  String jsonResponse;
  serializeJson(doc, jsonResponse);
  server.send(200, "application/json", jsonResponse);
}

void handleRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Agri-Guard Monitor</title>
        <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;600;700&display=swap" rel="stylesheet">
        <style>
            body { font-family: 'Inter', sans-serif; text-align: center; margin: 0; background-color: #f0f4f8; color: #333; }
            .container { max-width: 500px; margin: 20px auto; padding: 25px; background-color: #ffffff; border-radius: 12px; box-shadow: 0 4px 15px rgba(0, 0, 0, 0.1); }
            h1 { color: #2c3e50; margin-bottom: 25px; }
            .section { margin-bottom: 20px; padding: 15px; background-color: #ecf0f1; border-radius: 8px; border: 1px solid #dfe6e9; }
            .section h2 { color: #34495e; margin-top: 0; font-size: 1.3em; }
            p { font-size: 1.1em; margin: 8px 0; }
            strong { color: #2980b9; }
            .data-item { display: flex; justify-content: space-between; padding: 5px 0; border-bottom: 1px dashed #bdc3c7; }
            .data-item:last-child { border-bottom: none; }
            .status-display { font-weight: bold; font-size: 1.3em; margin-top: 15px; padding: 10px; border-radius: 8px; }
            .optimal { background-color: #d4edda; color: #155724; border-color: #c3e6cb; }
            .warning { background-color: #fff3cd; color: #856404; border-color: #ffeeba; }
            .alert { background-color: #f8d7da; color: #721c24; border-color: #f5c6cb; }
            .button { background-color: #28a745; color: white; padding: 12px 25px; text-align: center; text-decoration: none; display: inline-block; font-size: 16px; margin: 10px 5px; cursor: pointer; border: none; border-radius: 8px; transition: background-color 0.3s ease; }
            .button:hover { background-color: #218838; }
            .button.off { background-color: #dc3545; }
            .button.off:hover { background-color: #c82333; }
            .form-group { margin-bottom: 15px; text-align: left; }
            .form-group label { display: block; margin-bottom: 8px; font-weight: 600; color: #34495e; }
            .form-group input[type='number'] { width: calc(100% - 20px); padding: 10px; border-radius: 6px; border: 1px solid #ccc; box-sizing: border-box; }
            .form-group button { background-color: #007bff; color: white; padding: 12px 25px; border: none; border-radius: 8px; cursor: pointer; width: 100%; font-size: 16px; transition: background-color 0.3s ease; }
            .form-group button:hover { background-color: #0056b3; }
            #pumpStatus span { font-weight: bold; }
        </style>
    </head>
    <body>
        <div class="container">
            <h1>Agri-Guard Smart Farm Monitor</h1>

            <div class="section">
                <h2>Current Conditions</h2>
                <div class="data-item"><span>Temperature:</span> <span id="tempValue">N/A &deg;C</span></div>
                <div class="data-item"><span>Humidity:</span> <span id="humValue">N/A %</span></div>
                <div class="data-item"><span>Soil Moisture:</span> <span id="soilValue">N/A %</span></div>
                <div class="data-item"><span>Rain Detected:</span> <span id="rainValue">NO</span></div>
            </div>

            <div class="section status-display" id="riskStatus">
                Risk: OPTIMAL<br>All conditions are good.
            </div>

            <div class="section">
                <h2>Automated Watering</h2>
                <p id="pumpStatus">Pump Status: <span>OFF</span></p>
                <button class="button" id="pumpOnBtn">Turn Pump ON</button>
                <button class="button off" id="pumpOffBtn">Turn Pump OFF</button>
                <p style='font-size:0.8em; margin-top:10px; color:#555;'>*Automated watering can override manual control.</p>
            </div>

            <div class="section">
                <h2>Watering Thresholds</h2>
                <div class="form-group">
                    <label for="waterOnInput">Water ON (%)</label>
                    <input type="number" id="waterOnInput" min="0" max="100">
                </div>
                <div class="form-group">
                    <label for="waterOffInput">Water OFF (%)</label>
                    <input type="number" id="waterOffInput" min="0" max="100">
                </div>
                <button class="button" id="updateThresholdsBtn">Update Thresholds</button>
            </div>
        </div>

        <script>
            async function updateData() {
                try {
                    const response = await fetch('/data');
                    const data = await response.json();

                    document.getElementById('tempValue').textContent = data.temp + ' °C';
                    document.getElementById('humValue').textContent = data.hum + ' %';
                    document.getElementById('soilValue').textContent = data.soil + ' %';
                    document.getElementById('rainValue').textContent = data.rain;

                    const riskStatusDiv = document.getElementById('riskStatus');
                    riskStatusDiv.className = 'section status-display ';
                    if (data.riskLevel === 'OPTIMAL') riskStatusDiv.classList.add('optimal');
                    else if (data.riskLevel === 'NEEDS WATER' || data.riskLevel === 'MODERATE RISK') riskStatusDiv.classList.add('warning');
                    else riskStatusDiv.classList.add('alert');
                    riskStatusDiv.innerHTML = Risk: ${data.riskLevel}<br>${data.riskMessage};

                    const pumpStatusSpan = document.querySelector('#pumpStatus span');
                    pumpStatusSpan.textContent = data.pumpState;
                    pumpStatusSpan.style.color = data.pumpState === 'ON' ? 'green' : 'red';

                    document.getElementById('waterOnInput').value = data.waterOnThreshold;
                    document.getElementById('waterOffInput').value = data.waterOffThreshold;

                } catch (error) {
                    console.error('Failed to fetch data:', error);
                }
            }

            async function sendPumpCommand(command) {
                try {
                    const response = await fetch('/' + command);
                    const result = await response.json();
                    console.log(result.message);
                    updateData();
                } catch (error) {
                    console.error('Failed to send pump command:', error);
                }
            }

            async function updateThresholds() {
                const waterOn = document.getElementById('waterOnInput').value;
                const waterOff = document.getElementById('waterOffInput').value;
                if (waterOn === "" || waterOff === "" || isNaN(waterOn) || isNaN(waterOff)) {
                    alert("Please enter valid numbers for thresholds.");
                    return;
                }
                const response = await fetch(/updateThresholds?waterOn=${waterOn}&waterOff=${waterOff});
                const result = await response.json();
                console.log(result.message);
                updateData();
            }

            document.getElementById('pumpOnBtn').addEventListener('click', () => sendPumpCommand('pumpOn'));
            document.getElementById('pumpOffBtn').addEventListener('click', () => sendPumpCommand('pumpOff'));
            document.getElementById('updateThresholdsBtn').addEventListener('click', updateThresholds);

            document.addEventListener('DOMContentLoaded', () => {
                updateData();
                setInterval(updateData, 5000);
            });

        </script>
    </body>
    </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handlePumpOn() {
  controlPump(true);
  DynamicJsonDocument doc(64);
  doc["status"] = "success";
  doc["message"] = "Pump turned ON.";
  String jsonResponse;
  serializeJson(doc, jsonResponse);
  server.send(200, "application/json", jsonResponse);
}

void handlePumpOff() {
  controlPump(false);
  DynamicJsonDocument doc(64);
  doc["status"] = "success";
  doc["message"] = "Pump turned OFF.";
  String jsonResponse;
  serializeJson(doc, jsonResponse);
  server.send(200, "application/json", jsonResponse);
}

void handleUpdateThresholds() {
  DynamicJsonDocument doc(64);
  doc["status"] = "fail";
  doc["message"] = "Invalid watering threshold values received.";

  if (server.hasArg("waterOn") && server.hasArg("waterOff")) {
    int newWaterOn = server.arg("waterOn").toInt();
    int newWaterOff = server.arg("waterOff").toInt();

    if (newWaterOn >= 0 && newWaterOn <= 100 &&
        newWaterOff >= 0 && newWaterOff <= 100) {
      watering_threshold_on = newWaterOn;
      watering_threshold_off = newWaterOff;
      Serial.print("Watering thresholds updated: ON=");
      Serial.print(watering_threshold_on);
      Serial.print("%, OFF=");
      Serial.print(watering_threshold_off);
      Serial.println("%");
      doc["status"] = "success";
      doc["message"] = "Thresholds updated successfully.";
    }
  }
  String jsonResponse;
  serializeJson(doc, jsonResponse);
  server.send(200, "application/json", jsonResponse);
}

void setup() {
  Serial.begin(115200);

  dht.begin();
  pinMode(RAIN_SENSOR_PIN, INPUT_PULLUP);
  pinMode(PUMP_RELAY_PIN, OUTPUT);
  digitalWrite(PUMP_RELAY_PIN, HIGH);

  setupOLED();

  Serial.println("Agri-Guard System Starting Up...");

  Serial.print("Setting up AP: ");
  Serial.println(ssid);
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP Address: ");
  Serial.println(myIP);

  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("AP Mode Active!");
  display.print("SSID: ");
  display.println(ssid);
  display.print("IP: ");
  display.println(myIP);
  display.display();
  delay(2000);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/data", HTTP_GET, handleGetData);
  server.on("/pumpOn", HTTP_GET, handlePumpOn);
  server.on("/pumpOff", HTTP_GET, handlePumpOff);
  server.on("/updateThresholds", HTTP_GET, handleUpdateThresholds);
  server.begin();
  Serial.println("HTTP server started on AP.");
}

void loop() {
  server.handleClient();

  readDHT11();
  readSoilMoisture();
  readRainSensor();

  automatedWatering();
  assessRisk();

  updateOLED();

  delay(5000);
}
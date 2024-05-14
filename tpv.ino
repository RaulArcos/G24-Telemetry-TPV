#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include "FS.h"
#include <LittleFS.h>
#include <esp_task_wdt.h>
#include "time.h"
#include "soc/rtc_wdt.h"

#define RSTBTN_PIN 27
#define MULTIBTN_PIN 14
#define LASER_PIN 32

void handleResetPress();
void handleMultiFunctionPress();
void onResetPressed();
void onMultiFunctionPressed();
void onLaserActivated();
void waitForLaser(int print_height);
void tftTitle();
void showMenu();
bool runSkidpad();
void handleButtons();

const char* ssid = "FormulaGades";
const char* password = "g24evo24";
WiFiServer server(8080);

volatile bool multiFunctionPressed = false;
volatile bool resetPressed = false;
volatile bool laserActivated = false;
unsigned long debounceDelay = 200;
unsigned long lastDebounceTime = 0;
unsigned long lastLaserTime = 0;
int menuSelection = 0;
int timesMenuSelection = 0;
unsigned long startTime = 0;
unsigned long elapsedTime = 0;
bool timerRunning = false;

enum Mode { MENU, SKIDPAD, AUTOCROSS, ENDURANCE, ACCELERATION, TIMES};
enum Result {FAILED, SAVED, NOT_IMPROVED, IMPROVED, BEST};
Mode currentMode = MENU;

TFT_eSPI tft = TFT_eSPI();

typedef struct {
  unsigned long leftTime;
  unsigned long rightTime;
  unsigned long avgTime;
} Skidpad;

typedef struct {
  unsigned long time;
} AutoCross;

typedef struct {
  unsigned long time;
} Endurance;

enum class SkidpadFase{
  READY = 0,
  FIRSTRIGHT = 1,
  FIRSTLEFT = 2,
  SECONDRIHT = 3,
  SECONDLEFT = 4,
  END = 5,
  EXIT = 6
};

enum class AutoCrossFase{
  READY = 0,
  LAP = 1,
  END = 2,
  EXIT = 3
};

void handleResetPress() {
  if (currentMode == MENU) {
    menuSelection = (menuSelection + 1) % 6; // Cycle through menu options
    showMenu();
  } else if (currentMode == SKIDPAD) {
    currentMode = MENU;
    showMenu();
  } else if (currentMode == TIMES) {
    timesMenuSelection = (timesMenuSelection + 1) % 5;
    showTimesMenu();
  }
}

void handleMultiFunctionPress() {
  if (currentMode == MENU) {
    switch (menuSelection) {
      case 0: currentMode = SKIDPAD; runSkidpad(); break;
      case 1: currentMode = AUTOCROSS; runAutoCross(); break;
      case 2: currentMode = ENDURANCE; runEndurance(); break;
      case 4: currentMode = TIMES; showTimesMenu(); break;
      case 5: clearTimes(); showMenu(); break;
    }
  } else if (currentMode == TIMES) {
    switch (timesMenuSelection){
      case 0: currentMode = SKIDPAD; showSkidpadTimes(); break;
      case 1: currentMode = AUTOCROSS; showAutocrossTimes(); break;
    }
  } 
}

void onResetPressed() {
  if ((millis() - lastDebounceTime) > debounceDelay) {
    resetPressed = true;
    lastDebounceTime = millis();
  }
}

void onMultiFunctionPressed() {
  if ((millis() - lastDebounceTime) > debounceDelay) {
    multiFunctionPressed = true;
    lastDebounceTime = millis();
  }
}

void onLaserActivated() {
  if ((millis() - lastLaserTime) > 2500) {
    laserActivated = true;
    lastLaserTime = millis();
  }
}

void waitForLaser(int print_height = 0) {
  while(laserActivated == false){
    tft.setCursor(5, print_height);
    elapsedTime = millis() - startTime;
    tft.print("Tiempo: ");
    tft.print(elapsedTime / 1000.0, 3);
    tft.print(" s");
  }
  laserActivated = false;
}

void tftTitle(){
  tft.fillScreen(TFT_WHITE);
  tft.setCursor(20, 5);
  tft.setTextSize(4);
  tft.println("TPV - FG");
}

void clearTimes() {
  // List of all files to clear
  const char* files[] = {"/skidpad_best.txt", "/skidpad_lasts.txt", "/autocross_best.txt", "/autocross_lasts.txt", "/endurance_best.txt", "/endurance_lasts.txt", "/acceleration_best.txt", "/acceleration_lasts.txt"};
  for (int i = 0; i < (sizeof(files) / sizeof(files[0])); i++) {
    if (LittleFS.exists(files[i])) {
      LittleFS.remove(files[i]);
    }
  }
  Serial.println("All times cleared.");
}

void showMenu() {
  tftTitle();
  tft.setCursor(45, 50);
  tft.setTextSize(2);
  tft.println("Select Mode:");
  tft.setTextSize(3);
  tft.setCursor(20, 70);
  switch (menuSelection) {
    case 0: tft.setCursor(50, 80); tft.print("Skidpad"); break;
    case 1: tft.setCursor(40, 80); tft.print("Autocross"); break;
    case 2: tft.setCursor(40, 80); tft.print("Endurance"); break;
    case 3: tft.setCursor(15, 80); tft.print("Acceleration"); break;
    case 4: tft.setCursor(50, 80); tft.print("Tiempos"); break;
    case 5: tft.setCursor(50, 80); tft.print("Clear Times"); break;
  }
}

void showTimesMenu(){
  tftTitle();
  tft.setCursor(40, 50);
  tft.setTextSize(3);
  tft.println("TIEMPOS");
  tft.setTextSize(2);
  tft.setCursor(45, 80);
  tft.println("Categoria:");
  tft.setTextSize(3);
  tft.setCursor(20, 110);
  switch (timesMenuSelection) {
    case 0: tft.setCursor(50, 110); tft.print("Skidpad"); break;
    case 1: tft.setCursor(40, 110); tft.print("Autocross"); break;
    case 2: tft.setCursor(40, 110); tft.print("Endurance"); break;
    case 3: tft.setCursor(15, 110); tft.print("Acceleration"); break;
    case 4: tft.setCursor(50, 110); tft.print("Volver"); break;
    case 5: tft.setCursor(15, 110); tft.print("Borrar"); break;
  }
}

Result process_endurance_data(Endurance endurance) {
  const char* lastsFilename = "/endurance_lasts.txt";

  // Get current date and time
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char dateTime[64];
  strftime(dateTime, sizeof(dateTime), "%d/%m/%Y - %H:%M:%S", &timeinfo);

  // Append the new time to the lasts file
  fs::File file = LittleFS.open(lastsFilename, FILE_APPEND);
  if (!file) {
    Serial.println("Error opening endurance lasts file");
    return FAILED;
  }
  file.printf("%lu,%s\n", endurance.time, dateTime);
  file.close();
  Serial.println("Endurance time added");

  return SAVED;
}

void runEndurance() {
  tftTitle();
  tft.setCursor(50, 40);
  tft.setTextSize(3);
  tft.println("Endurance");
  tft.setTextSize(2);
  Endurance endurance;
  tft.setCursor(5, 70);
  tft.print("Ready! Green Flag!");
  while(laserActivated == false);
  laserActivated = false;

  startTime = millis();
  while (resetPressed == false) {
    handleButtons();
    waitForLaser(90);
    endurance.time = millis() - startTime;
    startTime = millis(); // Reset lap start time for next lap
    
    // Process and save the current lap time
    process_endurance_data(endurance);
  }
  // Optionally, handle the end of Endurance mode, e.g., show results or go back to the menu
  showMenu();
}

void showSkidpadTimes(){
  tftTitle();
  tft.setCursor(45, 50);
  tft.setTextSize(3);
  tft.print("Skidpad");
  tft.setCursor(50, 80);
  tft.print("Tiempo");
  tft.setTextSize(2);
  const char* filename = "/skidpad.txt";
  fs::File file = LittleFS.open(filename, FILE_READ);
  if (!file) {
    tft.println("No hay tiempos guardados");
  }else{
    // Suponiendo que solo hay una línea que seguir este formato.
    if(file.available()){
      String line = file.readStringUntil('\n');
      // Divide la línea en tres partes usando ',' como delimitador
      int firstCommaIndex = line.indexOf(',');
      int secondCommaIndex = line.indexOf(',', firstCommaIndex + 1);

      // Extrae cada tiempo como String
      String rightTimeStr = line.substring(0, firstCommaIndex);
      String leftTimeStr = line.substring(firstCommaIndex + 1, secondCommaIndex);
      String avgTimeStr = line.substring(secondCommaIndex + 1);

      // Convierte los Strings a float para mantener los decimales
      float rightTime = rightTimeStr.toFloat() / 1000;
      float leftTime = leftTimeStr.toFloat() / 1000;
      float avgTime = avgTimeStr.toFloat() / 1000;

      // Imprime los valores en el TFT
      tft.setCursor(5, 120);
      tft.printf("Drcha: %.3f\n", rightTime);
      tft.setCursor(5, 140);
      tft.printf("Izq: %.3f\n", leftTime);
      tft.setCursor(5, 160);
      tft.printf("Avg: %.3f\n", avgTime);
    }
  }
  file.close();
  while(resetPressed == false);
  resetPressed = false;
  menuSelection = 0;
  currentMode = MENU;
  showMenu();
}

void showAutocrossTimes(){
  tftTitle();
  tft.setCursor(45, 50);
  tft.setTextSize(3);
  tft.print("Autocross");
  tft.setCursor(50, 80);
  tft.print("Tiempo");
  tft.setTextSize(2);
  const char* filename = "/autocross.txt";
  fs::File file = LittleFS.open(filename, FILE_READ);
  if (!file) {
    tft.println("No hay tiempos guardados");
  }else{
    if(file.available()){
      String line = file.readStringUntil('\n');
      String timeStr = line;

      float time = timeStr.toFloat() / 1000;

      tft.setCursor(5, 120);
      tft.printf("Tiempo: %.3f\n", time);
    }
  }
  file.close();
  while(resetPressed == false);
  resetPressed = false;
  menuSelection = 0;
  currentMode = MENU;
  showMenu();
}

Result process_autocross_data(AutoCross autocross) {
  // Ensure NTP time is synchronized before calling this function

  // Define filenames for the best and last times
  const char* bestFilename = "/autocross_best.txt";
  const char* lastsFilename = "/autocross_lasts.txt";

  // Get current date and time
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char dateTime[64];
  strftime(dateTime, sizeof(dateTime), "%d/%m/%Y - %H:%M:%S", &timeinfo); // Format date and time

  // Check and update best time
  bool isBest = false;
  unsigned long bestTime = ULONG_MAX;

  if (LittleFS.exists(bestFilename)) {
    fs::File bestFile = LittleFS.open(bestFilename, FILE_READ);
    if (!bestFile) {
      Serial.println("Error al abrir el archivo de mejor tiempo");
      return FAILED;
    }
    if (bestFile.available()) {
      String bestTimeStr = bestFile.readStringUntil(',');
      bestTime = strtoul(bestTimeStr.c_str(), NULL, 10);
    }
    bestFile.close();
  }

  if (autocross.time < bestTime) {
    isBest = true;
    fs::File bestFile = LittleFS.open(bestFilename, FILE_WRITE);
    if (!bestFile) {
      Serial.println("Error al actualizar el archivo de mejor tiempo");
      return FAILED;
    }
    bestFile.printf("%lu,%s\n", autocross.time, dateTime);
    bestFile.close();
    Serial.println("Mejor tiempo actualizado");
  }

  fs::File lastsFile = LittleFS.open(lastsFilename, FILE_APPEND);
  if (!lastsFile) {
    Serial.println("Error al abrir el archivo de últimos tiempos");
    return FAILED;
  }
  lastsFile.printf("%lu,%s\n", autocross.time, dateTime);
  lastsFile.close();
  Serial.println("Tiempo añadido a últimos tiempos");

  return isBest ? BEST : SAVED;
}

Result process_skidpad_data(Skidpad skidpad) {
  // Ensure NTP time is synchronized before calling this function

  const char* bestFilename = "/skidpad_best.txt";
  const char* lastsFilename = "/skidpad_lasts.txt";

  // Get current date and time
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  char dateTime[64];
  strftime(dateTime, sizeof(dateTime), "%d/%m/%Y - %H:%M:%S", &timeinfo); // Format date and time

  bool isBest = false;
  unsigned long bestTime = ULONG_MAX;

  if (LittleFS.exists(bestFilename)) {
    fs::File bestFile = LittleFS.open(bestFilename, FILE_READ);
    if (!bestFile) {
      Serial.println("Error al abrir el archivo de mejor tiempo");
      return FAILED;
    }
    if (bestFile.available()) {
      String bestTimeStr = bestFile.readStringUntil(',');
      bestTime = strtoul(bestTimeStr.c_str(), NULL, 10);
    }
    bestFile.close();
  }

  if (skidpad.avgTime < bestTime) {
    isBest = true;
    fs::File bestFile = LittleFS.open(bestFilename, FILE_WRITE);
    if (!bestFile) {
      Serial.println("Error al actualizar el archivo de mejor tiempo");
      return FAILED;
    }
    bestFile.printf("%lu,%lu,%lu,%s\n", skidpad.rightTime, skidpad.leftTime, skidpad.avgTime, dateTime);
    bestFile.close();
    Serial.println("Mejor tiempo actualizado");
  }

  fs::File lastsFile = LittleFS.open(lastsFilename, FILE_APPEND);
  if (!lastsFile) {
    Serial.println("Error al abrir el archivo de últimos tiempos");
    return FAILED;
  }
  lastsFile.printf("%lu,%lu,%lu,%s\n", skidpad.rightTime, skidpad.leftTime, skidpad.avgTime, dateTime);
  lastsFile.close();
  Serial.println("Tiempo añadido a últimos tiempos");

  return isBest ? BEST : SAVED;
}

bool runAutoCross(){
  tftTitle();
  tft.setCursor(50, 40);
  tft.setTextSize(3);
  tft.println("Autocross");
  tft.setTextSize(2);
  AutoCross autocross;
  AutoCrossFase fase = AutoCrossFase::READY;
  while(fase != AutoCrossFase::EXIT){
    if (resetPressed) {
      resetPressed = false;
      return false;
    }
    handleButtons();
    switch(fase){
      case AutoCrossFase::READY:
        tft.setCursor(5, 70);
        tft.print("Ready! Green Flag!");
        while(laserActivated == false);
        laserActivated = false;
        fase = AutoCrossFase::LAP;
        break;
      case AutoCrossFase::LAP:
        tft.setCursor(5, 70);
        tft.print("                            ");
        tft.setCursor(5, 70);
        tft.print("Autocross ---->");
        startTime = millis();
        tft.setCursor(5, 90);
        waitForLaser(90);
        autocross.time = millis() - startTime;
        fase = AutoCrossFase::END;
        break;
      case AutoCrossFase::END:
        tft.setTextSize(3);
        tft.setCursor(5, 250);
        tft.print("Tiempo: ");
        tft.print(autocross.time / 1000.0, 4);
        tft.print(" s");
        Result autocross_result = process_autocross_data(autocross);
        if (autocross_result == SAVED) {
          tft.fillRect(5, 290, 230, 25, TFT_GREEN);
        } else if (autocross_result == NOT_IMPROVED) {
          tft.fillRect(5, 290, 230, 50, TFT_YELLOW);
        } else if (autocross_result == IMPROVED) {
          tft.fillRect(5, 290, 230, 50, TFT_GREEN);
        } else if (autocross_result == BEST) {
          tft.fillRect(5, 290, 230, 50, TFT_PURPLE);
        }
        while(resetPressed == false);
        resetPressed = false;
        menuSelection = 0;
        currentMode = MENU;
        fase = AutoCrossFase::EXIT;
        break;
    }
    if (fase == AutoCrossFase::EXIT) {
      showMenu();
    }
  }
  return true;
}

bool runSkidpad() {
  tftTitle();
  tft.setCursor(50, 40);
  tft.setTextSize(3);
  tft.println("Skidpad");
  tft.setTextSize(2);
  Skidpad skidpad;
  SkidpadFase fase = SkidpadFase::READY;
  while(fase != SkidpadFase::EXIT){
    if (resetPressed) {
      resetPressed = false;
      return false;
    }
    handleButtons();
    switch(fase){
      case SkidpadFase::READY:
        tft.setCursor(5, 70);
        tft.print("Ready! Green Flag!");
        while(laserActivated == false);
        laserActivated = false;
        fase = SkidpadFase::FIRSTRIGHT;
        break;
      case SkidpadFase::FIRSTRIGHT:
        tft.setCursor(5, 70);
        tft.print("                            ");
        tft.setCursor(5, 70);
        tft.print("Skidpad 1a ---->");
        startTime = millis();
        tft.setCursor(5, 90);
        waitForLaser(90);
        fase = SkidpadFase::SECONDRIHT;
        break;
      case SkidpadFase::SECONDRIHT:
        tft.setCursor(5, 110);
        tft.print("Skidpad 2a ---->");
        startTime = millis();
        tft.setCursor(5, 130);
        waitForLaser(130);
        skidpad.rightTime = millis() - startTime;
        fase = SkidpadFase::FIRSTLEFT;
        break;
      case SkidpadFase::FIRSTLEFT:
        tft.setCursor(5, 150);
        tft.print("Skidpad 1a <----");
        startTime = millis();
        tft.setCursor(5, 170);
        waitForLaser(170);
        fase = SkidpadFase::SECONDLEFT;
        break;
      case SkidpadFase::SECONDLEFT:
        tft.setCursor(5, 190);
        tft.print("Skidpad 2a <----");
        startTime = millis();
        tft.setCursor(5, 210);
        waitForLaser(210);
        skidpad.leftTime = millis() - startTime;
        fase = SkidpadFase::END;
        break;
      case SkidpadFase::END:
        skidpad.avgTime = (skidpad.rightTime + skidpad.leftTime) / 2;
        tft.setTextSize(3);
        tft.setCursor(5, 250);
        tft.print("Avg: ");
        tft.print(skidpad.avgTime / 1000.0, 4);
        tft.print(" s");
        Result skidpad_result = process_skidpad_data(skidpad);
        if (skidpad_result == SAVED) {
          tft.fillRect(5, 290, 230, 25, TFT_GREEN);
        } else if (skidpad_result == NOT_IMPROVED) {
          tft.fillRect(5, 290, 230, 50, TFT_YELLOW);
        } else if (skidpad_result == IMPROVED) {
          tft.fillRect(5, 290, 230, 50, TFT_GREEN);
        } else if (skidpad_result == BEST) {
          tft.fillRect(5, 290, 230, 50, TFT_PURPLE);
        }
        while(resetPressed == false);
        resetPressed = false;
        menuSelection = 0;
        currentMode = MENU;
        fase = SkidpadFase::EXIT;
        break;
    }
    if (fase == SkidpadFase::EXIT) {
      showMenu();
    }
  }
  return true;
}

void showWifiSetup(){
  tftTitle();
  tft.setCursor(5, 50);
  tft.setTextSize(2);
  tft.println("Conectando a la red");
  tft.setCursor(5, 70);
  tft.print("RESET para cancelar");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    handleButtons();
    if (resetPressed){
      resetPressed = false;
      return;
    }
    tft.setCursor(100, 90);
    tft.print(".");
    delay(100);
    tft.setCursor(100, 90);
    tft.print("..");
    delay(100);
    tft.setCursor(100, 90);
    tft.print("...");
    delay(100);
    tft.setCursor(100, 90);
    tft.print("   ");
  }
  tft.setCursor(5, 110);
  tft.println("Conectado a la red");
  tft.setCursor(40, 130);
  tft.println("Direccion IP: ");
  tft.setCursor(35, 150);
  tft.println(WiFi.localIP());
  tft.setCursor(5, 180);
  tft.print("RESET para continuar");
  while(resetPressed == false){
    handleButtons();
  }
  tft.fillScreen(TFT_WHITE);
}

void handleButtons(){
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (multiFunctionPressed) {
      multiFunctionPressed = false;
      handleMultiFunctionPress();
    }

    if (resetPressed) {
      resetPressed = false;
      handleResetPress();
    }
  }
}

String readBestAndLastTimes(String baseFilename) {
  String bestTimeFile = baseFilename + "_best.txt";
  String lastTimesFile = baseFilename + "_lasts.txt";
  String displayText = "Best Time: ";

  // Read and process the best time
  if (LittleFS.exists(bestTimeFile)) {
    fs::File file = LittleFS.open(bestTimeFile, FILE_READ);
    if (file && file.available()) {
      String line = file.readStringUntil('\n');
      displayText += processLine(line);
    } else {
      displayText += "N/A";
    }
    file.close();
  } else {
    displayText += "N/A";
  }

  displayText += "<br>---------------<br>Last Times:<br>";

  // Read and process last times
  if (LittleFS.exists(lastTimesFile)) {
    fs::File file = LittleFS.open(lastTimesFile, FILE_READ);
    if (file) {
      while(file.available()){
        String line = file.readStringUntil('\n');
        if(line.length() > 0) { // Check if the line is not empty
          displayText += processLine(line) + "<br>";
        }
      }
    }
    file.close();
  } else {
    displayText += "No last times recorded";
  }

  return displayText;
}

// Helper function to process each line of the file
String processLine(String line) {
  String processedLine = "";
  int lastCommaIndex = line.lastIndexOf(',');
  String date = line.substring(lastCommaIndex + 1); // Extract the date
  String times = line.substring(0, lastCommaIndex); // Extract the times

  // Manually split times by comma
  int start = 0;
  int end = times.indexOf(',', start);
  while (end != -1) {
    float timeValue = times.substring(start, end).toFloat() / 1000.0; // Convert to seconds
    processedLine += String(timeValue, 3); // Format with 3 decimal places
    processedLine += ", "; // Separate times with a comma
    start = end + 1;
    end = times.indexOf(',', start);
  }

  // Process the last time segment before the date
  float lastTimeValue = times.substring(start).toFloat() / 1000.0; // Convert to seconds
  processedLine += String(lastTimeValue, 3); // Format with 3 decimal places

  processedLine += " | " + date; // Append the date separated by ' | '
  return processedLine;
}
void webServer(void *pvParameters){
  server.begin();
  Serial.println("Server started");
  while (true) {
    WiFiClient client = server.available();
    if (!client) {
      vTaskDelay(1 / portTICK_PERIOD_MS);
      continue;
    }
    Serial.println("New client");
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.write(c);
        if (c == '\n') {
          if (currentLine.length() == 0) {
            // Send the standard HTTP response headers
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println();

            // Start the HTML document
            client.println("<html>");
            client.println("<head>");
            client.println("<title>TPV - FG</title>");
            client.println("<style>td { text-align: center; }</style>"); // Center text in table cells
            client.println("</head>");
            client.println("<body>");
            client.println("<h1>TPV - FG</h1>");

            // Setup a table for horizontal display
            client.println("<table width='100%'>");
            client.println("<tr><th>Skidpad</th><th>Autocross</th><th>Endurance</th><th>Acceleration</th></tr>");
            client.println("<tr>");
            client.println("<td>" + readBestAndLastTimes("/skidpad") + "</td>");
            client.println("<td>" + readBestAndLastTimes("/autocross") + "</td>");
            client.println("<td>" + readBestAndLastTimes("/endurance") + "</td>");
            client.println("<td>" + readBestAndLastTimes("/acceleration") + "</td>");
            client.println("</tr>");
            client.println("</table>");

            // End the HTML document
            client.println("</body>");
            client.println("</html>");
            client.println();
            break;
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
      vTaskDelay(1 / portTICK_PERIOD_MS);
      rtc_wdt_feed();
    }
    client.stop();
    rtc_wdt_feed();
    Serial.println("Client disconnected");
    vTaskDelay(1 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(RSTBTN_PIN, INPUT_PULLUP);
  pinMode(MULTIBTN_PIN, INPUT_PULLUP);
  pinMode(LASER_PIN, INPUT_PULLUP);

  rtc_wdt_protect_off();    // Turns off the automatic wdt service
  rtc_wdt_enable();         // Turn it on manually
  rtc_wdt_set_time(RTC_WDT_STAGE0, 100000);  // Define how long you desire to let dog wait.

  attachInterrupt(digitalPinToInterrupt(RSTBTN_PIN), onResetPressed, FALLING);
  attachInterrupt(digitalPinToInterrupt(MULTIBTN_PIN), onMultiFunctionPressed, FALLING);
  attachInterrupt(digitalPinToInterrupt(LASER_PIN), onLaserActivated, FALLING);

  if (!LittleFS.begin()) {
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  tft.init();
  tft.fillScreen(TFT_WHITE);
  tft.setRotation(2);
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  showWifiSetup();
 // xTaskCreate(
  //  webServer,          // Task function
   // "WebServerTask",    // Name of the task (for debugging)
    //100000,              // Stack size (bytes)
    //NULL,               // Parameter to pass to the task
    //tskIDLE_PRIORITY,                  // Priority (higher numbers = higher priority)
    //NULL);              // Task handle (not needed here)
  showMenu();

  configTime(0,0, "pool.ntp.org", "time.nist.gov");
  Serial.println("Waiting for NTP time sync");
  time_t now = time(nullptr);
  while (now < 24 * 3600) {
    delay(100);
    now = time(nullptr);
  }
  Serial.println("Time synchronized");
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void loop() {
  handleButtons();
  //rtc_wdt_feed();
  // Serial.print("Free heap memory: ");
  // Serial.println(esp_get_free_heap_size());
}

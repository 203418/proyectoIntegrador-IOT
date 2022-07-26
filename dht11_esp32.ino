#include <DHT.h>
#include <DHT_U.h> 
#include <HTTPClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoJson.h>
#include <esp_wifi.h>
#define trigPin 14
#define echoPin 13
#define bomba 26
#define sensorHum 34

DHT dht(5,DHT11);

const char *ssid = "kevin";
const char *password = "12345678";

uint8_t newMACAddress[] = {0x94, 0x08, 0x53, 0x39, 0xae, 0x87};

float t, humedad, distance1, temperatureC, estado_bomba, hS, agua;

String datos;

const int sensorPin = 2;

// GPIO where the DS18B20 is connected to
const int oneWireBus = 4;     

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);



float temp, hume;

int lcdColumns = 16;
int lcdRows = 2;
int lightInit;
int lightVal;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

void setup(){
  // Inicializamos comunicación serie
  Serial.begin(115200);
  digitalWrite(bomba, LOW);
  //conect_to_wifi(); //funcion para conectarse a wifi
 
  // Comenzamos el sensor DHT
  dht.begin();

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(34, INPUT);
  
  sensors.begin();

  lcd.init();
  
  lcd.backlight();

  pinMode(bomba, OUTPUT);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(trigPin, LOW);
  

  estado_bomba = 0;

  // mac custom
  Serial.begin(115200);
  Serial.println();
  WiFi.mode(WIFI_STA);
  Serial.print("[OLD] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());
  esp_wifi_set_mac(WIFI_IF_STA, &newMACAddress[0]);
  Serial.print("[NEW] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());

  //Conectar wifi 
  WiFi.begin(ssid, password);
  Serial.print("Conectando...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("Conectado con éxito, mi IP es: ");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.macAddress());

  lightInit = analogRead(sensorPin);
  
}

void loop() {
  estado_bomba = 0;
  HTTPClient http;
  http.begin("http://192.168.89.218:3001/api/datos");
  http.addHeader("Content-Type","application/x-www-form-urlencoded");
  
  // Leemos la temperatura en grados centígrados (por defecto)
  t = dht.readTemperature();
 
  // Comprobamos si ha habido algún error en la lectura
  if (isnan(t)) {
    Serial.println("Error obteniendo los datos del sensor DHT11");
    return;
  }


  distance1 = getDistance();
  humedad= analogRead(34); // 4095 seco y 1554 humedad perfecta
  sensors.requestTemperatures(); 
  temperatureC = sensors.getTempCByIndex(0);
  lightVal = analogRead(sensorPin);
  hS = ((4095.0 - humedad) * 100.0) / 4095.0;

  agua = ((14 - distance1) * 100.0) / 7;

  Serial.println("datos: "+String(datos));

  Serial.println(hS);
  
  Serial.println("------------------------------");
  Serial.print(t);
  Serial.print(",");
  Serial.print(distance1);
  Serial.print(",");
  Serial.print(hS);
  Serial.print(",");
  Serial.print(temperatureC);
  Serial.println(",");
  Serial.println("------------------------------");
  lcd.setCursor(0,0);
  lcd.print(t);
  lcd.print(",");
  lcd.print(distance1);
  lcd.print(",");
  lcd.print(String(hS) + "%");
  lcd.print(",");
  lcd.setCursor(0,1);
  lcd.print(temperatureC);
  lcd.print(",");
  lcd.print(lightVal - lightInit);
  regar(hS);
  enviar_datos();
  delay(1000);
}

void regar(float hS)
{     
   if (hS < 51 && agua > 25 && temperatureC < 38)
  { 
    lcd.setCursor(0,2);
    lcd.print("Suelo: regando");
    digitalWrite(bomba,HIGH);
    estado_bomba = 1;
    enviar_datos();
    delay(3000);
    digitalWrite(bomba,LOW);
    estado_bomba = 0;
    lcd.clear();
  }
      else{
        digitalWrite(bomba,LOW);
    }
}

void enviar_datos(){
   if (WiFi.status() == WL_CONNECTED){
    
    HTTPClient http;

    String code = "123456";
    
    datos = String(t) + ","+ String(hS) + "," + String(agua)+ ","+String(lightVal - lightInit) + ","+String(temperatureC) + ","+ String(estado_bomba);
    String json;
    StaticJsonDocument<300> doc;
    doc["data"]=datos;
    doc["code"]=code;
    serializeJson(doc, json);
    Serial.println(json);
    //Indicamos el destino
    http.begin("http://192.168.89.242:8000/api/plant");
    http.addHeader("Content-Type", "application/json");
    
    int status_response = http.POST(json);
    
    if (status_response > 0){
      Serial.println("Código HTTP ► " + String(status_response)); // Print return code

      if (status_response == 200){
        String data_response= http.getString();
        Serial.println("El servidor respondió ▼ ");
        Serial.println(data_response);
      }
    }else{
      String data_response= http.getString();
      Serial.print(" Error enviando POST, código: ");
      Serial.println(status_response);
      Serial.println(data_response);
    }
    http.end(); 
  }
}

int getDistance(){
  int duration, distance;
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(1000);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = (duration/2)/29.1; // CM
  return distance;
}

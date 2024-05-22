/*
Para usar la placa NodeMCU Amica usar NodeMCU 0.9 (ESP-12 Module)
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <Rfid134.h>
#include "configHX711_ADC.h"
#include <HX711_ADC.h>
#include "SuperMon.h"


//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

float balanzaLastData;

const char* ssid = "Balanza"; // Nombre del Access Point
const char* password = "balanza"; // Contraseña para el Access Point
const char* domainName = "balanza.local"; // Dominio personalizado

ESP8266WebServer server(80); // Crear una instancia del servidor web en el puerto 80
DNSServer dnsServer;        // Crear una instancia del servidor DNS


/****** Lector de caravana ******/

//Variable global. La uso para almacenar los datos recibidos por RfidNotify()
bool lc_newData = false;
unsigned char lc_error;
unsigned long  lc_ID;

// implement a notification class,
// its member methods will get called 
//
class RfidNotify
{
public:
  static void OnError(Rfid134_Error errorCode)
  {
    lc_error = errorCode;
    lc_newData = true;
  }

  static void OnPacketRead(const Rfid134Reading& reading)
  {
    char temp[8];

    lc_newData = true;
    lc_ID = reading.id;
  }

};

// instance a Rfid134 object, 
// defined with the above notification class and the hardware serial class
//
//  Voy a usar la RX del Serial para recibir y el Tx lo dejo para transmitir los datos a la pc
Rfid134<HardwareSerial, RfidNotify> rfid(Serial);

/****** FIN Lector de caravana ******/


void handleRoot() {
  Serial.println("sending web page");
  server.send(200, "text/html", PAGE_MAIN);
}


char XML[2048];
char buf[32];


// code to send the main web page
// I avoid string data types at all cost hence all the char mainipulation code
void sendXML() {
  char aux;
    
  //Serial.println("sending xml");

  strcpy(XML, "<?xml version = '1.0'?>\n<Data>\n");

  // send caravana nueva true o false
  if(lc_newData) aux = 1;
  else aux = 0;
  sprintf(buf, "<caravananew>%d</caracananew>\n", aux );
  strcat(XML, buf);

  // send caravana
  sprintf(buf, "<caravana>%d</caravana>\n", String(lc_ID));
  strcat(XML, buf);

  // send Peso
  sprintf(buf, "<peso>%d</peso>\n", String(balanzaLastData));
  strcat(XML, buf);

  strcat(XML, "</Data>\n");
  // wanna see what the XML code looks like?
  // actually print it to the serial monitor and use some text editor to get the size
  // then pad and adjust char XML[2048]; above
  Serial.println(XML);

  // you may have to play with this value, big pages need more porcessing time, and hence
  // a longer timeout that 200 ms
  server.send(200, "text/xml", XML);
}


void handlePeso(){
  server.send(200, "text/data", String(balanzaLastData));
}

void handlePesoEstable(){
  server.send(501, "text/data", "Función no implementada");
}

void handleCalibrarPeso(){
 String s_calibration = server.arg("VALUE");

  // conver the string sent from the web page to an int
  float calibrationValue = s_calibration.toFloat();
  LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
  server.send(200, "text/data", "Se calibra el sensor" + String(calibrationValue));
}

void handleTarar(){
  LoadCell.tareNoDelay();
  server.send(200, "text/data", "");
}

void handleCaravanaData(){
  server.send(200, "text/data", String(lc_ID));
  lc_newData = false;
}

void handleCaravanaNewData(){

  if(lc_newData){
      server.send(200, "text/data", "Nuevo dato disponible");
  }else{
      server.send(200, "text/data", "Sin dato nuevo");
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT); // Usaremos el LED integrado en la placa para indicar el estado del Access Point

  Serial.begin(57600);
  Serial.println();

  Serial.println("Esperar 10s para tarar");
  LoadCell.begin();
  LoadCell.setReverseOutput(); //uncomment to turn a negative output value to positive
  unsigned long stabilizingtime = 10000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);
  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    float calibrationValue = 139.45;
    LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
    Serial.println("Startup is complete");
  }


  // Configurar el ESP8266 como Access Point
  WiFi.softAP(ssid, password);

  // Obtener la dirección IP del Access Point
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("Punto de Acceso creado: ");
  Serial.println(ssid);
  Serial.print("Dirección IP del Access Point: ");
  Serial.println(apIP);

  // Configurar el servicio DNS con el dominio "esp8266.local"
  dnsServer.start(53, domainName, apIP);
  Serial.println("Servidor DNS iniciado");

  digitalWrite(LED_BUILTIN, HIGH); // Encender el LED para indicar que el Access Point está activo

  // Manejadores de rutas para el servidor web

  server.on("/", handleRoot);
  server.on("/xml", sendXML);
  server.on("/peso", handlePeso);
  server.on("/pesoestable", handlePesoEstable);
  server.on("/calibrarpeso", handleCalibrarPeso);
  server.on("/tarar", handleTarar);
  server.on("/caravanadata", handleCaravanaData);
  server.on("/caravananew", handleCaravanaNewData);
  server.begin();

  rfid.begin();
}

void loop() {
  dnsServer.processNextRequest(); // Procesar solicitudes DNS
  server.handleClient(); // Manejar las solicitudes del servidor web
  // Tu código principal, si necesitas ejecutar alguna otra lógica mientras el Access Point está activo
  rfid.loop();
  balanzaLoop();
  
}

void balanzaLoop() {
  static boolean newDataReady = 0;
  static unsigned long t = 0;
  const int serialPrintInterval = 1000; //increase value to slow down serial print activity

  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  // get smoothed value from the dataset:
  if (newDataReady) {
    if (millis() > t + serialPrintInterval) {
      balanzaLastData = LoadCell.getData();
      Serial.print("Load_cell output val: ");
      Serial.println(balanzaLastData);
      newDataReady = 0;
      t = millis();
    }
  }
}

/*
Para usar la placa NodeMCU Amica usar NodeMCU 0.9 (ESP-12 Module)
*/

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <Rfid134.h>




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
  server.send(200, "text/html", "<h1>Buen Augurio</h1>");
}


void handlePeso(){
  float peso = obtenerValorDelSensor();
  String mensaje = "<h1>Buen Augurio</h1><p>Valor del peso: " + String(peso) + " kg</p>";
  server.send(200, "text/html", mensaje);
}

void handlePesoData(){
  float peso = obtenerValorDelSensor();
  
  server.send(200, "text/data", String(peso));

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

  // Configurar el ESP8266 como Access Point
  WiFi.softAP(ssid, password);

  // Obtener la dirección IP del Access Point
  IPAddress apIP = WiFi.softAPIP();
  Serial.begin(9600);
  Serial.println();
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
  server.on("/peso", handlePeso);
  server.on("/pesodata", handlePesoData);
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
  
}


float obtenerValorDelSensor() {
  static int x = 0;// Implementa la lógica para obtener el valor del sensor aquí
  // Reemplaza esto con la lógica específica para tu sensor
  x++;
  return 10.5 + x; // Reemplaza con el valor real obtenido del sensor
}
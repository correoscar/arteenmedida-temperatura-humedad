//************************************
//* CONTEO DE PERSONAS
//* 
//* OBJETIVO: Contar el número de accesos al centro o una de las salas del centro.
//*           Además permite controlar los parámetros de temperatura y humedad de la sala.
//* 
//* CONFIGURACIÓN: SSID: nombre de la red WIFI donde se conectará el dispositivo
//*                PASS: contraseña de la red WIFI para conectarse.
//*                CENTRO: identificador numérico del centro donde se encuentra el dispositivo.
//*                SALA: identificador numérico de la sala donde se encuentra el dispositivo.
//*
//* FECHA ÚLT ACTUALIZACIÓN: 25/03/2026
//* PROGRAMADOR: Óscar Úrbez
//************************************

//Declaración de librerías
#include <WiFi.h>             //librerías de WIFI
#include <WiFiClientSecure.h> //librería para hacer conexión por HTTPS
#include <HTTPClient.h>       //librerias para protocolo HTTP
#include <DHT.h>              //Libreria para leer temperatura y humedad

//Declaración de variables globales
int num_sala = 125;       //número de sala donde se instala el sensor
int num_centro = 1;       //número de centro donde se situa el sensor
int altura_sensor = 150;  //Distancia que permite ajustar a que altura se considera una entrada.
const String endpoint = "https://www.....com/api/"; //luego introducimos la ruta para temperatura, humedad o conteo
const String token = "....";
const int sala = 2;

//Declaración del WIFI
const char* ssid = "WIFI";
const char* pass = "mikey";

//Declaracion de relojes
long tiempoAnt = 0;
const long intervalo = 60000;

//Declaración de posicioens de los sensores
//Sensor de temperatura y humedad
#define DHTPIN 0
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//Sensor de proximidad
const int Trigger = 13;
const int Echo = 12; 

void setup() {
  // Configuración basica de puertos
  Serial.begin(115200);

  //Inicio del sensor de temperatura y humedad
  dht.begin();

  //Sensor de distancia
  pinMode(Trigger, OUTPUT); 
  pinMode(Echo, INPUT);  
  digitalWrite(Trigger, LOW);

  //Inicializando
  WiFi.begin(ssid, pass);

}

void loop() 
{

  //Controlando cada cuanto tiempo lanzamos las operaciones
  //el conteo de personas es permanente y temperatura y humedad cada 60 segundos.
  long tiempoActual = millis();
  int httpCode = 0;


  if (tiempoActual - tiempoAnt >= intervalo) 
  {
    tiempoAnt = tiempoActual;
    //Revisaremos la temperatura cada 60 segundos
    enviarInfoTemperatura(sala);
    delay(2000);
    enviarInfoHumedad(sala);
  }

  //El conteo de visitas es en todo momento.
  enviarInfoConteo(sala);

}

//*********************************************************************************
//* enviarInfoConteo()
//* permite enviar la información de conteo de persona a BBDD
//* persona - INT - número de personas que están pasando
//* salas_id - INT- identificador único de la sala donde está colocado el sensor
//*********************************************************************************
void enviarInfoConteo(int sala)
{
  digitalWrite(Trigger, HIGH);
  delayMicroseconds(10);
  digitalWrite(Trigger, LOW);

  float t = pulseIn(Echo, HIGH);  //tiempo del pulso
  float d = t/59;                 //calculado a cm.
  float alturaMinima = 50;
  float alturaMaxima = 100;
  HTTPClient http;
  int httpCode = 0;

  
  if (d > alturaMinima && d < alturaMaxima)
  {
    Serial.print("Distancia: ");
    Serial.print(d);      //Enviamos serialmente el valor de la distancia
    Serial.print("cm");
    Serial.println("****ENTRA*****");

    //Anotamos en BBDD
    if (WiFi.status() == WL_CONNECTED) 
    {
      Serial.println("Está conectado al WIFI");

      //Revisando conexión
      while (WiFi.status() != WL_CONNECTED) 
      {
        delay(500);
        Serial.print(".");
      }

      WiFiClientSecure client;
      client.setInsecure();
      String endpointPresencial = endpoint + "presencia";

      //ENVIO A API
      http.begin(client, endpointPresencial);
      http.addHeader("Content-Type", "application/json");
      String auth = "Bearer " + token;
      http.addHeader("Authorization", auth);
      String json = "{\"salas_id\":" + String(sala) + "}";
      Serial.println("Código JSON: " + json);
      int httpResponseCode = http.POST(json);  
      Serial.println("Código respuesta: " + String(httpResponseCode));
      String response = http.getString();
      Serial.println("Respuesta: " + response);
    }
    //Controlando respuesta
    if (httpCode > 0)
    {
      Serial.println("código != 0");
    }
    else
    {
      Serial.println("código = 0");
    }

    //Finalizando conexion
    //http.end();

  }
}

//***********************************************************************************
//* enviarInfoTemperaturaHumedad()
//* permite enviar la información de temperatura y humedad a BBDD
//* temperatura - DECIMAL - en grados celsius, temperatura recogida por el sensor.
//* humedad - DECIMAL - número de personas que están pasando
//* salas_id - INT- identificador único de la sala donde está colocado el sensor
//***********************************************************************************
void enviarInfoTemperatura(int sala)
{
  HTTPClient http;
  int httpCode = 0;

  // Lectura de temperatura y humedad
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float hic = dht.computeHeatIndex(t, h, false); //temperatura percibida
  
  // Si da erorr al inicializar salimos.
  if (isnan(h) || isnan(t)) {
    Serial.println("Error en la incializacion de los sensores");
    return;
  }

  Serial.print("Enviando datos de temperatura: ");
  Serial.print(t);
  Serial.print("ºC y humedad: ");
  Serial.println(h);  
  Serial.print("%, en sala: ");
  Serial.println(sala);

  //Revisando conexión
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }

  //Conectados
  if (WiFi.status() == WL_CONNECTED) 
  {
    Serial.println("Está conectado al WIFI");

    WiFiClientSecure client;
    client.setInsecure();
    String endpointTemperatura = endpoint + "temperatura";
    String endpointHumedad = endpoint + "humedad";

    //HUMEDAD
    http.begin(client, endpointHumedad);
    http.addHeader("Content-Type", "application/json");
    String auth = "Bearer " + token;
    http.addHeader("Authorization", auth);
    String json = "{\"humedad\":" + String(h) + ",\"salas_id\":" + String(sala) + "}";
    Serial.println("Código JSON: " + json);
    httpCode = http.POST(json);  
    Serial.println("Código respuesta: " + String(httpCode));
    String response = http.getString();
    Serial.println("Respuesta: " + response);
    http.end();
  }

  //Controlando respuesta
  if (httpCode > 0)
  {
    Serial.println("código != 0");
  }
  else
  {
    Serial.println("código = 0");
  }

}

//***********************************************************************************
//* enviarInfoHumedad()
//* permite enviar la información de temperatura y humedad a BBDD
//* temperatura - DECIMAL - en grados celsius, temperatura recogida por el sensor.
//* humedad - DECIMAL - número de personas que están pasando
//* salas_id - INT- identificador único de la sala donde está colocado el sensor
//***********************************************************************************
void enviarInfoHumedad(int sala)
{
  HTTPClient http;
  int httpCode = 0;

  // Lectura de temperatura y humedad
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  float hic = dht.computeHeatIndex(t, h, false); //temperatura percibida
  
  // Si da erorr al inicializar salimos.
  if (isnan(h) || isnan(t)) {
    Serial.println("Error en la incializacion de los sensores");
    return;
  }

  Serial.print("Enviando datos de temperatura: ");
  Serial.print(t);
  Serial.print("ºC y humedad: ");
  Serial.println(h);  
  Serial.print("%, en sala: ");
  Serial.println(sala);

  //Revisando conexión
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }

  //Conectados
  if (WiFi.status() == WL_CONNECTED) 
  {
    Serial.println("Está conectado al WIFI");

    WiFiClientSecure client;
    client.setInsecure();
    String endpointTemperatura = endpoint + "temperatura";
    String endpointHumedad = endpoint + "humedad";

    //HUMEDAD
    http.begin(client, endpointHumedad);
    http.addHeader("Content-Type", "application/json");
    String auth = "Bearer " + token;
    http.addHeader("Authorization", auth);
    String json = "{\"humedad\":" + String(h) + ",\"salas_id\":" + String(sala) + "}";
    Serial.println("Código JSON: " + json);
    httpCode = http.POST(json);  
    Serial.println("Código respuesta: " + String(httpCode));
    String response = http.getString();
    Serial.println("Respuesta: " + response);
    http.end();
  }

  //Controlando respuesta
  if (httpCode > 0)
  {
    Serial.println("código != 0");
  }
  else
  {
    Serial.println("código = 0");
  }

}


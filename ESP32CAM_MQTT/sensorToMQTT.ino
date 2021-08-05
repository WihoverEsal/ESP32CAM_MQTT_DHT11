/*
 * Envio de datos de sensor DHT11 por MQTT
 * 
 * Por: Víctor Estupiñán Álvarez
 * Fecha: 28/07/2021
 * 
 * Componente     PinESP32CAM     Estados lógicos
 * ledStatus------GPIO 33---------On=>LOW, Off=>HIGH
 * ledFlash-------GPIO 4----------On=>HIGH, Off=>LOW
 * 
 * Este programa envia los datos capturados por el sensor DHT11 a traves de internet. Los datos 
 * son de temperatura y humedad, el envio lo realiza por medio del protocolo MQTT. Para poder 
 * comprobar el funcionamiento de este programa, puede conectarse a un broker y usar NodeRed para
 * visualzar que la información se está recibiendo correctamente ó bien suscribirse a los temas 
 * utilizados en este programa desde la Terminal. Este programa requiere un sensor DHT11, por lo 
 * que es necesario instalar la biblioteca DHT sensor library para manipular los datos capturados 
 * por el sensor de temperatura y humedad.
 */


//Bibliotecas
#include <WiFi.h>  // Biblioteca para el control de WiFi
#include <PubSubClient.h> //Biblioteca para conexion MQTT
#include "DHT.h"// Biblioteca para poder capturar las lecturas del sensor
#include "string.h" // Biblioteca para manipular strings
#include "iostream"

//Constantes DHT
#define DHTTYPE DHT11//Constante para el tipo de sensor DHT que se usará, en este caso el sensor a usar es DHT11
#define DHTPIN 14//Constante que define el pin que se utilizara en la ESP32CAM

//Objeto dht
DHT dht(DHTPIN, DHTTYPE);//Definicion de objeto tipo DHT con las constantes anteriores

//Datos de WiFi
const char* ssid = "**********";  // Aquí debes poner el nombre de tu red
const char* password = "**********";  // Aquí debes poner la contraseña de tu red

//Datos del broker MQTT
const char* mqtt_server = "127.0.0.1"; // Si estas en una red local, coloca la IP asignada, en caso contrario, coloca la IP publica
IPAddress server(127,0,0,1);

// Objetos
WiFiClient espClient; // Este objeto maneja los datos de conexion WiFi
PubSubClient client(espClient); // Este objeto maneja los datos de conexion al broker

// Variables
int flashLedPin = 4;  // Para indicar el estatus de conexión
int statusLedPin = 33; // Para ser controlado por MQTT
long timeNow, timeLast; // Variables de control de tiempo no bloqueante
int data = 0; // Contador
int wait = 5000;  // Indica la espera cada 5 segundos para envío de mensajes MQTT

// Inicialización del programa
void setup() {
  // Iniciar comunicación serial
  Serial.begin (115200);
  pinMode (flashLedPin, OUTPUT);
  pinMode (statusLedPin, OUTPUT);
  digitalWrite (flashLedPin, LOW);
  digitalWrite (statusLedPin, HIGH);

  Serial.println();
  Serial.println();
  Serial.print("Conectar a ");
  Serial.println(ssid);
 
  WiFi.begin(ssid, password); // Esta es la función que realiz la conexión a WiFi
 
  while (WiFi.status() != WL_CONNECTED) { // Este bucle espera a que se realice la conexión
    digitalWrite (flashLedPin, HIGH);
    delay(500); //dado que es de suma importancia esperar a la conexión, debe usarse espera bloqueante
    digitalWrite (flashLedPin, LOW);
    Serial.print(".");  // Indicador de progreso
    delay (5);
  }
  
  // Cuando se haya logrado la conexión, el programa avanzará, por lo tanto, puede informarse lo siguiente
  Serial.println();
  Serial.println("WiFi conectado");
  Serial.println("Direccion IP: ");
  Serial.println(WiFi.localIP());

  // Si se logro la conexión, encender led
  if (WiFi.status () > 0){
    digitalWrite (flashLedPin, HIGH);
  }
  
  delay (1000); // Esta espera es solo una formalidad antes de iniciar la comunicación con el broker

  // Conexión con el broker MQTT
  client.setServer(server, 1883); // Conectarse a la IP del broker en el puerto indicado
  client.setCallback(callback); // Activar función de CallBack, permite recibir mensajes MQTT y ejecutar funciones a partir de ellos
  delay(1500);  // Esta espera es preventiva, espera a la conexión para no perder información

  //Inicializacion del objeto DHT para las condiciones iniciales
  dht.begin();

  timeLast = millis (); // Inicia el control de tiempo
}// fin del void setup ()

// Cuerpo del programa, bucle principal
void loop() {
  //Verificar siempre que haya conexión al broker
  if (!client.connected()) {
    reconnect();  // En caso de que no haya conexión, ejecutar la función de reconexión, definida despues del void setup ()
  }// fin del if (!client.connected())
  client.loop(); // Esta función es muy importante, ejecuta de manera no bloqueante las funciones necesarias para la comunicación con el broker  
  
  timeNow = millis(); // Control de tiempo para esperas no bloqueantes
  if (timeNow - timeLast > wait) { // Manda un mensaje por MQTT cada cinco segundos
    timeLast = timeNow; // Actualización de seguimiento de tiempo

    //Esta funcion del objeto dht permite realizar la lectura de la humedad
    float h = dht.readHumidity();//Leer la humedad
  
    //Esta funcion del objeto dht permite realizar la lectura de la temperatura
    float t = dht.readTemperature();//Leer temperatura en °C

    //Comprueba si ah habido un error de lectura
    if( isnan(h) || isnan(t) ){
      Serial.println("Ocurrio un error en la lectura");
      return;
    }
    
    //Se prepara el mensaje a enviar
    std::string msj = "Temperatura: " + std::to_string(t);
    client.publish("esp32/data/temperatura", msj.c_str());// Esta funcion publica los datos por MQTT, el primer parametro es el tema y el segundo parametro es el mensaje a enviar

    msj = "Humedad: " + std::to_string(h); // Se modifica la variable msj para que solo contenga la lectura de la Humedad
    client.publish("esp32/data/humedad", msj.c_str());// Se publica el valor de la humedad leida por el sensor DHT 11 al tema /esp32/data/humedad

  }// fin del if (timeNow - timeLast > wait)
}// fin del void loop ()

// Funciones de usuario

// Esta función permite tomar acciones en caso de que se reciba un mensaje correspondiente a un tema al cual se hará una suscripción
void callback(char* topic, byte* message, unsigned int length) {

  // Indicar por serial que llegó un mensaje
  Serial.print("Llegó un mensaje en el tema: ");
  Serial.print(topic);

  // Concatenar los mensajes recibidos para conformarlos como una varialbe String
  String messageTemp; // Se declara la variable en la cual se generará el mensaje completo  
  for (int i = 0; i < length; i++) {  // Se imprime y concatena el mensaje
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }

  // Se comprueba que el mensaje se haya concatenado correctamente
  Serial.println();
  Serial.print ("Mensaje concatenado en una sola variable: ");
  Serial.println (messageTemp);

  // En esta parte puedes agregar las funciones que requieras para actuar segun lo necesites al recibir un mensaje MQTT

  // Ejemplo, en caso de recibir el mensaje true - false, se cambiará el estado del led soldado en la placa.
  // El ESP323CAM está suscrito al tema esp/output
  if (String(topic) == "esp32/output") {  // En caso de recibirse mensaje en el tema esp32/output
    if(messageTemp == "true"){
      Serial.println("Led encendido");
      digitalWrite(statusLedPin, LOW);
    }// fin del if (String(topic) == "esp32/output")
    else if(messageTemp == "false"){
      Serial.println("Led apagado");
      digitalWrite(statusLedPin, HIGH);
    }// fin del else if(messageTemp == "false")
  }// fin del if (String(topic) == "esp32/output")
}// fin del void callback

// Función para reconectarse
void reconnect() {
  // Bucle hasta lograr conexión
  while (!client.connected()) { // Pregunta si hay conexión
    Serial.print("Tratando de contectarse...");
    // Intentar reconexión
    if (client.connect("ESP32CAMClient")) { //Pregunta por el resultado del intento de conexión
      Serial.println("Conectado");
      client.subscribe("esp32/output"); // Esta función realiza la suscripción al tema
    }// fin del  if (client.connect("ESP32CAMClient"))
    else {  //en caso de que la conexión no se logre
      Serial.print("Conexion fallida, Error rc=");
      Serial.print(client.state()); // Muestra el codigo de error
      Serial.println(" Volviendo a intentar en 5 segundos");
      // Espera de 5 segundos bloqueante
      delay(5000);
      Serial.println (client.connected ()); // Muestra estatus de conexión
    }// fin del else
  }// fin del bucle while (!client.connected())
}// fin de void reconnect(

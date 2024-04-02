#include <Arduino.h>
#include <SPI.h>
#include <MFRC522.h>
#include<FreeRTOS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <WebSockets.h>

TaskHandle_t * const task = NULL;
void ReadValue( void * parameter );

#define RST_PIN 9
//Pin 9 para el reset del RC522
#define SS_PIN 10
//Pin 10 para el SS (SDA) del RC522
MFRC522 mfrc522(SS_PIN, RST_PIN); //Creamos el objeto para el RC522
const char* ssid = "SSID";
const char* password = "PASSWORD";

// Puerto del servidor web
const uint16_t webPort = 80;

// Puerto del servidor WebSocket
const uint16_t wsPort = 81;

// Variable para almacenar el tiempo
String code;

// Servidor web
AsyncWebServer server(webPort);

// Servidor WebSocket
AsyncWebSocket ws("/ws");


void setup() {
  Serial.begin(115200); //Iniciamos la comunicación
  SPI.begin();
  //Iniciamos el Bus SPI
  mfrc522.PCD_Init(); // Iniciamos el MFRC522
  Serial.println("Lectura del UID");
  //preparamos una tasca que ejecute la lectura
  xTaskCreate(ReadValue,"ReadRFID",10000,NULL,1,task);

  // Conectarse a la red WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi conectado");

  // Iniciar el servidor web
  server.begin();

  // Iniciar el servidor WebSocket
  ws.begin();

  // Ruta para la página web
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  // Función para recibir mensajes del cliente WebSocket !!!!!!SE A DE MIERAR EL 26
  ws.onWebSocketMessage([](AsyncWebSocket *client, void *pMsg) {
    // Convertir el mensaje a String
    String msg = (char*)pMsg;

  // Si el mensaje es "gettime", enviar el tiempo actual
    if (msg == "code") {
      client->text(code);
    }
}

void ReadValue( void * parameter ){
  for(;;)
  {
    // Revisamos si hay nuevas tarjetas presentes
    if ( mfrc522.PICC_IsNewCardPresent())
    {
      //Seleccionamos una tarjeta
      if ( mfrc522.PICC_ReadCardSerial())
      {
        // Enviamos serialemente su UID
        Serial.print("Card UID:");
        for (byte i = 0; i < mfrc522.uid.size; i++) {
          Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
          Serial.print(mfrc522.uid.uidByte[i], HEX);
        }
        Serial.println();
        code = code + mfrc522.uid.uidByte[i];
        // Terminamos la lectura de la tarjeta actual
        mfrc522.PICC_HaltA();
      }
      client->text(code);
      code = "";
    }
  }
}
// Página web index.html
const char* index_html = R"(
<!DOCTYPE html>
<html>
<head>
<title>Tiempo actual</title>
</head>
<body>
<h1>Tiempo actual:</h1>
<p id="time"></p>
<script>
var socket = new WebSocket("ws://192.168.1.100:81/ws");

socket.onmessage = function(event) {
  document.getElementById("time").innerHTML = event.data;
};

socket.onopen = function() {
  socket.send("gettime");
};
</script>
</body>
</html>
)";
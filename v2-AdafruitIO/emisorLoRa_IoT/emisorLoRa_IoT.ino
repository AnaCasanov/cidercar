/*LIBRERÍAS*/
#include <heltec_unofficial.h>
#include <Adafruit_BME280.h>
#include <Preferences.h>
#include <WiFi.h>
#include "AdafruitIO_WiFi.h"

//Definir credenciales de Adafruit y WiFi
#define IO_USERNAME "Leytha"
#define IO_KEY "..."
#define WIFI_SSID "CidercarNava"
#define WIFI_PASS "tecnologia4eso"
#define INTERVALO_ENVIO 2000
/*DEFINICIÓN DE PINES*/
#define ACS_PIN 7
#define DIV_PIN 6
#define FAN_INA 19
#define FAN_INB 20
#define SDA_BME 5    
#define SCL_BME 4  
/*DEFINICIÓN DE FUNCIONES ACTIVADAS*/ 
#define RADIO_ON 1  //Enciende o apaga la emisión por radio a otra placa en base
#define REGTEMP_ON 0 //Enciende o apaga el encendido automático del ventilador cuando supera cierto umbral
#define IOT_ON 1 // Enciende o apaga el envío a IoT de los datos
#define UMBRAL_TEMP 40 //Temperatura máxima antes de encender el ventilador

/* VARIABLES GLOBALES*/
//Variables para controlar el tiempo
unsigned long t_ciclo=5;
unsigned long t_ultimo_paq;
//Objetos para utilizar el sensor de temperatura
TwoWire I2CBME = TwoWire(1);
Adafruit_BME280 bme;
//Objetos para interactuar con la plataforma IoT Adafruit
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);
AdafruitIO_Feed *feed_temp = io.feed("temperatura");
AdafruitIO_Feed *feed_fan = io.feed ("fan_on");
int fan_on=0; //Variable para encender o apagar el ventilador

/*-----------------------------------------------*/
/*FUNCIÓN PARA LEER EL FEED DE ADAFRUIT*/
void handleFanOn(AdafruitIO_Data *data){
  if (io.status()==AIO_CONNECTED){
    fan_on=atoi(data->value());
  }
  t_ultimo_paq = millis();
}
/*CONFIGURACIÓN*/
void setup() {
  heltec_setup();   //Inicialización de la placa
  I2CBME.begin(SDA_BME, SCL_BME); //Inicialización del sensor de temperatura
  display.println(bme.begin(0x76, &I2CBME) ? "BME OK" : "BME280 ERROR"); //Mostrar error por pantalla si da error
  pinMode(FAN_INA, OUTPUT);   // Inicialización del ventilador
  pinMode(FAN_INB, OUTPUT);
  int state = radio.begin(868.0); //Inicialización de la radio
  display.println(state == RADIOLIB_ERR_NONE ? "Radio works" : "Radio fail, code: " + String(state)); //Mostrar error si lo hubiera
  //Inicialización de los feeds de lectura de Adafruit
  io.connect();
  feed_fan->onMessage(handleFanOn);
  while(io.status() < AIO_CONNECTED) {
    delay(50);
  }
  display.println("Conectado a Adafruit IO");
}
void loop() {
  io.run();
  //Guardar el instante en el que arranca el bucle, para poder asegurar al final que cada ciclo durua exactamente t_ciclo segundos
  unsigned long t_inicio=millis();
  // 1. LEER SENSOR DE TEMPERATURA
  // Lee el sensor de temperatura
  float temperatura = bme.readTemperature();
  // 2. CONTROL DE ACTUADOR: VENTILADOR
  // Encender el ventilador si se supera el umbral de temperatura
  // o si se enciende desde IoT (fan_on se actualiza cuando llega mensaje)
  if ((REGTEMP_ON && temperatura>UMBRAL_TEMP )||fan_on == 1) {
    digitalWrite(FAN_INA, HIGH);
    digitalWrite(FAN_INB, LOW);
    Serial.println("Encender");
  } //Si no se supera el umbral, apagar el ventilador
  else {
    digitalWrite(FAN_INA, LOW);
    digitalWrite(FAN_INB, LOW);
  }

  // 3. ENVIAR A ADAFRUIT SI TOCA
  if (IOT_ON && millis() - t_ultimo_paq >= INTERVALO_ENVIO) {
    feed_temp->save(temperatura);
    t_ultimo_paq =millis();
  }
  // 4. IMPORTANTE: ESPERAR UN TIEMPO EXACTO HASTA LLEGAR A T_CICLO
  //Si usaramos directamente delay(t_ciclo) esperaría t_ciclo segundos sin importar cuánto haya tardado en hacer todo lo anterior. 
  //Al usar esta instrucción, aseguramos que todos los ciclos duran exactamente t_ciclo segundos, independientemente de lo que tarde cada vez en hacer todas las instrucciones.
  while ((millis()-t_inicio)<t_ciclo){
    //Se queda aquí esperando hasta que el tiempo transcurrido desde el principio del bucle hasta ahora alcance la duración exacta t_ciclo
    delay(1);
  }
}

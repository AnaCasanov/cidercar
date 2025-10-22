/*LIBRERÍAS*/
#include <heltec_unofficial.h>
#include <Adafruit_BME280.h>
#include <Preferences.h>
Preferences prefs;
/*DEFINICIÓN DE PINES*/
#define ACS_PIN 7
#define DIV_PIN 6
#define FAN_INA 19
#define FAN_INB 20
#define SDA_BME 5    
#define SCL_BME 4   
#define RESET_PIN 21
#define RESET_BATERIA 1
#define DISPLAY_ON 0

/* CONSTANTES GLOBALES*/
// Constantes del divisor de tensión, para medir la tensión actual en la batería
const float VREF = 3.3;
const float SENSIBILIDAD = 0.066;
const float VOLTAJE_REPOSO = 1.55;
const float R1 = 100000.0;
const float R2 = 10000.0;
// Constante que define cuándo arranca el ventilador y cuándo no
const float UMBRAL_TEMP = 30.0;
//Carga total de las baterías, aunque son dos de 36A·h, al estar en serie se suman las tensiones pero no la carga total. 2x12V 36Ah = 1x24V 36Ah
const float q_total = 36.0; 
//Tiempo que define cada cuanto se lee la tensión y corriente instantánea y determina el tiempo para calcular la carga restada de la total q_gastada=i_consumida·t_ciclo
const float t_ciclo=1000; //ms
const float n_muestras=700;  

/*VARIABLES GLOBALES*/
//Variables para utilizar el sensor de temperatura
TwoWire I2CBME = TwoWire(1);
Adafruit_BME280 bme;
//Variable para guardar la carga que le queda a la batería
float q_restante;
//Variable para saber si el ventilador está encendido o apagado
int fan_on = 0;
//Variable para controlar bien el tiempo que tarda cada ciclo
unsigned long t_anterior=0;

/* LEER CORRIENTE PROMEDIO*/
float leerCorriente() {
  float suma = 0;
  for (int i = 0; i < n_muestras; i++) {
    int lectura = analogRead(ACS_PIN);
    float v = lectura * VREF / 4095.0;
    suma += v;
    delay(1);  // pequeño retardo entre lecturas
  }
  float voltaje_medio = suma / n_muestras;
  float i_medida = (VOLTAJE_REPOSO - voltaje_medio) / SENSIBILIDAD;
  if (i_medida<0) i_medida=0;
  return i_medida;
}


/*CONFIGURACIÓN*/
void setup() {
  //Inicialización de la placa
  heltec_setup();
  // Inicialización de BME280
  I2CBME.begin(SDA_BME, SCL_BME); 
  if (!bme.begin(0x76, &I2CBME)) {
    display.println("BME280 ERROR");
  }
  else {
    display.println("BME OK");
  }
  // Inicialización del ventilador
  pinMode(FAN_INA, OUTPUT);
  pinMode(FAN_INB, OUTPUT);
  // Inicialización de la radio LoRa
  int state = radio.begin(868.0);
  if (state == RADIOLIB_ERR_NONE) {
    display.println("Radio works");
  }
  else {
    display.printf("Radio fail, code: %i\n", state);
  }
  //Leer cuánta carga queda de la batería, que se guarda permanentemente en la placa
  prefs.begin("bateria", false); //Accede al espacio "bateria" de la memoria de la placa, false indica que NO es readOnly, es decir, que se puede tanto leer como escribir en ese espacio de memoria
  if (RESET_BATERIA){
    q_restante=q_total;
  }
  else{
    q_restante = prefs.getFloat("q_restante", q_total); //Lee el valor "q_restante" de la memoria, si no lo encuentra, guarda q_total, es decir, 72A·h
  }
}

void loop() {
  //Guardar el instante en el que arranca el bucle, para poder asegurar al final que cada ciclo durua exactamente t_ciclo segundos
  unsigned long t_inicio=millis();

  /* 1. LECTURA DE SENSORES */
  // Lee el sensor de temperatura
  float temperatura = bme.readTemperature();

  //Lee el divisor de tensión
  int lectura_divisor = analogRead(DIV_PIN);
  float voltaje_divisor = lectura_divisor * VREF/4095.0;
  float v_bateria = voltaje_divisor * (R1 + R2) / R2;
  //Lee el sensor de corriente
  float i_consumida=leerCorriente();
  i_consumida=15;

  /*2. CONTROL DE ACTUADOR: VENTILADOR*/
  // Encender el ventilador si se supera el umbral de temperatura definido para el motor
  if (temperatura > UMBRAL_TEMP) {
    digitalWrite(FAN_INA, HIGH);
    digitalWrite(FAN_INB, LOW);
    fan_on = 1;
  } //Si no se supera el umbral, apagar el ventilador
  else {
    digitalWrite(FAN_INA, LOW);
    digitalWrite(FAN_INB, LOW);
    fan_on = 0;
  }

  /*3. CÁLCULOS INTERNOS*/    
  // Cálculo de carga consumida de la batería Q=I·t
  float q_consumida = i_consumida*t_ciclo/ 3600000.0; //Pasarlo de A·ms a A·h - 1000ms/1s * 60s/1min* 60min/1h =3600000
  // Calcular cuanta carga queda, leyendo la carga guardada
  q_restante = q_restante-q_consumida; //Habíamos leido q_restante al arrancar la placa, así que no tenemos que leerlo cada vez
  if (q_restante<0) q_restante=0; //Evita números negativos
  prefs.putFloat("q_restante", q_restante); //Sí tenemos que guardarlo de cada vez por si se apaga la placa, que quede guardado
  float perc_bateria = 100*q_restante/q_total;


  /*4. EMISIÓN DE DATOS POR RADIO*/
  // Formar y enviar mensaje
  String mensaje = String(temperatura, 2) + "," +
                   String(i_consumida, 2) + "," +
                   String(v_bateria, 2) + "," +
                   String(fan_on)+ "," +
                   String(q_restante);
  radio.transmit(mensaje);
  /*5. RESETEAR Q_RESTANTE SI CAMBIAMOS LA BATERÍA*/
  if (digitalRead(RESET_PIN)==LOW){ //RESET_PIN es una constante de la placa, que permite ver si se pulsa el botón Reset sin necesidad de definir el pin previamente
    q_restante=q_total; //La variable se guardará en memoria en la próxima iteración
  }
  /*MOSTRAR POR DISPLAY*/
  if(DISPLAY_ON){
    display.printf("Temp: %.2f C\n", temperatura);
    display.printf("V_bat: %.2f V\n", v_bateria);
    display.printf("I_cons: %.2f A\n", i_consumida);
    display.printf("fan_on: %d \n", fan_on);
    display.printf("q_restante: %.3f A·h\n", q_restante); 
    display.printf("%% batería: %.2f %%\n", perc_bateria);
  }
  /*6. IMPORTANTE: ESPERAR UN TIEMPO EXACTO HASTA LLEGAR A T_CICLO*/
  //Si usaramos directamente delay(t_ciclo) esperaría t_ciclo segundos sin importar cuánto haya tardado en hacer todo lo anterior. 
  //Al usar esta instrucción, aseguramos que todos los ciclos duran exactamente t_ciclo segundos, independientemente de lo que tarde cada vez en hacer todas las instrucciones.
  unsigned long t_ahora=millis();
  float diff = t_ahora-t_inicio;
  //display.printf("%.2f \n", diff);
  while ((t_ahora-t_inicio)<t_ciclo){
    //Se queda aquí esperando hasta que el tiempo transcurrido desde el principio del bucle hasta ahora alcance la duración exacta t_ciclo
    t_ahora=millis();
  }
  

}

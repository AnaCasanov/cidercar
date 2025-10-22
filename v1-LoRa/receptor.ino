#define HELTEC_POWER_BUTTON
#include <heltec_unofficial.h>
float temperatura;
float i_consumida;
float v_bateria;
float fan_on;
float p_consumida;
float q_restante;
//Funcion guardarMensaje a partir del mensaje recibido
void guardarMensaje(String mensaje){
    //Encontrar comas: función "indexOf"
    int coma1 = mensaje.indexOf(',');
    int coma2 = mensaje.indexOf(',', coma1 + 1);
    int coma3 = mensaje.indexOf(',', coma2 + 1);
    int coma4 = mensaje.indexOf(',', coma3 + 1);
    //Extraer subcadenas de texto: función "if" y "substring"
    if (coma1 > 0 && coma2 > 0 && coma3 > 0 && coma4>0) {
      String textoTemperatura = mensaje.substring(0, coma1);
      String textoIConsumida  = mensaje.substring(coma1 + 1, coma2);
      String textoVBateria    = mensaje.substring(coma2 + 1, coma3);
      String textoFanOn       = mensaje.substring(coma3 + 1, coma4);
      String textoQRestante      = mensaje.substring(coma4 + 1);      
      //Transformar texto a variables
      float temperatura  = textoTemperatura.toFloat();
      float i_consumida  = textoIConsumida.toFloat();
      float v_bateria    = textoVBateria.toFloat();
      float fan_on         = textoFanOn.toFloat();
      float p_consumida = i_consumida*v_bateria;
      float q_restante = textoQRestante.toFloat();
    }
}

//Función setup, funciones radio.begin() y Serial.begin()
void setup() {
  heltec_setup();
  Serial.begin(115200);
  int state = radio.begin(868.0);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println("Radio works");
  } else {
    Serial.printf("Radio fail, code: %i\n", state);
    while (true);
  }
}

//Función loop, funciones radio.receive y Serial.print()
void loop() {
  String mensaje;
  //Recibir mensaje en String
  int state = radio.receive(mensaje);
  if (state == RADIOLIB_ERR_NONE && mensaje.length() > 0) {
      //Lee el mensaje y lo guarda en las variables globales
      guardarMensaje();
	    //Envía todos los datos por puerto serie, separados por comas:
      Serial.print(temperatura, 2);
      Serial.print(",");
      Serial.print(i_consumida, 2);
      Serial.print(",");
      Serial.print(v_bateria, 2);
      Serial.print(",");
      Serial.print(fan_on);
      Serial.print(",");
      Serial.print(p_consumida, 2);
      Serial.print(",");
      Serial.println(q_restante, 3);
    }
  }
}


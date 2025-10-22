import processing.serial.*;

Serial puerto;
String[] datos = new String[5];

float[] historialTemp     = new float[200];
float[] historialCorr     = new float[200];
float[] historialVolt     = new float[200];
float[] historialPotencia = new float[200];
float[] historialFan = new float[200];
float[] historialCarga = new float[200];



void setup() {
  size(800, 600);
  printArray(Serial.list());
  puerto = new Serial(this, Serial.list()[5], 115200);
  puerto.bufferUntil('\n');
  //textFont(createFont("Arial", 16));
}

void serialEvent(Serial p) {
  String mensaje = puerto.readStringUntil('\n');
  if (mensaje != null) {
    mensaje = mensaje.trim();
    datos = split(mensaje, ',');

    if (datos.length == 6) {
      float nuevaTemp     = float(datos[0]);
      float nuevaCorr     = float(datos[1]);
      float nuevaVolt     = float(datos[2]);
      float nuevaPotencia = float(datos[3]);
      float nuevoFan = float(datos[4]);
      float nuevaCarga = float(datos[5]);

      // Desplaza y añade los nuevos valores
      desplazarEInsertar(historialTemp, nuevaTemp);
      desplazarEInsertar(historialCorr, nuevaCorr);
      desplazarEInsertar(historialVolt, nuevaVolt);
      desplazarEInsertar(historialPotencia, nuevaPotencia);
      desplazarEInsertar(historialFan, nuevoFan);
      desplazarEInsertar(historialCarga, nuevaCarga);
    }
  }
}

void draw() {
  background(240);
  fill(0);
  textSize(18);

  // Texto de valores recibidos
  if (datos.length == 6 && datos[0] != null && datos[1] != null && datos[2] != null && datos[3] != null && datos[4] != null) {
    text("Temp: " + datos[0] + " °C",   30, 30);
    text("Corriente: " + datos[1] + " A", 230, 30);
    text("Tensión: " + datos[2] + " V",   430, 30);
    text("Ventilador: " + (datos[3].equals("1") ? "ON" : "OFF"), 30, 60);
    text("Potencia: " + datos[4] + " W", 230, 60);
    text("Q Restante: " + datos[5] + "A·h", 430, 60);
    // Dibujar las gráficas
    //Fila 1
    drawGraph(historialTemp,     "Temperatura (°C)",  50, 80, 300, 150);
    drawGraph(historialCorr,     "Corriente (A)",     400, 80, 300, 150);
    //Fila 2
    drawGraph(historialVolt,     "Voltaje (V)",        50, 235, 300, 150);
    drawGraph(historialPotencia, "Potencia (W)",      400, 235, 300, 150);
    //Fila 3
    drawGraph(historialFan, "Ventilador (ON/OFF)", 50, 390, 300, 150);
    drawGraph(historialCarga, "Q restante (Ah)", 400, 390, 300, 150);
  }
  else {
    text("Esperando datos de LoRa...", 30, 30);
  }
  
}

void drawGraph(float[] data, String label, float xOffset, float yOffset, float w, float h) {
  fill(255);
  stroke(0);
  rect(xOffset, yOffset, w, h);

  fill(0);
  text(label, xOffset + 10, yOffset + 20);

  noFill();
  stroke(0, 180, 0);
  beginShape();
  for (int i = 0; i < data.length; i++) {
    float x = map(i, 0, data.length - 1, xOffset + 10, xOffset + w - 10);
    float y = map(data[i], 0, getMax(data), yOffset + h - 10, yOffset + 30);
    vertex(x, y);
  }
  endShape();
}

void desplazarEInsertar(float[] historial, float nuevoValor) {
  for (int i = 0; i < historial.length - 1; i++) {
    historial[i] = historial[i + 1];
  }
  historial[historial.length - 1] = nuevoValor;
}

float getMax(float[] array) {
  float maxVal = array[0];
  for (int i = 1; i < array.length; i++) {
    if (array[i] > maxVal) {
      maxVal = array[i];
    }
  }
  return maxVal == 0 ? 1 : maxVal;  // evita división entre 0
}

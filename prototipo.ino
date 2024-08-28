int coinPulse = 16;  // Pin para contar pulsos de la moneda
int contador = 0;  // Contador de pulsos
unsigned long tiempo_inicio = 0;  // Tiempo en que comenzó el conteo
unsigned long tiempo_actual;
const int TIEMPO_MAXIMO = 540;  // Tiempo máximo en milisegundos
bool debug = true;
void pulsoMoneda() {
  contador++;  // Incrementa el contador en cada pulso
  if (tiempo_inicio == 0) {
    tiempo_inicio = millis();  // Inicia el tiempo si es la primera vez
  }
}

void setup() {
  Serial.begin(115200);  // Inicializa la comunicación serial
  pinMode(coinPulse, INPUT_PULLUP);  // Configura el pin como entrada con resistencia pull-up
  attachInterrupt(digitalPinToInterrupt(coinPulse), pulsoMoneda, FALLING);  // Configura interrupción en el pin
}

void loop() {
  tiempo_actual = millis();
  // if (tiempo_inicio != 0) {
  //  if(contador == 5 && debug){
  //    // Imprime el tiempo transcurrido y el contador
  //   Serial.print("TIEMPO: ");
  //   Serial.print(tiempo_actual - tiempo_inicio);
  //   Serial.print(" CONTADOR: ");
  //   Serial.println(contador);
  //   debug = false;
  //  }
  // }

  // Si el tiempo máximo ha sido alcanzado
  if ((tiempo_inicio != 0) && (tiempo_actual - tiempo_inicio) >= TIEMPO_MAXIMO) {
    if(contador == 1){
      Serial.println("25 centavos");
    }
    if(contador == 2){
      Serial.println("100 centavos");
    }
    if(contador == 3){
      Serial.println("5 centavos");
    }
    if(contador == 4){
      Serial.println("10 centavos");
    }
    if(contador == 5){
      Serial.println("1 centavo");
    }

    debug = true;   
    contador = 0;  // Reinicia el contador
    tiempo_inicio = 0;  // Reinicia el tiempo
  }
}

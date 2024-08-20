// Define the input pin for the pulse signal
const int coinPulsePin = 16; // Change this pin number to the one you're using

// Variable to keep track of the coin pulse count
volatile int coinCount = 0;

// Interrupt service routine to handle pulse detection
void IRAM_ATTR handleCoinPulse() {
  coinCount++;
}

void setup() {
  // Start serial communication for debugging
  Serial.begin(115200);

  // Set the coinPulsePin as input
  pinMode(coinPulsePin, INPUT_PULLUP);

  // Attach the interrupt to the coinPulsePin
  attachInterrupt(digitalPinToInterrupt(coinPulsePin), handleCoinPulse, RISING);

  Serial.println("Coin acceptor initialized. Waiting for pulses...");
}

void loop() {
  // Print the coin count every second
  static unsigned long lastPrintTime = 0;
  unsigned long currentTime = millis();

  if (currentTime - lastPrintTime >= 1000) { // Print every second
    lastPrintTime = currentTime;
    Serial.print("Coin Count: ");
    Serial.println(coinCount);
    coinCount = 0; // Reset the count after printing
  }

  // Other code can go here
}

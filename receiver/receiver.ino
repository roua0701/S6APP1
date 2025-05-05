
#define TX 13
#define RX 14

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, RX, TX);
  
  Serial.println("Station de base");
}

void loop() {
  if (Serial1.available()) {
    String message = Serial1.readStringUntil('\n');
    Serial.println("Message reçu: " + message);
  }
}

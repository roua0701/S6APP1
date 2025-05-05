#define TX 14
#define RX 13

void setup() {
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, RX, TX);

  Serial.println("Station de capteurs");
}

void loop() {
  Serial1.println("Yo nigga haha");
  Serial.println("Données transmises");  
  
  delay(1000); 
}

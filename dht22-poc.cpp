void setup() {
  pinMode(13, OUTPUT);
  pinMode(4, INPUT_PULLUP);
  Serial.begin(9600);
}

struct temp_hum {
  float temperature;
  float humidity;
};

// Whyyyy.... https://cdn-shop.adafruit.com/datasheets/DHT22.pdf

void loop() {
  digitalWrite(13, HIGH);
  delay(10);

  struct temp_hum th;
  bool success = read_data(&th);
  if (success == true) {
    Serial.print(" Temperature: ");
    Serial.print(th.temperature);
    Serial.print(" Humidity: ");
    Serial.print(th.humidity);
    Serial.print("\n");
  } else {
    Serial.print("Checksum did not match or timed out whilst reading.\n");
  }
  
  digitalWrite(13, LOW);
  delay(2000);
}

struct bit_time {
  unsigned char low;
  unsigned char high;
};

unsigned char ticksSpentAt(bool level) {
  unsigned char ticks = 0;
  while (digitalRead(4) == level) { if (ticks++ > 100) return 0; }
  return ticks;
}

void read_bit(struct bit_time *bitt) {
  bitt->low = ticksSpentAt(LOW);
  bitt->high = ticksSpentAt(HIGH);
}

bool read_data(struct temp_hum *out) {
  struct bit_time bits[40];

  digitalWrite(4, HIGH);
  delay(10);
  
  // Send start pulse
  pinMode(4, OUTPUT);
  digitalWrite(4, LOW);
  delay(10);
  
  noInterrupts();
  digitalWrite(4, HIGH);
  delayMicroseconds(40);
  pinMode(4, INPUT_PULLUP);
  delayMicroseconds(10);

  // Wait for start pulse in response
  unsigned char start_delay_low = ticksSpentAt(LOW);
  unsigned char start_delay_high = ticksSpentAt(HIGH);

  for (int i = 0; i < 40; i++) {
    read_bit(bits + i);
  }

  interrupts();
  
  unsigned char bytes[5];
  for (int i = 0; i < 40; i++) {
    if (bits[i].high == 0 || bits[i].low == 0) return false;
    bytes[i>>3] = (bytes[i>>3] << 1) + ((bits[i].high > bits[i].low) ? 1 : 0);
  }
  unsigned char checksum = (bytes[0] + bytes[1] + bytes[2] + bytes[3]) & 0xff;
//  Serial.print(bytes[0], BIN); Serial.print(" ");
//  Serial.print(bytes[1], BIN); Serial.print(" ");
//  Serial.print(bytes[2], BIN); Serial.print(" ");
//  Serial.print(bytes[3], BIN); Serial.print(" ");
//  Serial.print(bytes[4], BIN); Serial.print(" ");
//  Serial.print(checksum, BIN); Serial.print(" ");
//  Serial.print("\n");

  if (checksum != bytes[4]) return false;

  out->humidity = ((bytes[0] * 256.0) + bytes[1]) / 10.0;
  out->temperature = (((bytes[2]&0x7f) * 256.0) + bytes[3]) / 10.0;
  if (bytes[2] & 0x80 != 0) out->temperature *= -1.0;
  
  return true;
}

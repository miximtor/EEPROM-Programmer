#define ADDR_SER 2
#define ADDR_CLK 3
#define ADDR_RCLK 4
#define IO_D0 5
#define IO_D7 12
#define WRITE_ENABLE 13
#define EEPROM_SIZE 2048

void pollDelay(int latency)
{
  unsigned long now = millis();
  while (now + latency > millis())
  {}
}

byte readEEPROM(unsigned int address)
{
  setAddress(address, true);
  
  byte data = 0;
  for (int pin = IO_D7; pin >= IO_D0; pin -= 1)
  {
    data = (data << 1) + digitalRead(pin);
  }
  return data;
}

void writeEEPROM(unsigned int address, byte data)
{
  setAddress(address,  false);
  
  for (int pin = IO_D0; pin <= IO_D7; pin += 1)
  {
    digitalWrite(pin, data & 0x1);
    data = data >> 1;
  }

  digitalWrite(WRITE_ENABLE, LOW);
  pollDelay(1);
  digitalWrite(WRITE_ENABLE, HIGH);
  pollDelay(10);
}

void writeSignleByte(unsigned int address, byte data)
{
  for (int pin = IO_D0; pin <= IO_D7; pin += 1)
  {
    digitalWrite(pin, HIGH);
    pinMode(pin, OUTPUT);
  }

  writeEEPROM(address, data);
}

void dumpEEPROM()
{
  for (int pin = IO_D0; pin <= IO_D7; pin += 1)
  {
    pinMode(pin, INPUT);
  }

  for (unsigned int address = 0; address < EEPROM_SIZE; address += 1)
  {
    byte data = readEEPROM(address);
    Serial.write(data);
  }
}

void eraseEEPROM(byte fill)
{
  for (int pin = IO_D0; pin <= IO_D7; pin += 1)
  {
    digitalWrite(pin, HIGH);
    pinMode(pin, OUTPUT);
  }

  for (unsigned int address = 0; address < EEPROM_SIZE; address += 1)
  {
    writeEEPROM(address, fill);
  }
}

void setAddress(unsigned int address, bool enable_output)
{
  address = address & 0x7FF;
  if (!enable_output)
  {
    address = address | 0x8000;
  }

  shiftOut(ADDR_SER, ADDR_CLK, MSBFIRST, address >> 8);
  shiftOut(ADDR_SER, ADDR_CLK, MSBFIRST, address);

  digitalWrite(ADDR_RCLK, LOW);
  digitalWrite(ADDR_RCLK, HIGH);
  digitalWrite(ADDR_RCLK, LOW);
}

// the setup function runs once when you press reset or power the board
void setup() {
  pinMode(ADDR_SER, OUTPUT);
  pinMode(ADDR_CLK, OUTPUT);
  pinMode(ADDR_RCLK, OUTPUT);

  digitalWrite(WRITE_ENABLE, HIGH);
  pinMode(WRITE_ENABLE, OUTPUT);

  Serial.begin(57600);
  Serial.println("ACK");
  Serial.println(EEPROM_SIZE);
  
  byte command[4];
  while (Serial.available() < 4)
  {}
  Serial.readBytes(command, 4);

  if (command[0] == 0x01)
  {
    dumpEEPROM();
    Serial.println("ACK");
  }
  else if (command[0] == 0x02)
  {
    eraseEEPROM(command[1]);
    pollDelay(1000);
    Serial.println("ACK");
  }
  else if (command[0] == 0x03)
  {
    unsigned int addr_low = command[1];
    unsigned int addr_high = command[2];
    unsigned int address = (addr_high << 8) | addr_low;
    
    writeSignleByte(address, command[3]);
    Serial.println("ACK");
  }
  
}

void loop() {
}

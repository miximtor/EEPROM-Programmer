#define ADDR_SER 2
#define ADDR_CLK 3
#define ADDR_RCLK 4
#define IO_D0 5
#define IO_D7 12
#define WRITE_ENABLE 13
#define EEPROM_SIZE 2048

void pollDelay(int latency)
{
  unsigned long start = millis();
  while (start + latency > millis())
  {}
}

void setupRead()
{
  for (int pin = IO_D0; pin <= IO_D7; ++pin )
  {
    pinMode(pin, INPUT);
  }

  digitalWrite(ADDR_SER, LOW);
  pinMode(ADDR_SER, OUTPUT);

  digitalWrite(ADDR_CLK, LOW);
  pinMode(ADDR_CLK, OUTPUT);

  digitalWrite(ADDR_RCLK, LOW);
  pinMode(ADDR_RCLK, OUTPUT);

  digitalWrite(WRITE_ENABLE, HIGH);
  pinMode(WRITE_ENABLE, OUTPUT);
}

void setupWrite()
{
  for (int pin = IO_D0; pin <= IO_D7; ++pin )
  {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }

  digitalWrite(ADDR_SER, LOW);
  pinMode(ADDR_SER, OUTPUT);

  digitalWrite(ADDR_CLK, LOW);
  pinMode(ADDR_CLK, OUTPUT);

  digitalWrite(ADDR_RCLK, LOW);
  pinMode(ADDR_RCLK, OUTPUT);

  digitalWrite(WRITE_ENABLE, HIGH);
  pinMode(WRITE_ENABLE, OUTPUT);
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

  digitalWrite(ADDR_RCLK, HIGH);
  digitalWrite(ADDR_RCLK, LOW);
}

byte readEEPROM(uint16_t address)
{
  setAddress(address, true);
  
  byte data = 0;
  for (int pin = IO_D7; pin >= IO_D0; pin -= 1)
  {
    data = (data << 1) + digitalRead(pin);
  }
  return data;
}

void writeEEPROM(uint16_t address, byte data)
{
  digitalWrite(WRITE_ENABLE, HIGH);
  setAddress(address,  false);
  
  for (int pin = IO_D0; pin <= IO_D7; pin += 1)
  {
    digitalWrite(pin, data & 0x1);
    data = data >> 1;
  }

  digitalWrite(WRITE_ENABLE, LOW);
  digitalWrite(WRITE_ENABLE, HIGH);
  pollDelay(10);
}

void writeSignleByte(unsigned int address, byte data)
{
  writeEEPROM(address, data);
}

void dumpEEPROM()
{


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



// the setup function runs once when you press reset or power the board
void setup() {



  Serial.begin(57600);
  Serial.println("ACK");

  Serial.println(EEPROM_SIZE);
  
  byte command[4];
  while (Serial.available() < 4)
  {}
  Serial.readBytes(command, 4);

  switch (command[0])
  {
  case 0x01:
    dumpEEPROM();
    break;
  case 0x02:
    eraseEEPROM(command[1]);
    break; 
  case 0x03:
    uint16_t address = (uint16_t(command[1]) << 8) | uint16_t(command[2]);
    writeSignleByte(address, command[3]);
    break;
  case 0x04:
    uint16_t file_size = (uint16_t(command[1]) << 8) | uint16_t(command[2]);
    break;
  default:
    break;
  }
  Serial.println("ACK");
  Serial.end();
}

void loop() {
}

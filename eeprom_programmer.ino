#define ADDR_SER 2
#define ADDR_CLK 3
#define ADDR_RCLK 4
#define IO_D0 5
#define IO_D7 12
#define WRITE_ENABLE 13
#define EEPROM_SIZE 2048
#define BAUD_RATE 56400

enum EEPROM_CTRL
{
  CMD_DUMP = 0x01,
  CMD_ERASE,
  CMD_WRITE_BYTE,
  CMD_WRITE_FILE
};

void pollSerialRead(uint8_t *buffer, int size)
{
  int pos = 0;
  while (true)
  {
    pos += Serial.readBytes(buffer + pos, size - pos);
    if (pos >= size)
    {
      break;
    }
  }
}

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

void triggerWE()
{
  digitalWrite(WRITE_ENABLE, LOW);
  pollDelay(1);
  digitalWrite(WRITE_ENABLE, HIGH);
}

void sendACK()
{
  Serial.println("ACK");
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
  pollDelay(1);
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

  triggerWE();
}

void writeSignleByte(unsigned int address, byte data)
{
  writeEEPROM(address, data);
}

void dumpEEPROM()
{
  uint8_t data[EEPROM_SIZE];
  for (uint16_t address = 0; address < EEPROM_SIZE; ++address)
  {
    data[address] = readEEPROM(address);
  }

  Serial.write(data, EEPROM_SIZE);
}

void eraseEEPROM(byte fill)
{
  for (uint16_t address = 0; address < EEPROM_SIZE; ++address)
  {
    writeEEPROM(address, fill);
  }
}

// the setup function runs once when you press reset or power the board
void setup() {
  Serial.begin(BAUD_RATE);
  while (!Serial)
  {
    // polling wait
  }

  Serial.println(EEPROM_SIZE);
  sendACK();

  byte command[4];
  pollSerialRead(command, 4);
  sendACK();

  if (command[0] == CMD_DUMP)
  {
    setupRead();
    dumpEEPROM();
  }
  else if (command[0] == CMD_ERASE)
  {
    setupWrite();
    eraseEEPROM(command[1]);
  }
  else if (command[0] == CMD_WRITE_BYTE)
  {
    setupWrite();
    uint16_t address = (uint16_t(command[1]) << 8) | uint16_t(command[2]);
    writeSignleByte(address, command[3]);
  }
  else if (command[0] == CMD_WRITE_FILE)
  {
    setupWrite();
    uint16_t file_size = (uint16_t(command[1]) << 8) | uint16_t(command[2]);
  }
  else
  {
    Serial.println("NACK");
  }

  Serial.println("ACK");
  Serial.end();
}

void loop() {
}

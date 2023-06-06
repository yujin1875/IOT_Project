#include <wiringPiI2C.h>       // I2C ��� ���̺귯��
#include <wiringPi.h>          // WiringPi ���̺귯��
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <pcf8591.h>           // ADC ��� ���̺귯��
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define HOST "172.20.48.142"   // ���� IP �ּ�
#define PORT 5764              // ���� ��Ʈ ��ȣ
#define BUFFER_SIZE 1024       // ���� ��ſ� ���� ũ��

#define I2C_ADDR   0x27        // LCD I2C �ּ�

#define LCD_CHR  1             // LCD ������ ���� ���
#define LCD_CMD  0             // LCD ��� ���� ���

#define LINE1  0x80            // LCD 1��
#define LINE2  0xC0            // LCD 2��

#define LCD_BACKLIGHT   0x08   // LCD �����Ʈ ����

#define ENABLE  0b00000100     // LCD ENABLE ��Ʈ

#define PCF 120                // ADC ��� �ε���
#define LED_PIN 0              // LED ���� GPIO �� ��ȣ
#define SENSOR_PIN 23          // ���� ���� GPIO �� ��ȣ
#define LED_PIN_gas 29         // ���� ���� LED ���� GPIO �� ��ȣ

#define MAXTIMINGS    85       // DHT11 ���� ���� ��ũ��
#define DHTPIN        7

unsigned char soundDetected = 0; // �Ҹ� ���� ���� ����

int dht11_dat[5] = { 0, 0, 0, 0, 0 }; // DHT11 ���� ������ �迭
int lcd_fd; // LCD ���� ��ũ����

float prev_temperature = 0.0; // ���� �µ� ��
int prev_humidity = 0; // ���� ���� ��

#define PIR_PIN 2   // PIR ���� GPIO �� ��ȣ
#define SOUND_IN 6  // �Ҹ� ���� GPIO �� ��ȣ

void lcd_init(void);  // LCD �ʱ�ȭ �Լ�
void lcd_byte(int bits, int mode);  // LCD�� ������ ��Ʈ�� ��� ���� �Լ�
void lcd_toggle_enable(int bits);  // LCD�� Enable �� ��� �Լ�
void lcdLoc(int line);   // LCD�� Ŀ�� ��ġ ���� �Լ�
void ClrLcd(void); // ȭ�� ����� �Լ�
void typeln(const char* s);  // ���ڿ� ��� �Լ�
void typeChar(char val);  // ���� ���� ��� �Լ�
void read_dht11_dat(void);  // DHT11 �������� ������ �д� �Լ�

void soundInterrupt(void) {  // �Ҹ� ���ͷ�Ʈ ó�� �Լ�
    soundDetected = 1; // �Ҹ� ���� ���¸� 1�� ����
}

void typeFloat(float myFloat)  // �ε� �Ҽ��� ���� ��� �Լ�
{
    char buffer[20];
    sprintf(buffer, "%4.2f", myFloat);
    // ������ �Ҽ��� 2�ڸ����� ������ �������� ���ڿ��� ��ȯ
    typeln(buffer);  // ��ȯ�� ���ڿ��� typeln �Լ��� ����Ͽ� ���
}

void typeInt(int I) // ���� ��� �Լ�
{
    char array1[20];
    sprintf(array1, "%d", i);  // I ������ ���ڿ��� ��ȯ
    typeln(array1);  // ��ȯ�� ���ڿ��� typeln �Լ��� ����Ͽ� ���
}

void setup(void)
{
    if (wiringPiSetup() == -1) // WiringPi �ʱ�ȭ
        exit(1);
    lcd_fd = wiringPiI2CSetup(I2C_ADDR); // I2C �ּ� ����
    lcd_init(); // LCD �ʱ�ȭ
}

int main(void)
{
    int value;
    int off = 1;
    wiringPiSetup(); // WiringPi �ʱ�ȭ
    pcf8591Setup(PCF, 0x48); // ADC ��� ����
    pinMode(LED_PIN, OUTPUT); // LED ���� ��� ���� ����
    pinMode(PIR_PIN, INPUT); // PIR ���� ���� �Է� ���� ����
    pinMode(SENSOR_PIN, INPUT); // ���� ���� ���� �Է� ���� ����
    pinMode(LED_PIN_gas, OUTPUT); // ���� ���� LED ���� ��� ���� ����
    wiringPiISR(SOUND_IN, INT_EDGE_RISING, &soundInterrupt); // �Ҹ� ���� ���ͷ�Ʈ ����
    printf("Raspberry Pi wiringPi DHT11 Temperature and Humidity LCD Display\n");

    setup(); // �ʱ� ����

    // ���� ����
    int server_socket, client_socket; // ���� ���� �ĺ���, Ŭ���̾�Ʈ ���� �ĺ���
    // ���� �ּ� ����ü, Ŭ���̾�Ʈ �ּ� ����ü
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_length;  // Ŭ���̾�Ʈ �ּ� ����ü�� ����
    char buffer[BUFFER_SIZE];  // �����͸� ������ ����

    // IPv4 �ּ� ü�� ����ϴ� TCP ���� ����
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    // ���� ������ ������ ��� ���� �޽����� ����ϰ� ���α׷��� ����
    if (server_socket == -1) {
        perror("Error");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;  // IPv4 �ּ� ü��
    server_address.sin_port = htons(PORT);  // ��Ʈ ��ȣ�� ��Ʈ��ũ ����Ʈ ������ ��ȯ
    // ȣ��Ʈ �ּҸ� ��Ʈ��ũ ����Ʈ ������ ��ȯ
    server_address.sin_addr.s_addr = inet_addr(HOST);

    // ���� ���ϰ� �ּҸ� ���ε�
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        // ���ε��� ������ ��� ���� �޽����� ����ϰ� ���α׷��� ����
        perror("Error");
        exit(EXIT_FAILURE);
    }
    // Ŭ���̾�Ʈ�� ���� ��û�� ������ �� �ֵ��� ���� ������ ���� ��� ���·� ����
    if (listen(server_socket, 1) == -1) {
        // ������ ������ ��� ���� �޽����� ����ϰ� ���α׷��� ����
        perror("Error");
        exit(EXIT_FAILURE);
    }
    printf("connection start\n");  // ������ ���� ��û�� ��ٸ��� ������ ���

    // ���� ��û�� �����ϰ�, Ŭ���̾�Ʈ�� ����� ������ ����
    client_address_length = sizeof(client_address);
    client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_length); // Ŭ���̾�Ʈ ���� ���
    if (client_socket == -1) {
        perror("Fail");
        exit(EXIT_FAILURE);
    }
    printf("connect!\n");  // Ŭ���̾�Ʈ���� ����

    ssize_t sent_bytes;

    while (1)
    {
        read_dht11_dat(); // DHT11 ���� ������ �б�

        if ((dht11_dat[0] != 0) && (dht11_dat[2] != 0))
        {
            // ���ο� �µ��� ���� ���� LCD�� ���
            lcdLoc(LINE1);
            typeln("Temperature:"); // �µ� ���
            lcdLoc(LINE2);
            typeInt(dht11_dat[2]);
            typeln("C");

            delay(2000);
            ClrLcd();

            lcdLoc(LINE1);
            typeln("Humidity:");
            lcdLoc(LINE2);
            typeInt(dht11_dat[0]);
            typeln("%");

            // ���� ���� ����
            prev_temperature = dht11_dat[2];
            prev_humidity = dht11_dat[0];
        }
        else
        {
            lcdLoc(LINE1);
            typeln("Temperature:");
            lcdLoc(LINE2);
            typeInt(prev_temperature);
            typeln("C");

            delay(2000);
            ClrLcd();// LCD ȭ�� �����

            lcdLoc(LINE1);
            typeln("Humidity:"); // ���� ���
            lcdLoc(LINE2);
            typeInt(prev_humidity);
            typeln("%");
        }

        if (digitalRead(SENSOR_PIN) == HIGH) { // ���� ���� ������ HIGH�� ���
            digitalWrite(LED_PIN_gas, HIGH); // ���� ���� LED �ѱ�
            printf("on\n");
            sent_bytes = send(client_socket, "fire\n", strlen("fire\n"), 0); // ������ "fire" ���ڿ� ����
            if (sent_bytes == -1) {
                perror("fail");
                exit(EXIT_FAILURE);
            }
        }
        else {
            digitalWrite(LED_PIN_gas, LOW); // ���� ���� LED ����
            printf("off\n");
        }
        delay(1000);

        value = analogRead(PCF + 0); // ADC ��⿡�� �� �б�

        if (value < 150)
        {
            printf("LED off\n");
            digitalWrite(LED_PIN, LOW); // LED ����
        }
        else if (value >= 150)
        {
            printf("LED on\n");
            digitalWrite(LED_PIN, HIGH); // LED �ѱ�
        }
        analogWrite(PCF + 0, value); // ���� �� ���

        delay(2000);

        char* msg; // ������ ���� �޽��� ����

        if (digitalRead(PIR_PIN) == HIGH && soundDetected == 1) {
            printf("Motion and Sound detected!\n");
            msg = "ms\n";
            sent_bytes = send(client_socket, msg, strlen(msg), 0); // ������ �޽��� ����
        }
        else if (digitalRead(PIR_PIN) == HIGH) {
            printf("Motion detected!\n");
            msg = "m\n";
            sent_bytes = send(client_socket, msg, strlen(msg), 0);
        }
        else if (soundDetected == 1) {
            printf("Sound detected!\n");
            msg = "s\n";
            sent_bytes = send(client_socket, msg, strlen(msg), 0);
        }
        else {
            printf("Nothing\n");
        }

        soundDetected = 0; // �Ҹ� ���� ���� �ʱ�ȭ

        delay(2000);
        ClrLcd(); // LCD ȭ�� �����
    }

    close(client_socket);
    close(server_socket);

    return 0;
}
// LCD ����Ʈ ���� �Լ�
void lcd_byte(int bits, int mode)
{
    int bits_high;
    int bits_low;
    bits_high = mode | (bits & 0xF0) | LCD_BACKLIGHT;
    bits_low = mode | ((bits << 4) & 0xF0) | LCD_BACKLIGHT;
    wiringPiI2CReadReg8(lcd_fd, bits_high);
    lcd_toggle_enable(bits_high);

    wiringPiI2CReadReg8(lcd_fd, bits_low);
    lcd_toggle_enable(bits_low);
}
// LCD Enable ��� �Լ�
void lcd_toggle_enable(int bits)
{
    delayMicroseconds(500);
    wiringPiI2CReadReg8(lcd_fd, (bits | ENABLE));
    delayMicroseconds(500);
    wiringPiI2CReadReg8(lcd_fd, (bits & ~ENABLE));
    delayMicroseconds(500);
}
// LCD �ʱ�ȭ �Լ�
void lcd_init(void)
{
    lcd_byte(0x33, LCD_CMD);
    lcd_byte(0x32, LCD_CMD);
    lcd_byte(0x06, LCD_CMD);
    lcd_byte(0x0C, LCD_CMD);
    lcd_byte(0x28, LCD_CMD);
    lcd_byte(0x01, LCD_CMD);
    delayMicroseconds(500);
}
// LCD Ŀ�� ��ġ ���� �Լ�
void lcdLoc(int line)
{
    lcd_byte(line, LCD_CMD);
}
// LCD ȭ�� ����� �Լ�
void ClrLcd(void)
{
    lcd_byte(0x01, LCD_CMD);
    lcd_byte(0x02, LCD_CMD);
}
// ���ڿ� ��� �Լ�
void typeln(const char* s)
{
    while (*s)
        lcd_byte(*(s++), LCD_CHR);
}
// ���� ��� �Լ�
void typeChar(char val)
{
    lcd_byte(val, LCD_CHR);
}
// DHT11 ���� ������ �б� �Լ�
void read_dht11_dat(void)
{
    uint8_t laststate = HIGH;
    uint8_t counter = 0;
    uint8_t j = 0, i;
    float f;
    dht11_dat[0] = dht11_dat[1] = dht11_dat[2] = dht11_dat[3] = dht11_dat[4] = 0;

    pinMode(DHTPIN, OUTPUT);
    digitalWrite(DHTPIN, LOW);
    delay(18);
    digitalWrite(DHTPIN, HIGH);
    delayMicroseconds(40);
    pinMode(DHTPIN, INPUT);

    for (i = 0; i < MAXTIMINGS; i++)
    {
        counter = 0;
        while (digitalRead(DHTPIN) == laststate)
        {
            counter++;
            delayMicroseconds(1);
            if (counter == 255)
            {
                break;
            }
        }
        laststate = digitalRead(DHTPIN);

        if (counter == 255)
            break;

        if ((i >= 4) && (i % 2 == 0))
        {
            dht11_dat[j / 8] <<= 1;
            if (counter > 16)
                dht11_dat[j / 8] |= 1;
            j++;
        }
    }
    //LCD���� �ֿܼ� ����ϴ� �κ�
    if ((j >= 40) && (dht11_dat[4] == ((dht11_dat[0] + dht11_dat[1] + dht11_dat[2] + dht11_dat[3]) & 0xFF)))
    {
        f = dht11_dat[2] * 9. / 5. + 32;
        printf("Humidity = %d.%d %% Temperature = %d.%d C (%.1f F)\n",
            dht11_dat[0], dht11_dat[1], dht11_dat[2], dht11_dat[3], f);
    }
    else
    {
        printf("Data not good, skip\n");
    }
}

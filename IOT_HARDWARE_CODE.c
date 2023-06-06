#include <wiringPiI2C.h>       // I2C 통신 라이브러리
#include <wiringPi.h>          // WiringPi 라이브러리
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <pcf8591.h>           // ADC 모듈 라이브러리
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define HOST "172.20.48.142"   // 서버 IP 주소
#define PORT 5764              // 서버 포트 번호
#define BUFFER_SIZE 1024       // 소켓 통신용 버퍼 크기

#define I2C_ADDR   0x27        // LCD I2C 주소

#define LCD_CHR  1             // LCD 데이터 전송 모드
#define LCD_CMD  0             // LCD 명령 전송 모드

#define LINE1  0x80            // LCD 1행
#define LINE2  0xC0            // LCD 2행

#define LCD_BACKLIGHT   0x08   // LCD 백라이트 상태

#define ENABLE  0b00000100     // LCD ENABLE 비트

#define PCF 120                // ADC 모듈 인덱스
#define LED_PIN 0              // LED 제어 GPIO 핀 번호
#define SENSOR_PIN 23          // 가스 센서 GPIO 핀 번호
#define LED_PIN_gas 29         // 가스 감지 LED 제어 GPIO 핀 번호

#define MAXTIMINGS    85       // DHT11 센서 관련 매크로
#define DHTPIN        7

unsigned char soundDetected = 0; // 소리 감지 상태 변수

int dht11_dat[5] = { 0, 0, 0, 0, 0 }; // DHT11 센서 데이터 배열
int lcd_fd; // LCD 파일 디스크립터

float prev_temperature = 0.0; // 이전 온도 값
int prev_humidity = 0; // 이전 습도 값

#define PIR_PIN 2   // PIR 센서 GPIO 핀 번호
#define SOUND_IN 6  // 소리 센서 GPIO 핀 번호

void lcd_init(void);  // LCD 초기화 함수
void lcd_byte(int bits, int mode);  // LCD에 데이터 비트와 모드 전송 함수
void lcd_toggle_enable(int bits);  // LCD의 Enable 핀 토글 함수
void lcdLoc(int line);   // LCD에 커서 위치 설정 함수
void ClrLcd(void); // 화면 지우기 함수
void typeln(const char* s);  // 문자열 출력 함수
void typeChar(char val);  // 단일 문자 출력 함수
void read_dht11_dat(void);  // DHT11 센서에서 데이터 읽는 함수

void soundInterrupt(void) {  // 소리 인터럽트 처리 함수
    soundDetected = 1; // 소리 감지 상태를 1로 설정
}

void typeFloat(float myFloat)  // 부동 소수점 숫자 출력 함수
{
    char buffer[20];
    sprintf(buffer, "%4.2f", myFloat);
    // 변수를 소수점 2자리까지 포함한 형식으로 문자열로 변환
    typeln(buffer);  // 변환된 문자열을 typeln 함수를 사용하여 출력
}

void typeInt(int I) // 정수 출력 함수
{
    char array1[20];
    sprintf(array1, "%d", i);  // I 변수를 문자열로 변환
    typeln(array1);  // 변환된 문자열을 typeln 함수를 사용하여 출력
}

void setup(void)
{
    if (wiringPiSetup() == -1) // WiringPi 초기화
        exit(1);
    lcd_fd = wiringPiI2CSetup(I2C_ADDR); // I2C 주소 설정
    lcd_init(); // LCD 초기화
}

int main(void)
{
    int value;
    int off = 1;
    wiringPiSetup(); // WiringPi 초기화
    pcf8591Setup(PCF, 0x48); // ADC 모듈 설정
    pinMode(LED_PIN, OUTPUT); // LED 핀을 출력 모드로 설정
    pinMode(PIR_PIN, INPUT); // PIR 센서 핀을 입력 모드로 설정
    pinMode(SENSOR_PIN, INPUT); // 가스 센서 핀을 입력 모드로 설정
    pinMode(LED_PIN_gas, OUTPUT); // 가스 감지 LED 핀을 출력 모드로 설정
    wiringPiISR(SOUND_IN, INT_EDGE_RISING, &soundInterrupt); // 소리 센서 인터럽트 설정
    printf("Raspberry Pi wiringPi DHT11 Temperature and Humidity LCD Display\n");

    setup(); // 초기 설정

    // 소켓 설정
    int server_socket, client_socket; // 서버 소켓 식별자, 클라이언트 소켓 식별자
    // 서버 주소 구조체, 클라이언트 주소 구조체
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_length;  // 클라이언트 주소 구조체의 길이
    char buffer[BUFFER_SIZE];  // 데이터를 저장할 버퍼

    // IPv4 주소 체계 사용하는 TCP 소켓 생성
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    // 소켓 생성에 실패한 경우 오류 메시지를 출력하고 프로그램을 종료
    if (server_socket == -1) {
        perror("Error");
        exit(EXIT_FAILURE);
    }

    server_address.sin_family = AF_INET;  // IPv4 주소 체계
    server_address.sin_port = htons(PORT);  // 포트 번호를 네트워크 바이트 순서로 변환
    // 호스트 주소를 네트워크 바이트 순서로 변환
    server_address.sin_addr.s_addr = inet_addr(HOST);

    // 서버 소켓과 주소를 바인딩
    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        // 바인딩에 실패한 경우 오류 메시지를 출력하고 프로그램을 종료
        perror("Error");
        exit(EXIT_FAILURE);
    }
    // 클라이언트의 연결 요청을 수신할 수 있도록 서버 소켓을 수동 대기 상태로 설정
    if (listen(server_socket, 1) == -1) {
        // 설정에 실패한 경우 오류 메시지를 출력하고 프로그램을 종료
        perror("Error");
        exit(EXIT_FAILURE);
    }
    printf("connection start\n");  // 서버가 연결 요청을 기다리는 중임을 출력

    // 연결 요청을 수락하고, 클라이언트와 통신할 소켓을 생성
    client_address_length = sizeof(client_address);
    client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_length); // 클라이언트 연결 대기
    if (client_socket == -1) {
        perror("Fail");
        exit(EXIT_FAILURE);
    }
    printf("connect!\n");  // 클라이언트와의 연결

    ssize_t sent_bytes;

    while (1)
    {
        read_dht11_dat(); // DHT11 센서 데이터 읽기

        if ((dht11_dat[0] != 0) && (dht11_dat[2] != 0))
        {
            // 새로운 온도와 습도 값을 LCD에 출력
            lcdLoc(LINE1);
            typeln("Temperature:"); // 온도 출력
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

            // 이전 값을 갱신
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
            ClrLcd();// LCD 화면 지우기

            lcdLoc(LINE1);
            typeln("Humidity:"); // 습도 출력
            lcdLoc(LINE2);
            typeInt(prev_humidity);
            typeln("%");
        }

        if (digitalRead(SENSOR_PIN) == HIGH) { // 가스 감지 센서가 HIGH인 경우
            digitalWrite(LED_PIN_gas, HIGH); // 가스 감지 LED 켜기
            printf("on\n");
            sent_bytes = send(client_socket, "fire\n", strlen("fire\n"), 0); // 서버에 "fire" 문자열 전송
            if (sent_bytes == -1) {
                perror("fail");
                exit(EXIT_FAILURE);
            }
        }
        else {
            digitalWrite(LED_PIN_gas, LOW); // 가스 감지 LED 끄기
            printf("off\n");
        }
        delay(1000);

        value = analogRead(PCF + 0); // ADC 모듈에서 값 읽기

        if (value < 150)
        {
            printf("LED off\n");
            digitalWrite(LED_PIN, LOW); // LED 끄기
        }
        else if (value >= 150)
        {
            printf("LED on\n");
            digitalWrite(LED_PIN, HIGH); // LED 켜기
        }
        analogWrite(PCF + 0, value); // 읽은 값 출력

        delay(2000);

        char* msg; // 서버에 보낼 메시지 변수

        if (digitalRead(PIR_PIN) == HIGH && soundDetected == 1) {
            printf("Motion and Sound detected!\n");
            msg = "ms\n";
            sent_bytes = send(client_socket, msg, strlen(msg), 0); // 서버에 메시지 전송
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

        soundDetected = 0; // 소리 감지 상태 초기화

        delay(2000);
        ClrLcd(); // LCD 화면 지우기
    }

    close(client_socket);
    close(server_socket);

    return 0;
}
// LCD 바이트 전송 함수
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
// LCD Enable 토글 함수
void lcd_toggle_enable(int bits)
{
    delayMicroseconds(500);
    wiringPiI2CReadReg8(lcd_fd, (bits | ENABLE));
    delayMicroseconds(500);
    wiringPiI2CReadReg8(lcd_fd, (bits & ~ENABLE));
    delayMicroseconds(500);
}
// LCD 초기화 함수
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
// LCD 커서 위치 설정 함수
void lcdLoc(int line)
{
    lcd_byte(line, LCD_CMD);
}
// LCD 화면 지우기 함수
void ClrLcd(void)
{
    lcd_byte(0x01, LCD_CMD);
    lcd_byte(0x02, LCD_CMD);
}
// 문자열 출력 함수
void typeln(const char* s)
{
    while (*s)
        lcd_byte(*(s++), LCD_CHR);
}
// 문자 출력 함수
void typeChar(char val)
{
    lcd_byte(val, LCD_CHR);
}
// DHT11 센서 데이터 읽기 함수
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
    //LCD값을 콘솔에 출력하는 부분
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

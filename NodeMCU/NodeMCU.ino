#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_MQTT.h>
#include <Adafruit_MQTT_Client.h>
#include "ArduinoJson.h"
#include "LiquidCrystal_I2C.h"
#include "DHT.h"
#include "music.h"

// LCD IIC 设备地址
#define LCD2004_ADDRESS 0x27

// WiFi SSID
#define WIFI_SSID "wifi320_1"
// WiFi 密码
#define WIFI_PASS "******"

// MQTT服务器地址
#define MQTT_SERVER "39.97.161.176"
// MQTT服务端口号
#define MQTT_PORT 1883
// MQTT服务用户名
#define MQTT_USERNAME "iot"
// MQTT服务密码
#define MQTT_PASS "******"

WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, MQTT_SERVER, MQTT_PORT, MQTT_USERNAME, MQTT_PASS);
// 传感器数据上报通道
Adafruit_MQTT_Publish mqttPush = Adafruit_MQTT_Publish(&mqtt, "/warehouse/data");
// 设备ID和传感器报警阈值接收通道
Adafruit_MQTT_Subscribe mqttSub = Adafruit_MQTT_Subscribe(&mqtt, "/warehouse/CriticalValue");

const char *mqtt_data = "{\"id\":%d,\"hum\":%.0f,\"temp\":%.1f,\"mq135\":%d}";
char buf[100];
char lastSend[100];

DHT dht;
StaticJsonDocument<512> doc;
LiquidCrystal_I2C lcd(LCD2004_ADDRESS, 16, 2);

int tonePin = D0;
int temp = 0;

// 设备ID
int mid = 0;
// 传感器报警阈值
float TEMP_MIN = -10;
float TEMP_MAX = 50;
float HUM_MIN = 5;
float HUM_MAX = 50;
int MQ135_MIN = 0;
int MQ135_MAX = 300;

void setup()
{
    lastSend[0] = '\0';
    // 初始化串口 波特率 115200
    Serial.begin(115200);
    pinMode(A0, INPUT);
    // 初始化温湿度传感器
    dht.setup(D3);
    // 初始化LCD
    lcd.init();
    lcd.backlight();
    pinMode(tonePin, OUTPUT);
    // 连接WiFi
    wifiInit(WIFI_SSID, WIFI_PASS);
    // 订阅MQTT接收通道
    mqtt.subscribe(&mqttSub);
    // 连接MQTT服务器
    MQTT_connect();
}

void loop()
{
    // 检测MQTT服务器连通性
    MQTT_connect();
    Adafruit_MQTT_Subscribe *subscription;
    // 如果有数据则接收数据
    while ((subscription = mqtt.readSubscription(1000)))
    {
        if (subscription == &mqttSub)
        {
            for (temp = 0; temp < 100; temp++)
                buf[temp] = '\0';
            Serial.print("CriticalValue: ");
            Serial.println((char *)mqttSub.lastread);
            // 解析传感器报警阈值
            deserializeJson(doc, (char *)mqttSub.lastread);
            temp = doc["mid"];
            TEMP_MIN = doc["temp"][0];
            TEMP_MAX = doc["temp"][1];
            HUM_MIN = doc["hum"][0];
            HUM_MAX = doc["hum"][1];
            MQ135_MIN = doc["mq135"][0];
            MQ135_MAX = doc["mq135"][1];
            Serial.printf("MAC: %s\nTEMP_MIN = %f\nTEMP_MAX = %f\nHUM_MIN=%f\nHUM_MAX = %f\nMQ135_MIN = %d\nMQ135_MAX = %d\n\n", buf, TEMP_MIN, TEMP_MAX, HUM_MIN, HUM_MAX, MQ135_MIN, MQ135_MAX);
            // 如果收到的mac地址是自己的mac地址，则mid就是自己的
            if(strcmp(doc["mac"],WiFi.macAddress().c_str()) == 0){
                mid = temp;
            }
        }
    }
    if (mid == 0)
    {
        // 向服务器发送自己的mac地址请求mid
        Serial.println("Wait register device ...");
        sprintf(buf, "{\"mac\":\"%s\"}", WiFi.macAddress().c_str());
        Serial.println(buf);
        if (mqttPush.publish(buf))
        {
            Serial.println("MQTT Register [OK]\n");
        }
        else
        {
            Serial.println("MQTT Register [Failed]\n");
        }
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.printf("Wait register");
        lcd.setCursor(0, 1);
        lcd.printf(" device...");
        return;
    }
    // 读取温湿度传感器数据
    delay(dht.getMinimumSamplingPeriod());
    float humidity = dht.getHumidity();
    float temperature = dht.getTemperature();
    // 读取空气质量
    int mq135Value = analogRead(A0);
    if (dht.getStatusString() == "OK")
    {
        // 如果传感器数值在正常范围内，则正常显示
        if (CriticalCheck(humidity, temperature, mq135Value) == 0)
        {
            lcd.setCursor(0, 0);
            lcd.printf("ID:%d MQ-135:%3d ", mid, mq135Value);
            lcd.setCursor(0, 1);
            lcd.printf("H:%.0f%% T:%.1f%cC  ", humidity, temperature, 0xdf);
        }
        sprintf(buf, mqtt_data, mid, humidity, temperature, mq135Value);
        // 如果上次发送的数据和这次不一样则上报传感器数据
        if (strcmp(buf, lastSend) != 0)
        {
            strcmp(lastSend, buf);
            Serial.printf(buf);
            Serial.printf("\n");
            if (mqttPush.publish(buf))
            {
                Serial.printf("MQTT Publish [OK]\n");
            }
            else
            {
                Serial.printf("MQTT Publish [Failed]\n");
            }
        }
    }
}

// 初始化 wifi 连接
void wifiInit(const char *ssid, const char *passphrase)
{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.printf("Wait connect");
    lcd.setCursor(0, 1);
    lcd.printf(" to WiFI...");
    Serial.print("\nMac Address: ");
    Serial.println(WiFi.macAddress());
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, passphrase);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
        Serial.println("WiFi not Connect");
    }
    Serial.println("WiFi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    lcd.clear();
}

void MQTT_connect()
{
    int8_t ret;
    if (mqtt.connected())
    {
        return;
    }
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.printf("Wait connect");
    lcd.setCursor(0, 1);
    lcd.printf(" to MQTT...");
    Serial.print("Connecting to MQTT... ");
    while ((ret = mqtt.connect()) != 0)
    {
        mid = 0;
        Serial.println("Retrying MQTT connection in 5 seconds...");
        Serial.println(mqtt.connectErrorString(ret));
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Reconnect to");
        for (temp = 1; temp <= 5;temp++){
            lcd.setCursor(0, 1);
            sprintf(buf, " MQTT in %ds...", 6 - temp);
            lcd.print(buf);
            delay(1000);
        }
        mqtt.disconnect();
    }
    Serial.println("MQTT Connected!");
}

// 播放音乐（未调用）
void play_music()
{
    int length = sizeof(tune) / sizeof(tune[0]);
    for (int x = 0; x < length; x++)
    {
        tone(tonePin, tune[x]);
        delay(800 * duration[x]);
        noTone(tonePin);
    }
    delay(5000);
}

// 检测传感器数值是否在正常范围内
int CriticalCheck(float hum, float temp, int mq135)
{
    static int mode = 0;
    static int i = 200;

    int alert = 0;
    if(hum < HUM_MIN){
        alert = 1;
        sprintf(buf, "H:%.0f%%<MIN %.0f%%   ", hum, HUM_MIN);
    }else if(hum > HUM_MAX){
        alert = 1;
        sprintf(buf, "H:%.0f%%>MAX %.0f%%   ", hum, HUM_MAX);
    }
    else if (temp < TEMP_MIN)
    {
        alert = 1;
        sprintf(buf, "T:%.0f%cC<MIN %.0f%cC   ", temp, 0xdf, TEMP_MIN, 0xdf);
    }
    else if (temp > TEMP_MAX)
    {
        alert = 1;
        sprintf(buf, "T:%.0f%cC<MAX %.0f%cC", temp, 0xdf, TEMP_MAX, 0xdf);
    }
    else if (mq135 < MQ135_MIN)
    {
        alert = 1;
        sprintf(buf, "MQ-135:%d < %d   ", mq135, MQ135_MIN);
    }
    else if (mq135 > MQ135_MAX)
    {
        alert = 1;
        sprintf(buf, "MQ-135:%d > %d   ", mq135, MQ135_MAX);
    }
    if(alert == 1){
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.printf("ID:%d Alert!!!   ", mid);
        lcd.setCursor(0, 1);
        lcd.printf(buf);
        if(mode == 0){
            pinMode(D0, OUTPUT);
            tone(D0, i);
            delay(5);
            i += 1;
            if (i == 800){
                mode = 1;
                return alert;
            }
        }else if(mode == 1){
            pinMode(D0, OUTPUT);
            tone(D0, i);
            delay(5);
            i -= 1;
            if(i==200){
                mode = 0;
                return alert;
            }
        }
    }else{
        pinMode(D0, OUTPUT);
        noTone(D0);
    }
    return alert;
}
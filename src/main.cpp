#include <MakestroCloudClient32.h>
#include "WiFi.h"
#include "i2sRecorder.h"
#include <Wire.h>
#define use_ESPectro32 1

#if use_ESPectro32
    #include <ESPectro32_Board.h>
    #include <ESPectro32_LedMatrix_Animation.h>
    ESPectro32_LedMatrix_Animation ledMatrixAnim;
    #include <ESPectro32_RGBLED_Animation.h>
    //ESPectro32_LedMatrix_ScrollTextAnimation ledMatrixTextAnim;
    RgbLedColor_t aCol(200, 0, 80);
    ESPectro32_RGBLED_GlowingAnimation glowAnim(ESPectro32.RgbLed(), aCol);

    #include "icon.h"
    #include <LSM6DSL.h>
    LSM6DSL imu(LSM6DSL_MODE_I2C, 0x6B);
#endif

const int ledPin = 15;
const int sendPeriod = 2000;
unsigned long oldTime = 0;

const char* SSID = "DyWare 1";
const char* Password = "p@ssw0rd";
const char* username = "Hisyam_Kamil";
const char* token = "CjIzfO689VdBxb0O5krsgLk7zdQWEDlwhz49eA0AaZ7b6rW2ZMAryQiguDIqeyE0";
const char* project = "ESPectro32";
const char* deviceId = "Hisyam_Kamil-ESPectro32-default";
const char* topic = "Hisyam_Kamil/ESPectro32/data";
const char* subsTopic = "Hisyam_Kamil/ESPectro32/control";

MakestroCloudClient32 makestro;

void callback(char* topic, byte* payload, unsigned int length) {
    Serial.println("Message arrived");
    char msg = (char)payload[9];
    
    if (msg == '1') {
        digitalWrite(ledPin, LOW);
    } else {
        digitalWrite(ledPin, HIGH);
    }
}

void setup(){
    Serial.begin(115200);
    #if use_ESPectro32
        ESPectro32.begin();
    #endif

    // If too long to connect, uncomment line below, then reupload
    // WiFi.disconnect();
    delay(100);
    pinMode(ledPin, OUTPUT);
    WiFi.begin(SSID, Password);
    ledMatrixAnim.setLedMatrix(ESPectro32.LedMatrix());
	//ledMatrixTextAnim.setLedMatrix(ESPectro32.LedMatrix());

	ledMatrixAnim.addFrameWithData((uint8_t*)LED_MATRIX_WIFI_1);
	ledMatrixAnim.addFrameWithData((uint8_t*)LED_MATRIX_WIFI_2);
	ledMatrixAnim.addFrameWithData((uint8_t*)LED_MATRIX_WIFI_3);
	ledMatrixAnim.addFrameWithDataCallback([](ESPectro32_LedMatrix &ledM) {
		ledM.clear();
	});

    while (WiFi.status() != WL_CONNECTED) {
    
        Serial.print(".");
        ledMatrixAnim.start(1800);
        ESPectro32.LED().setAnimation(ESPectro_LED_Animation_Fading, 3000, 3);
	    glowAnim.start(500, 1);
        delay(500);
    }
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
    
    ledMatrixAnim.stop();
    delay(3000);   

    // MQTT initializing..
    makestro.connect(username, token, project, deviceId);
    makestro.subscribe(subsTopic);
    makestro.data()->setCallback(callback);
    delay(1000);
    if (!imu.begin()) {
        Serial.println("Failed initializing LSM6DSL");
    }
    delay(3000);

}

void loop(){
    
    if(!makestro.connected()){
        makestro.connect(username, token, project, deviceId);
        makestro.subscribe(subsTopic);
    }
    makestro.loop();

    if(millis() - oldTime > sendPeriod){
        oldTime = millis();
        int phototr = ESPectro32.readPhotoTransistorValue();
        Serial.print("phototr: ");
        Serial.println(phototr);

        if (phototr < 500){
            makestro.publishKeyValue(subsTopic, "state", 1);
        }

        else if (phototr >500){
            makestro.publishKeyValue(subsTopic, "state", 0);   
        }
        double accX = imu.readFloatAccelX();
        Serial.print("accX: ");
        Serial.println(accX);
        makestro.publishKeyValue(topic, "accX", accX);

        double accY = imu.readFloatAccelY();
        Serial.print("accY: ");
        Serial.println(accY);
        makestro.publishKeyValue(topic, "accY", accY);

        double accZ = imu.readFloatAccelZ();
        Serial.print("accZ: ");
        Serial.println(accZ);
        makestro.publishKeyValue(topic, "accZ", accZ);

        double gyroX = imu.readFloatGyroX();
        Serial.print("gyroX: ");
        Serial.println(gyroX);
        makestro.publishKeyValue(topic, "gyroX", gyroX);

        double gyroY = imu.readFloatGyroY();
        Serial.print("gyroY: ");
        Serial.println(gyroY);
        makestro.publishKeyValue(topic, "gyroY", gyroY);

        double gyroZ = imu.readFloatGyroZ();
        Serial.print("gyroZ: ");
        Serial.println(gyroZ);
        makestro.publishKeyValue(topic, "gyroZ", gyroZ);

        double tempC = imu.readTemperatureC();
        Serial.print("temp: ");
        Serial.println(tempC);
        makestro.publishKeyValue(topic, "tempC", tempC);
        
        makestro.publishKeyValue(topic, "sensor", phototr);
        Serial.println("publish to cloud");
            
    }
}

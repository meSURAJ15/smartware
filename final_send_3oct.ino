#include <Wire.h>
#include <MAX30100_PulseOximeter.h>
#include <Adafruit_MLX90614.h>
#include <LoRa.h>
#include <SPI.h>

PulseOximeter pox;
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

#define SDA_PIN_MAX30100 21
#define SCL_PIN_MAX30100 22

#define SDA_PIN_MLX90614 4
#define SCL_PIN_MLX90614 27
#define MLX90614_ADDR 0x5A

#define GSR_PIN 34

#define LORA_SS 5
#define LORA_RST 14
#define LORA_DIO0 2

TwoWire WireMAX30100(0);
TwoWire WireMLX90614(1);

uint32_t tsLastReport = 0;

void onBeatDetected() {
    Serial.println("Beat!");
}

bool initializePulseOximeter() {
    Serial.print("Initializing pulse oximeter...");
    if (!pox.begin()) {
        Serial.println("FAILED");
        return false;
    } else {
        Serial.println("SUCCESS");
        pox.setOnBeatDetectedCallback(onBeatDetected);
        return true;
    }
}

bool initializeMLX90614() {
    Serial.print("Initializing MLX90614...");
    WireMLX90614.begin(SDA_PIN_MLX90614, SCL_PIN_MLX90614);
    if (mlx.begin(MLX90614_ADDR, &WireMLX90614)) {
        delay(40);
        Serial.println("SUCCESS");
        return true;
    } else {
        Serial.println("FAILED");
        return false;
    }
}

bool initializeLoRa() {
    Serial.print("Initializing LoRa...");
    LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
    if (!LoRa.begin(433E6)) {
        Serial.println("FAILED");
        return false;
    }
    LoRa.setSyncWord(0xA5);
    Serial.println("SUCCESS");
    return true;
}

void setup() {
    Serial.begin(115200);
    while (!Serial);

    if (!initializePulseOximeter() || !initializeMLX90614() || !initializeLoRa()) {
        Serial.println("Initialization failed. Halting program.");
        while (true);
    }

    pinMode(GSR_PIN, INPUT);
}

void loop() {
    pox.update();

    if (millis() - tsLastReport > 10000) {  // Send data every 10 seconds
        float heartRate = pox.getHeartRate();
        float spO2 = pox.getSpO2();
        float ambientTempC = mlx.readAmbientTempC();
        float objectTempC = mlx.readObjectTempC();
        int gsrValue = analogRead(GSR_PIN);
      

        // Prepare data string
        String data = "HR:" + String(heartRate) + 
                      ",SpO2:" + String(spO2) + 
                      ",AmbT:" + String(ambientTempC) + 
                      ",ObjT:" + String(objectTempC) + 
                      ",GSR:" + String(gsrValue);

        // Send data via LoRa
        LoRa.beginPacket();
        LoRa.print(data);
        LoRa.endPacket();

        // Print data to Serial for debugging
        Serial.println("Sending LoRa packet: " + data);

        tsLastReport = millis();
    }
}
#include <Arduino.h>

class Buzzer {
   public:
    Buzzer(int buzzerPin) {
        this->buzzerPin = buzzerPin;
    }
    
    void setup() {
        pinMode(buzzerPin, OUTPUT);
    }

    void beep() {
        tone(buzzerPin, 800, 300);
    }

    void leaderTone() {
        tone(buzzerPin, 800, 500);
    }

    void followerTone() {
        tone(buzzerPin, 800, 150);
        delay(300);
        tone(buzzerPin, 800, 150);
    }

    void connectionTone() {
        tone(buzzerPin, 600, 150);
        delay(150);
        tone(buzzerPin, 800, 150);
        delay(150);
        tone(buzzerPin, 1000, 150);
    }

    void activationTone() {
        tone(buzzerPin, 400, 200);
        delay(200);
        tone(buzzerPin, 800, 200);
    }

    void deactivationTone() {
        tone(buzzerPin, 800, 200);
        delay(200);
        tone(buzzerPin, 400, 200);
    }

   private:
    int buzzerPin;
};

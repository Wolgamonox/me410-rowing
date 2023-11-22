#include <Arduino.h>

class Buzzer {
   public:
    Buzzer(int buzzerPin) {
        this->buzzerPin = buzzerPin;
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

   private:
    int buzzerPin;
};

#include <Servo.h>

Servo ctrl;
int val =0;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
    ctrl.attach(9);

    
    ctrl.write(180);

}

void loop() {
  // put your main code here, to run repeatedly:

}

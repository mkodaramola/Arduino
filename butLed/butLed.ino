int led = 8;

int button = 9;

void setup() {
  // put your setup code here, to run once:

        pinMode(led,OUTPUT);
        pinMode(button,INPUT);
        
}

void loop() {
  // put your main code here, to run repeatedly:


    if(digitalRead(button) == HIGH){
      
        digitalWrite(led,HIGH);
      }
      else{

        digitalWrite(led,LOW);
        
        }
      



}

int ipt = 0;
//extern unsigned long timer0_millis;
#define D6 6

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(A2, INPUT);
  pinMode(A3, INPUT);
  pinMode(D6, OUTPUT);
  digitalWrite(D6, LOW);
}

int TransInput(int a){
  if(a > 150){
    return 462 - 0.412 * a;             //Digital 150 is corresponding to 1kPa and scale to 50-400ms interval
  }
  else{
    return 0;
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  ipt = TransInput(analogRead(A2));
  Serial.println(ipt);                  //check the value of ipt
  if(ipt >= 50){
    digitalWrite(D6, HIGH);
    //Serial.println(digitalRead(D6));  //check the state of D6
    delay(100);                         //--adjustable parameter--
    digitalWrite(D6, LOW);
    //Serial.println(digitalRead(D6));  //check the state of D6
    delay(ipt);
  }
  else if(ipt > 0){
    digitalWrite(D6, HIGH);
    delay(100);
    digitalWrite(D6, LOW);
    delay(50);                          //up-limit 50ms
  }
  else{
    digitalWrite(D6 ,LOW);
  }
  //Serial.println(digitalRead(A6));
}

int A = A0;

int B = A1;

 int pwm = 9;
 int c1 = 0;
 int c2 = 0;
 int c3 = 0;
 int c4 = 0;
 int c5 = 0;
 int c6 = 0;
 int s1 = 0;
 int s2 = 0;
 int s3 = 0;
 int s4 = 0;
 int s5 = 0;
 int s6 = 0;
 float output = 0.00;
 int sin1 = 0;
 int cos1 = 0;
 int position_state = 1;
 int get_position = 0;

 void setup() 
 { 
  // setup code here, to run once: pinMode(pwm, OUTPUT);
  Serial.begin(115200); 
}

void loop() {

if(get_position<5){ 
  switch(position_state){ 
    case(1): digitalWrite(pwm, HIGH);
     delayMicroseconds(15);
     position_state +=1;

    break;

    case(2): position_state +=1;
     delayMicroseconds(5);

    break;

    case(3): position_state +=1;
     c1+= analogRead(A);
     s1+= analogRead(B);
     delayMicroseconds(5);

    break;

    case(4): position_state +=1;
     c2+= analogRead(A);
     s2+= analogRead(B);
     delayMicroseconds(5);

    break;

    case(5): position_state +=1;
     delayMicroseconds(5);

    break;

    case(6): position_state +=1;
     digitalWrite(pwm, LOW);
     delayMicroseconds(5);

    break;

    case(7): position_state +=1;
     delayMicroseconds(5);

    break;

    case(8): position_state +=1;
     c3+= analogRead(A);
     s3+= analogRead(B);
     delayMicroseconds(5);

    break;

    case(9): position_state +=1;
     c4+= analogRead(A);
     s4+= analogRead(B);
     delayMicroseconds(5);

    break;

    case(10): position_state = 1;
     get_position +=1;
     delayMicroseconds(5);

    break;

    default: 
    
    break; 
  } 
} 

else 
  if(get_position>=5){ 
    cos1 = (c1+c2)-(c3+c4); sin1 = (s1+s2)-(s3+s4); 
    output = atan2(cos1, sin1); 
    c1 = 0; c2 = 0; c3 = 0; c4 = 0; s1 = 0; s2 = 0; s3 = 0; s4 = 0; 
    Serial.print("Position: "); 
    Serial.println(output); 
    get_position = 1; 
  }

// main code here, LOOP

}
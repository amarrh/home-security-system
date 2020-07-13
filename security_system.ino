 //uključivanje biblioteka
#include <LiquidCrystal.h>
#include <Password.h> 
#include <Keypad.h> 
#include <Servo.h> 
#include "SIM900.h"
#include <SoftwareSerial.h>
#include "sms.h"

//Servo
Servo motorCam;        // kreiranje Servo objekta   
int pozicija;         // varijabla za spremanje pozicije motora

int passwd_poz = 15;  // pozicija trenutnog unosa passworda

SMSGSM sms; // kreiranje SMSGSM objekta 
boolean started=false; // varijabla za provjeru da li je uspostavljena komunikacija sa GSM modulom
boolean poslano = false; // varijabla za provjeru da li je poslana SMS poruka  

Password password = Password( "1456" ); //Password

// tastatura
const byte ROWS = 4; // 4 reda
const byte COLS = 4; // 4 kolone

// tipke
char keys[ROWS][COLS] = { 
  {
    '1','2','3','A'      }
  ,
  {
    '4','5','6','B'      }
  ,
  {
    '7','8','9','C'      }
  ,
  {
    '*','0','#','D'      }
};

// pinovi
byte rowPins[ROWS] = {36,34,32,30}; 
byte colPins[COLS] = {28,26,24,22}; 

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS ); // Keypad objekt

LiquidCrystal lcd(1, 2, 4, 5, 6, 7); // LiquidCrystal lcd objekt sa 
                                        definisanim pinovima

// senzori pokreta
int senzorGaraza = 38;
int senzorKuca = 40; 

// magnetni kontakti za detekciju otvaranja vrata i prozora
int vrataKuca = 8;
int vrataGaraza = 12;
int prozorKuca = 44;

// LED diode
int boravakLED = 9;
int garazaLED = 10;
int kuhinjaLED = 11;

// RGB dioda - svjetlosni indikator
int crvenaRGB = 47;
int zelenaRGB = 48;
int plavaRGB = 49;

int alarm = 53; // alarm buzzer

// pomoćne varijable za rad sistema
int sistemAktivan = 0;
int alarmStatus = 0;
int prostorija = 0;
int slucaj;
int i;

void setup(){
  if (gsm.begin(2400)){ // uspostavljanje GSM komunikacije
      started=true;   // memorisanje ukoliko je komunikacija uspješna
  }
  lcd.begin(16, 2); //inicijalizacija LCD displeja
  lcdPocetna(); // ispis početne poruke na LCD ekranu
  motorCam.attach(52);  // dodjeljivanje pina servo motoru 
  motorCam.write(85); // početna pozicija servo motora

  // ulazni senzori definisani kao input
  pinMode(senzorGaraza, INPUT);
  pinMode(senzorKuca, INPUT);
  pinMode(vrataGaraza, INPUT_PULLUP);
  pinMode(vrataKuca, INPUT_PULLUP);
  pinMode(prozorKuca, INPUT_PULLUP);
  
  // alarm, LED i RGB diode definisani kao output
  pinMode(crvenaRGB, OUTPUT);
  pinMode(zelenaRGB, OUTPUT);
  pinMode(plavaRGB, OUTPUT);
  pinMode(garazaLED, OUTPUT);
  pinMode(boravakLED, OUTPUT);
  pinMode(kuhinjaLED, OUTPUT);
  pinMode(alarm, OUTPUT);
  digitalWrite(alarm, HIGH);
  
  keypad.addEventListener(keypadEvent); // "osluškivanje" tastature
}
void loop(){
  keypad.getKey(); // spremanje unešenog znaka sa tastature

  if (sistemAktivan == 1){ 
    if (digitalRead(senzorKuca) == HIGH || digitalRead(prozorKuca) == HIGH || digitalRead(vrataKuca) == HIGH)
    {
      prostorija = 0; // detekcija senzora koji se nalaze u prostoriji  
                         kuće

      if(digitalRead(senzorKuca) == HIGH && digitalRead(vrataKuca) == LOW && digitalRead(prozorKuca) == LOW) slucaj = 0; // preciziranje konkretnog slučaja detekcije
      
      else if(digitalRead(senzorKuca) == HIGH && digitalRead(vrataKuca) == HIGH && digitalRead(prozorKuca) == LOW) slucaj = 1; 
       
      else if(digitalRead(senzorKuca) == HIGH && digitalRead(vrataKuca) == LOW && digitalRead(prozorKuca) == HIGH) slucaj = 2; 

      else if(digitalRead(senzorKuca) == HIGH && digitalRead(vrataKuca) == HIGH && digitalRead(prozorKuca) == HIGH) slucaj = 3; 

      else if(digitalRead(senzorKuca) == LOW && digitalRead(vrataKuca) == LOW && digitalRead(prozorKuca) == HIGH) slucaj = 4; 

      else if(digitalRead(senzorKuca) == LOW && digitalRead(vrataKuca) == HIGH && digitalRead(prozorKuca) == LOW) slucaj = 5; 

      else if(digitalRead(senzorKuca) == LOW && digitalRead(vrataKuca) == HIGH && digitalRead(prozorKuca) == HIGH) slucaj = 6; 
        
      detekcija();
    }
    if (digitalRead(senzorGaraza) == HIGH || digitalRead(vrataGaraza) == HIGH)
    {
      prostorija = 1;

      if(digitalRead(senzorGaraza) == HIGH && digitalRead(vrataGaraza) == HIGH) slucaj = 7;

      else if(digitalRead(senzorGaraza) == HIGH && digitalRead(vrataGaraza) == LOW) slucaj = 8;

      else if(digitalRead(senzorGaraza) == LOW && digitalRead(vrataGaraza) == HIGH) slucaj = 9;
      
      detekcija();
    }
   }
  }

void keypadEvent(KeypadEvent eKey){
  switch (keypad.getState()){
  case PRESSED:
    if (passwd_poz - 15 >= 5) { // unos ograničen na 4 znaka
      return;
    }
    lcd.setCursor((passwd_poz++),0);
    switch (eKey){
    case '#':                 //# za potvrdu unosa
      passwd_poz  = 15;
      checkPassword(); // funkcija za provjeru unešenog PIN-a
      break;
    case '*':                 //* za reset unosa
      password.reset(); 
      passwd_poz = 15;
      break;
    default: 
      password.append(eKey); //spremanje unešenog znaka
      lcd.print("*");
    }
  }
}

void detekcija(){

  //digitalWrite(alarm, LOW); // aktivacija sigurnosnog alarma

  password.reset();
  alarmStatus = 1;
  //uključuju se sva svjetla kuće
  digitalWrite(zelenaRGB, LOW);        
  digitalWrite(crvenaRGB, HIGH);       
  digitalWrite(boravakLED, HIGH);       
  digitalWrite(kuhinjaLED, HIGH);       
  digitalWrite(garazaLED, HIGH);        

  //ispis na LCD displeju, te prolazak kroz slučajeve detekcije      
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("DETEKCIJA!");
  delay(1000);
  
  if(prostorija == 0){
    if(slucaj == 0){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("DETEKCIJA!");
        lcd.setCursor(0,1);
        lcd.print("Kretnja u kuci!"); // ispis na LCD displeju
        for(i = 85; i < 165; i++){ // motor mijenja ugao snimanja nadzorne kamere u zonu detekcije
         motorCam.write(i);
         delay(15);
        }
        motorCam.write(125);
    }
    else if(slucaj == 1){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Kretnja u kuci!");
        lcd.setCursor(0,1);
        lcd.print("Otvorena vrata!");
        for(i = 85; i < 130; i++){
          motorCam.write(i);
          delay(15);
        }
        motorCam.write(103);
      }
    else if(slucaj == 2){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Kretnja u kuci!");
        lcd.setCursor(0,1);
        lcd.print("Otvoren prozor!");
        for(i = 130; i < 165; i++){
          motorCam.write(i);
          delay(15);
     }
        motorCam.write(147);
      }
    else if(slucaj == 3){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Kretnja u kuci!");
        lcd.setCursor(0,1);
        lcd.print("Otvorena vrata!");
        delay(1000);
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Otvorena vrata!");
        lcd.setCursor(0,1);
        lcd.print("Otvoren prozor!");
        for(i = 85; i < 165; i++){
          motorCam.write(i);
          delay(15);
        }
        motorCam.write(125);
    }
    else if(slucaj == 4){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("DETEKCIJA!");
        lcd.setCursor(0,1);
        lcd.print("Otvoren prozor!");
        for(i = 130; i < 165; i++){
          motorCam.write(i);
          delay(15);
        }
        motorCam.write(147);
    }
    else if(slucaj == 5){
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.print("DETEKCIJA!");
       lcd.setCursor(0,1);
       lcd.print("Otvorena vrata!");
       for(i = 85; i < 130; i++){
        motorCam.write(i);
        delay(15);
       }
       motorCam.write(103); 
    }      
   else if(slucaj == 6){
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.print("Otvorena vrata!");
       lcd.setCursor(0,1);
       lcd.print("Otvoren prozor!"); 
       for(i = 85; i < 165; i++){
        motorCam.write(i);
        delay(15);
       }
       motorCam.write(125);
    }
  }
  else if(prostorija == 1){
    if(slucaj == 7){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Kretnja u garazi!");
        lcd.setCursor(0,1);
        lcd.print("Otvorena vrata!");
      }
    else if(slucaj == 8){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("DETEKCIJA!");
        lcd.setCursor(0,1);
        lcd.print("Kretnja u garazi");
      }
    else if(slucaj == 9){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Otvorena garazna");
        lcd.setCursor(0,1);
        lcd.print("vrata!");
      }
    for(i = 35; i < 67; i++){
        motorCam.write(i);
        delay(100);
    }
      motorCam.write(51);
  }
   svjetla();
    
  // slanje SMS poruke
    if(started == true && poslano == false){ 
      sms.SendSMS("38761816703", "DETEKCIJA! Aktiviran sigurnosni 
                   alarm!");
      poslano = true;
  }
 }

// Provjera unešenog PIN-a
void checkPassword(){                
  if (password.evaluate()) // evaluacija unosa
  {  
    if(sistemAktivan == 0) // provjera da li je sistem već aktiviran
    {
      provjera(); // ukoliko je unešen tačan PIN, i sistem nije 
                     prethodno aktiviran, prije aktivacije sistema 
                     vrši  se provjera da li su otvorena vrata ili 
                     prozor
    } 
    else if(sistemAktivan == 1) {
      deaktiviraj(); // ukoliko je sistem već aktivan, unosom PIN-a 
                        sistem se deaktivira 
    }
  } 
  else {
    invalidCode(); // unosom netačnog PIN-a sistem ispisuje 
                      odgovarajuću poruku implementiranu u funkciji   
                      invalidCode()
  }
}  

// prije aktivacije sistema vrši se provjera da li su otvorena vrata kuće, prozor ili garažna vrata, uz ispis poruke na ekranu
void provjera(){
    password.reset();
    if(digitalRead(vrataKuca) == HIGH){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Otvorena vrata!");
        lcd.setCursor(0,1);
        lcd.print("Pokusajte ponovo");
        delay(2000);
        lcdPocetna();
    }
    else if(digitalRead(vrataGaraza) == HIGH){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Otvorena garaza!");
        lcd.setCursor(0,1);
        lcd.print("Pokusajte ponovo");
        delay(2000);
        lcdPocetna();
        }
    else if(digitalRead(prozorKuca) == HIGH){
        lcd.clear();
        lcd.setCursor(0,0);
        lcd.print("Otvoren prozor!");
        lcd.setCursor(0,1);
        lcd.print("Pokusajte ponovo");
        delay(2000);
        lcdPocetna();
        }
 else aktiviraj();
  }

// ispis poruke ukoliko je PIN netačan
void invalidCode()   
{
  password.reset();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Pogresan pin");
  lcd.setCursor(0,1);
  lcd.print("Novi pokusaj");

  delay(2000);
  if(sistemAktivan == 0){
    lcdPocetna();
  }
}

// aktivacija sistema ukoliko je unešen ispravan PIN
void aktiviraj()      
{
    sistemAktivan = 1;
    digitalWrite(zelenaRGB, HIGH);
    digitalWrite(crvenaRGB, LOW);
    digitalWrite(plavaRGB, LOW);
    
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Sistem aktivan!"); 
    password.reset();
    delay(2000);
}

// deaktivacija sistema
void deaktiviraj() 
{
  sistemAktivan = 0;
  alarmStatus = 0;
  digitalWrite(alarm, HIGH);

  digitalWrite(boravakLED, LOW);       
  digitalWrite(kuhinjaLED, LOW);       
  digitalWrite(garazaLED, LOW);   
  digitalWrite(zelenaRGB, LOW);
  digitalWrite(crvenaRGB, LOW);
  digitalWrite(plavaRGB, HIGH);
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("  Sistem  ");
  lcd.setCursor(0,1);
  lcd.print("  deaktiviran!  ");
  
  password.reset();
  delay(5000);

  lcdPocetna();
}

// aktiviranje svjetlosnog indikatora
void svjetla(){                                                      
    digitalWrite(zelenaRGB,LOW);
    digitalWrite(crvenaRGB, HIGH);       
    delay(50); 
    digitalWrite(crvenaRGB, LOW);         
    delay(50); 
    digitalWrite(crvenaRGB, HIGH);       
    delay(50); 
    digitalWrite(crvenaRGB, LOW);         
    delay(50); 
    digitalWrite(crvenaRGB, HIGH);        
    delay(50); 
    digitalWrite(crvenaRGB, LOW);         
    delay(50); 
    digitalWrite(zelenaRGB,LOW);
    digitalWrite(crvenaRGB, HIGH);        
    delay(50); 
    digitalWrite(crvenaRGB, LOW);         
    delay(50); 
    digitalWrite(crvenaRGB, HIGH);        
    delay(50); 
    digitalWrite(crvenaRGB, LOW);        
    delay(50); 
    digitalWrite(crvenaRGB, HIGH);        
    delay(50); 
    digitalWrite(crvenaRGB, LOW);         
    delay(50); 
    delay(100); 
    digitalWrite(plavaRGB, HIGH);       
    delay(50); 
    digitalWrite(plavaRGB, LOW);        
    delay(50); 
    digitalWrite(plavaRGB, HIGH);       
    delay(50); 
    digitalWrite(plavaRGB, LOW);        
    delay(50); 
    digitalWrite(plavaRGB, HIGH);       
    delay(50); 
    digitalWrite(plavaRGB, LOW);        
    delay(50); 
    digitalWrite(plavaRGB, HIGH);      
    delay(50); 
    digitalWrite(plavaRGB, LOW);        
    delay(50); 
    digitalWrite(plavaRGB, HIGH);       
    delay(50); 
    digitalWrite(plavaRGB, LOW);       
    delay(50); 
    digitalWrite(plavaRGB, HIGH);       
    delay(50); 
    digitalWrite(plavaRGB, LOW);        
    delay(50); 
    digitalWrite(crvenaRGB, HIGH);
    }      
            
//početni ispis na LCD ekranu
void lcdPocetna()    
{
  digitalWrite(plavaRGB, HIGH);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("   SIGURNOSNI   ");
  lcd.setCursor(0,1);
  lcd.print("    SISTEM    ");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Unesite PIN:");
}



/*
>> Pulse Sensor Amped 1.1 <<
This code is for Pulse Sensor Amped by Joel Murphy and Yury Gitman
    www.pulsesensor.com 
    >>> Pulse Sensor purple wire goes to Analog Pin 0 <<<
Pulse Sensor sample aquisition and processing happens in the background via Timer 2 interrupt. 2mS sample rate.
PWM on pins 3 and 11 will not work when using this code, because we are using Timer 2!
The following variables are automatically updated:
Signal :    int that holds the analog signal data straight from the sensor. updated every 2mS.
IBI  :      int that holds the time interval between beats. 2mS resolution.
BPM  :      int that holds the heart rate value, derived every beat, from averaging previous 10 IBI values.
QS  :       boolean that is made true whenever Pulse is found and BPM is updated. User must reset.
Pulse :     boolean that is true when a heartbeat is sensed then false in time with pin13 LED going out.

Code Version 02 by Joel Murphy & Yury Gitman  Fall 2012
This update changes the HRV variable name to IBI, which stands for Inter-Beat Interval, for clarity.
Switched the interrupt to Timer2.  500Hz sample rate, 2mS resolution IBI value.
Fade LED pin moved to pin 5 (use of Timer2 disables PWM on pins 3 & 11).
Tidied up inefficiencies since the last version. 
*/


//  VARIABLES
int pulsePin = 0;                 // Pulse Sensor purple wire connected to analog pin 0
int fadePin = 1;                  // pin to do fancy classy fading blink at each beat
int fadeRate = 0;                 // used to fade LED on with PWM on fadePin
int useBPM;                       // BPM for computation use
int useSweat;                     // Sweat for computation use
int useBreath;                    // Breath for computation use
int lastHundredBPM[100];          // last 100 BPMs
int lastHundredSweat[100];        // last 100 Sweats
int lastHundredBreath[100];       // last 100 Breaths
int lastS;                        // last sweat[x]
int lastB;                        // last breath[x]
int lastIterator;                 // last BPM[x]
int baseline;                     // baseline reading 
//BASELINE = ((.50*BPM)+(.25*Sweat)+(.25*Breath))

// these variables are volatile because they are used during the interrupt service routine!
volatile int BPM;                   // used to hold the pulse rate
volatile int Signal;                // holds the incoming raw data
volatile int IBI = 600;             // holds the time between beats, the Inter-Beat Interval
volatile boolean Pulse = false;     // true when pulse wave is high, false when it's low
volatile boolean QS = false;        // becomes true when Arduoino finds a beat.

//for switch/button 
int inPin = 12;         // the number of the input pin
int state = HIGH;      // the current state of the output pin
int reading;           // the current reading from the input pin
int previous = LOW;    // the previous reading from the input pin

// the follow variables are long's because the time, measured in miliseconds,
// will quickly become a bigger number than can be stored in an int.
long time = 0;         // the last time the output pin was toggled
long debounce = 200;   // the debounce time, increase if the output flickers

void setup(){
  Serial.begin(115200);             // we agree to talk fast!
  interruptSetup();                 // sets up to read Pulse Sensor signal every 2mS 
   // UN-COMMENT THE NEXT LINE IF YOU ARE POWERING The Pulse Sensor AT LOW VOLTAGE, 
   // AND APPLY THAT VOLTAGE TO THE A-REF PIN
   //analogReference(EXTERNAL);   
   for(int i=0; i<100; i++)
   {
     lastHundredBPM[i]=0;
     lastHundredSweat[i]=0;
     lastHundredBreath[i]=0;
   }
  digitalWrite(1,LOW);
  digitalWrite(2,HIGH);
  digitalWrite(4,HIGH);
  pinMode(inPin, INPUT);
}

void handleSweat()
{  
  //Serial.println(useSweat);
  if(lastS == 99)
  {
    lastHundredSweat[99] = useSweat;
    lastS = 0;
  }
  else
  {
    lastHundredSweat[lastS] = useSweat;
    lastS++;
  }
}
void handleBreath()
{
  //Serial.println(useBreath);
  if(lastB == 99)
  {
    lastHundredBreath[99] = useBreath;
    lastB = 0;
  }
  else
  {
    lastHundredBreath[lastB] = useBreath;
    lastB++;
  }
}
/*
---------------------------------------------------
---------------------------------------------------
---------------------------------------------------
---------------------------------------------------
---------------------------------------------------
---------------------------------------------------
---------------------------------------------------
---------------------------------------------------
---------------------------------------------------
---------------------------------------------------
---------------------------------------------------
*/
void loop(){
  if (QS == true){                       // Quantified Self flag is true when arduino finds a heartbeat
        fadeRate = 255;                  // Set 'fadeRate' Variable to 255 to fade LED with pulse
        QS = false;                      // reset the Quantified Self flag for next time    
     }
  reading = digitalRead(inPin);
  // if the input just went from LOW and HIGH and we've waited long enough
  // to ignore any noise on the circuit, toggle the output pin and remember
  // the time
  if (reading == HIGH && previous == LOW && millis() - time > debounce) 
  {
    if (state == HIGH)
    {
      int tempa,tempb;
      int ts, tb;
      state = LOW;
      previous = HIGH;
      for(int i=0; i<100; i++)
      {
        tempa+=lastHundredSweat[i];
        tempb+=lastHundredBreath[i];
      }
      ts = tempa/100;
      tb = tempb/100;
      //baseline = ((useBPM*.5)+(ts*.3)+(tb*.2)/3); 
      baseline = useBPM;
       
      Serial.write(baseline);
      digitalWrite(5,HIGH);
      digitalWrite(6,HIGH);
    }
    time = millis();    
  }
  delay(20);
  useSweat = analogRead(A5);
  useBreath = analogRead(A3);
  handleSweat();
  handleBreath();
}
/*
--------------------------------------------------------------------
--------------------------------------------------------------------
--------------------------------------------------------------------
--------------------------------------------------------------------
--------------------------------------------------------------------
--------------------------------------------------------------------
--------------------------------------------------------------------
*/
volatile int rate[10];                    // used to hold last ten IBI values
volatile unsigned long sampleCounter = 0;          // used to determine pulse timing
volatile unsigned long lastBeatTime = 0;           // used to find the inter beat interval
volatile int P =512;                      // used to find peak in pulse wave
volatile int T = 512;                     // used to find trough in pulse wave
volatile int thresh = 512;                // used to find instant moment of heart beat
volatile int amp = 100;                   // used to hold amplitude of pulse waveform
volatile boolean firstBeat = true;        // used to seed rate array so we startup with reasonable BPM
volatile boolean secondBeat = true;       // used to seed rate array so we startup with reasonable BPM

void getupandSLAM()
{
  int temp; 
  temp = baseline;
  if(temp+60 < useBPM)
  { 
    digitalWrite(10,HIGH); 
    digitalWrite(9,HIGH);
    digitalWrite(8,HIGH); 
    digitalWrite(7,HIGH); 
    
  }  
  else if(temp+45 < useBPM)
  {
    digitalWrite(10,LOW);
    digitalWrite(9,HIGH);
    digitalWrite(8,HIGH);
    digitalWrite(7,HIGH); 
  }
  else if(temp+30 < useBPM)
  {
    digitalWrite(10,LOW);
    digitalWrite(9,LOW);
    digitalWrite(8,HIGH); 
    digitalWrite(7,HIGH);
  }
  else if(temp+15 < useBPM)
  {
    digitalWrite(10,LOW);
    digitalWrite(9,LOW);
    digitalWrite(8,LOW);
    digitalWrite(7,HIGH);
  }  
}

void interruptSetup(){     
  // Initializes Timer2 to throw an interrupt every 2mS.
  TCCR2A = 0x02;     // DISABLE PWM ON DIGITAL PINS 3 AND 11, AND GO INTO CTC MODE
  TCCR2B = 0x06;     // DON'T FORCE COMPARE, 256 PRESCALER 
  OCR2A = 0X7C;      // SET THE TOP OF THE COUNT TO 124 FOR 500Hz SAMPLE RATE
  TIMSK2 = 0x02;     // ENABLE INTERRUPT ON MATCH BETWEEN TIMER2 AND OCR2A
  sei();             // MAKE SURE GLOBAL INTERRUPTS ARE ENABLED      
} 


// THIS IS THE TIMER 2 INTERRUPT SERVICE ROUTINE. 
// Timer 2 makes sure that we take a reading every 2 miliseconds
ISR(TIMER2_COMPA_vect){                         // triggered when Timer2 counts to 124
    cli();                                      // disable interrupts while we do this
    Signal = analogRead(pulsePin);              // read the Pulse Sensor 
    sampleCounter += 2;                         // keep track of the time in mS with this variable
    int N = sampleCounter - lastBeatTime;       // monitor the time since the last beat to avoid noise

//  find the peak and trough of the pulse wave
    if(Signal < thresh && N > (IBI/5)*3){       // avoid dichrotic noise by waiting 3/5 of last IBI
        if (Signal < T){                        // T is the trough
            T = Signal;                         // keep track of lowest point in pulse wave 
         }
       }
      
    if(Signal > thresh && Signal > P){          // thresh condition helps avoid noise
        P = Signal;                             // P is the peak
       }                                        // keep track of highest point in pulse wave
    
  //  NOW IT'S TIME TO LOOK FOR THE HEART BEAT
  // signal surges up in value every time there is a pulse
if (N > 250){                                   // avoid high frequency noise
  if ( (Signal > thresh) && (Pulse == false) && (N > (IBI/5)*3) ){        
    Pulse = true;                               // set the Pulse flag when we think there is a pulse
    IBI = sampleCounter - lastBeatTime;         // measure time between beats in mS
    lastBeatTime = sampleCounter;               // keep track of time for next pulse
         if(firstBeat){                         // if it's the first time we found a beat, if firstBeat == TRUE
             firstBeat = false;                 // clear firstBeat flag
             return;                            // IBI value is unreliable so discard it
            }   
         if(secondBeat){                        // if this is the second beat, if secondBeat == TRUE
            secondBeat = false;                 // clear secondBeat flag
               for(int i=0; i<=9; i++){         // seed the running total to get a realisitic BPM at startup
                    rate[i] = IBI;                      
                    }
            }
          
    // keep a running total of the last 10 IBI values
    word runningTotal = 0;                   // clear the runningTotal variable    

    for(int i=0; i<=8; i++){                // shift data in the rate array
          rate[i] = rate[i+1];              // and drop the oldest IBI value 
          runningTotal += rate[i];          // add up the 9 oldest IBI values
        }
    int average;  
    rate[9] = IBI;                          // add the latest IBI to the rate array
    runningTotal += rate[9];                // add the latest IBI to runningTotal
    runningTotal /= 10;                     // average the last 10 IBI values 
    BPM = 60000/runningTotal;               // how many beats can fit into a minute? that's BPM!
    if(lastIterator == 99){
      lastHundredBPM[99] = BPM;
      lastIterator=0;
    }
    else{
      lastIterator++;
      lastHundredBPM[lastIterator] = BPM;
    }
    for(int i=0; i<100; i++)
    {
      average += lastHundredBPM[i];
    }
    if(state == HIGH){
        useBPM = average/100;}
    else{
        useBPM = average/100;
        getupandSLAM();
    }
    QS = true;                              // set Quantified Self flag 
    // QS FLAG IS NOT CLEARED INSIDE THIS ISR
    }                       
}

  if (Signal < thresh && Pulse == true){     // when the values are going down, the beat is over
      Pulse = false;                         // reset the Pulse flag so we can do it again
      amp = P - T;                           // get amplitude of the pulse wave
      thresh = amp/2 + T;                    // set thresh at 50% of the amplitude
      P = thresh;                            // reset these for next time
      T = thresh;
     }
  
  if (N > 2500){                             // if 2.5 seconds go by without a beat
      thresh = 512;                          // set thresh default
      P = 512;                               // set P default
      T = 512;                               // set T default
      lastBeatTime = sampleCounter;          // bring the lastBeatTime up to date        
      firstBeat = true;                      // set these to avoid noise
      secondBeat = true;                     // when we get the heartbeat back
     }
  
  sei();                                     // enable interrupts when youre done!
}// end isr


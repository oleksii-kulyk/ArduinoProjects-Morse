#include "Arduino.h"
#include <ArduinoSTL.h>
#include <map>
#include <SevenSegmentDisplay.h> https://github.com/alikabeel/Letters-and-Numbers-Seven-Segment-Display-Library

#define DEBUG 1

const byte MORSE_IN = 2; // pin for reading dots and dashes
const byte MORSE_OUT = 22; // pin for outputting DOTs and DASHes as HIGH logic pulses
unsigned long volatile HIGH_TIME_START, HIGH_TIME_STOP;
unsigned long volatile LOW_TIME_START, LOW_TIME_STOP;

class MORSE_CODE{
  public:                                                              //TODO   Write an explicit constructor for the MORSE class. Initialize the Display7Seg class in it.
    uint8_t _word_len = 16;         //all Morse words are assumed to be no longer than 16 letters
    uint16_t _tone_freq = 380;      //buzzer tone frequency in Hz

    // 7 Segment Display pinout
    const byte LCD_7_A = 49; const byte LCD_7_B = 47; const byte LCD_7_C = 48; const byte LCD_7_D = 50;
    const byte LCD_7_E = 52; const byte LCD_7_F = 51; const byte LCD_7_G = 53; const byte LCD_7_DP = 46;

    MORSE_CODE(){}

//Read a MORSE WORD by repetedly calling `_readLetter()`
//Longest word assumed to be no longer than 16 letters
    String readWord(byte MORSE_IN){
      String word0;
      word0.reserve(_word_len);

      while( !_pauseGreaterThan(_GAP * 12) ){ //if long LOW detected, stop accepting input
        char new_letter;
        new_letter = _readLetter(MORSE_IN);
        word0 += new_letter;
      }
      
      return word0;
    }

//Sound out the MORSE word from supplied text by repeatedly loooking `_writeLetter()`
    void writeWord(byte MORSE_OUT, String word0){
      word0.toUpperCase();

      for (int i = 0; i < word0.length(); i++){
        _writeLetter(MORSE_OUT, word0[i]);
      }
    }

//Interrupt Service Routine triggered by the MORSE button on the circuit
    void static isrTimePulse(){
      if(digitalRead(MORSE_IN) == HIGH){
        HIGH_TIME_START = millis();
        LOW_TIME_STOP = millis();
      }
      else{
        HIGH_TIME_STOP = millis();
        LOW_TIME_START = millis();
      }
    }

  private:
    uint16_t const _DASH = 750;    //minimum duration in ms for a HIGH pulse to count as a DASH
    uint16_t const _DOT = 250;      //duration in ms for a HIGH pulse to count as a DOT
    uint16_t const _GAP = _DOT * 2; //length of the delay
    uint8_t  const _letter_len = 5; //all Morse letters are assumed to be no longer than 5 marks
    
    std::map<uint16_t, char> const _LETTERS = { //DASH is represented as 2, DOT is 1, PAUSE is 0
      {12000, 'A'}, {21110, 'B'}, {21210, 'C'}, {21100, 'D'}, {10000, 'E'}, {11210, 'F'}, {22100, 'G'}, {11110, 'H'}, {11000, 'I'},
      {12220, 'J'}, {21200, 'K'}, {12110, 'L'}, {22000, 'M'}, {21000, 'N'}, {22200, 'O'}, {12210, 'P'}, {22120, 'Q'}, {12100, 'R'},
      {11100, 'S'}, {20000, 'T'}, {11200, 'U'}, {11120, 'V'}, {12200, 'W'}, {21120, 'X'}, {21220, 'Y'}, {22110, 'Z'}, {12222, '1'},
      {11222, '2'}, {11122, '3'}, {11112, '4'}, {11111, '5'}, {21111, '6'}, {22111, '7'}, {22211, '8'}, {22221, '9'}, {22222, '0'}
    };


  // Extension of the SevenSegmentDisplay library included above. Adding a few useful functions.
  class Display7Seg: SevenSegmentDisplay{
    private:
  //Delay function. Do nothing for the duration of `delayTime` in ms
      void _delay(unsigned long delayTime){
       unsigned long previousMillis, currentMillis;
        previousMillis = currentMillis = millis();
        while((currentMillis - previousMillis) < delayTime){
          currentMillis = millis();
       }
      }

    public:

      Display7Seg(){}
  //diplaying a non-alphanumeric char leads to clearing the display due to how the base method is implemented
      void clearDisplay(){
       SevenSegmentDisplay::displayCharacter('$');
     }

      void flashDOT(unsigned long delayTime){
       SevenSegmentDisplay::displayDecimalPoint(true);
       _delay(delayTime);
       SevenSegmentDisplay::displayDecimalPoint(false);
     }

      void flashDASH(unsigned long delayTime){
       digitalWrite(LCD_7_D, HIGH);
       _delay(delayTime);
       digitalWrite(LCD_7_D, LOW);
     }

}display7Seg;


//Delay function. Do nothing for the duration of `delayTime` in ms
    void _delay(unsigned long delayTime){
      unsigned long previousMillis, currentMillis;
      previousMillis = currentMillis = millis();
      while((currentMillis - previousMillis) < delayTime){
        currentMillis = millis();
      }
    }

//Pads the input letter with zeros if a sufficient input delay between the letters has elapsed
    uint16_t _padWithZeros(uint16_t letter, uint8_t letter_len){
      for(int i = 0; i < letter_len; i++){
        letter *= 10;
      }

      return letter;
    }

//Desides whether a HIGH pulse is a DASH or a DOT
//Resets the timer variables at the start, `HIGH_TIME_STOP` is later set by an ISR tied to the button
    uint8_t _readMark(){
      HIGH_TIME_START = HIGH_TIME_STOP = millis(); //reset the high pulse timer
      while(digitalRead(MORSE_IN) == HIGH){
        #if DEBUG
          Serial.println(millis());
          Serial.println(digitalRead(MORSE_IN));
        #endif
      }
      #if DEBUG
        Serial.print("HIGH_TIME_START: "); Serial.println(HIGH_TIME_START);
        Serial.print("HIGH_TIME_STOP: "); Serial.println(HIGH_TIME_STOP);
        Serial.print("LOW_TIME_START: "); Serial.println(LOW_TIME_START);
        Serial.print("LOW_TIME_STOP: "); Serial.println(LOW_TIME_STOP);
      #endif
      
      unsigned long pulse_len = (HIGH_TIME_STOP - HIGH_TIME_START);
      uint8_t mark = (pulse_len >= _DASH) ? 2 : 1;
      if (mark == 2){
        display7Seg.flashDASH(_DASH); // a method inherited form Display7Seg
      }
      return;
    }

//Desides whether a LOW pulse is a long enough pause for distingushing the gaps between MORSE chars and awaiting end of input
//Resets the timer variables at the start, `LOW_TIME_STOP` is later set by an ISR tied to the button
    bool _pauseGreaterThan(unsigned long pauseLength){
      LOW_TIME_START = LOW_TIME_STOP = millis(); //reset the low pulse timer
      while(digitalRead(MORSE_IN) == LOW){  
      }

      return ((LOW_TIME_STOP - LOW_TIME_START) > pauseLength) ? true : false;
    }

//Read an individual MORSE LETTER by timing individual pulses
//all Morse letters are assumed to be no longer than 5 marks
    char _readLetter(byte MORSE_IN){
      uint16_t letter = 0;
      uint32_t pulse_len = 0;        //to store duration of a logic pulse in ms
      uint8_t letter_len = _letter_len - 1;  //fix off by one kinda situation for use in formulas
      #if DEBUG
        Serial.println("Reading Letter");
      #endif
      while(digitalRead(MORSE_IN) == LOW){    //wait for input
        #if DEBUG
          Serial.println("Waiting for Input"); Serial.println(digitalRead(MORSE_IN));
        #endif
        }

      while(letter_len > 0){
        if(digitalRead(MORSE_IN) == HIGH){
          letter += _readMark();
          letter *= 10;
          letter_len--;
        }

        else{
          if(_pauseGreaterThan(_GAP * 3)){
            letter = _padWithZeros(letter, letter_len);
            break;
          }
        }
      }

      for(auto it: _LETTERS) {
        if(it.first == letter) {
          return it.second;
        }
      }
      return '-';
    }

//Sound out a single character by looking up the MORSE code using `letter` as key
    void _writeLetter(byte MORSE_OUT, char letter){
      uint16_t marks;

      //lookup MORSE key by letter O(n) traversal
      for(auto it : _LETTERS) { 
        if(it.second == letter) { 
          marks = it.first;
        }
      }

      uint16_t order = 10000;
      for(int i = 5; i > 0; i--){
        uint8_t mark = ( marks / order ) % 10;

        if(mark == 1) {tone(MORSE_OUT, _tone_freq, _DOT); delay(_GAP);}
        else if(mark == 2) {tone(MORSE_OUT, _tone_freq, _DASH); delay(_GAP);} //TODO SOMETHING WRONG IN THESE TONES, THE DELAY SEEMS TO BE GETTING CUT OFF BY THE `tone()` FUNCTION
        else if(mark == 0) {delay(_GAP * 3); break;}

        order /= 10;
      }
    }

} morse;


void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(MORSE_IN, INPUT);
  attachInterrupt(digitalPinToInterrupt(MORSE_IN), morse.isrTimePulse, CHANGE);
  
  morse.Display7Seg(LCD_7_A, LCD_7_B, LCD_7_C, LCD_7_D, LCD_7_E, LCD_7_F, LCD_7_G, LCD_7_DP, false);
  morse.Display7Seg.testDisplay();

  Serial.begin(9600);
}

void loop() {
  morse.writeWord(LED_BUILTIN, "ON");
  morse.writeWord(MORSE_OUT, "ON");
  
  String message;
  message.reserve(morse._word_len);
  message = morse.readWord(MORSE_IN);
  Serial.print("Message: ");
  Serial.println(message);
  morse.writeWord(MORSE_OUT, message);

}
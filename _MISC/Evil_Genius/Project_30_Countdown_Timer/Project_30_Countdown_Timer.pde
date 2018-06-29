// Project 30 - Countdown Timer

#include <EEPROM.h>

int segmentPins[] = {3, 2, 19, 16, 18, 4, 5, 17}; 
int displayPins[] = {14, 7, 15, 6};
int times[] = {5, 10, 15, 20, 30, 45, 100, 130, 200, 230, 300, 400, 500, 600, 700, 800, 900, 1000, 1500, 2000, 3000};
int numTimes = 19;
byte selectedTimeIndex;
int timerMinute;
int timerSecond;

int buzzerPin = 11;
int aPin = 8;
int bPin = 10;
int buttonPin = 9;

boolean stopped = true;


byte digits[10][8] = {
//  a  b  c  d  e  f  g  .
  { 1, 1, 1, 1, 1, 1, 0, 0},  // 0
  { 0, 1, 1, 0, 0, 0, 0, 0},  // 1
  { 1, 1, 0, 1, 1, 0, 1, 0},  // 2
  { 1, 1, 1, 1, 0, 0, 1, 0},  // 3
  { 0, 1, 1, 0, 0, 1, 1, 0},  // 4
  { 1, 0, 1, 1, 0, 1, 1, 0},  // 5
  { 1, 0, 1, 1, 1, 1, 1, 0},  // 6
  { 1, 1, 1, 0, 0, 0, 0, 0},  // 7
  { 1, 1, 1, 1, 1, 1, 1, 0},  // 8  
  { 1, 1, 1, 1, 0, 1, 1, 0}  // 9  
};
   
void setup()
{
  for (int i=0; i < 8; i++)
  {
    pinMode(segmentPins[i], OUTPUT);
  }
  for (int i=0; i < 4; i++)
  {
    pinMode(displayPins[i], OUTPUT);
  }
  pinMode(buzzerPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  pinMode(aPin, INPUT);  
  pinMode(bPin, INPUT);
  selectedTimeIndex = EEPROM.read(0);
  timerMinute = times[selectedTimeIndex] / 100;
  timerSecond = times[selectedTimeIndex] % 100;
}

void loop()
{
  if (digitalRead(buttonPin))
  {
    stopped = ! stopped;
    digitalWrite(buzzerPin, LOW); 
    while (digitalRead(buttonPin)) {};
    EEPROM.write(0, selectedTimeIndex);
  }
  updateDisplay();
}

void updateDisplay() // mmss
{
  int minsecs = timerMinute * 100 + timerSecond;
  int v = minsecs;
  for (int i = 0; i < 4; i ++)
  {
      int digit = v % 10;
      setDigit(i);
      setSegments(digit);
      v = v / 10;
      process();
  }
  setDigit(5); // all digits off to prevent uneven illumination
}

void process()
{
  for (int i = 0; i < 100; i++) // tweak this number between flicker and blur
  {
    int change = getEncoderTurn();
    if (stopped)
    {
      changeSetTime(change);
    }
    else
    {
      updateCountingTime();
    }
  }
  if (timerMinute == 0 && timerSecond == 0)
  {
    digitalWrite(buzzerPin, HIGH); 
  }

}

void changeSetTime(int change)
{
   selectedTimeIndex += change;
   if (selectedTimeIndex < 0)
   {
     selectedTimeIndex = numTimes;
   }
   else if (selectedTimeIndex > numTimes)
   {
     selectedTimeIndex = 0;
   }
   timerMinute = times[selectedTimeIndex] / 100;
   timerSecond = times[selectedTimeIndex] % 100;
}

void updateCountingTime()
{
   static unsigned long lastMillis;
   unsigned long m = millis();
   if (m > (lastMillis + 1000) && (timerSecond > 0 || timerMinute > 0))
   {
    digitalWrite(buzzerPin, HIGH); 
    delay(10);
    digitalWrite(buzzerPin, LOW); 

    if (timerSecond == 0)
    { 
       timerSecond = 59;
       timerMinute --;
    }
    else
    {
       timerSecond --; 
    }
    lastMillis = m;
  }
}

void setDigit(int digit)
{
  for (int i = 0; i < 4; i++)
  {
    digitalWrite(displayPins[i], (digit != i));
  } 
}

void setSegments(int n)
{
  for (int i = 0; i < 8; i++)
  {
    digitalWrite(segmentPins[i], ! digits[n][i]);
  } 
}


int getEncoderTurn()
{
  // return -1, 0, or +1
  static int oldA = LOW;
  static int oldB = LOW;
  int result = 0;
  int newA = digitalRead(aPin);
  int newB = digitalRead(bPin);
  if (newA != oldA || newB != oldB)
  {
    // something has changed
    if (oldA == LOW && newA == HIGH)
    {
      result = -(oldB * 2 - 1);
    }
  }
  oldA = newA;
  oldB = newB;
  return result;
} 


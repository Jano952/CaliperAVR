#include <avr/interrupt.h>

#define READING_DISTANCE_MS   20

typedef struct {
        uint32_t      LastEdge;
        uint32_t      UpdatePeriod;
        __int24       Reading[2]; 
        uint8_t       Valid:1,
                      Raw:1,
                      Absolute:1,
                      Continous:1,
                      Supress;
} caliper_t;

volatile caliper_t caliper = {0} ;

//Interrupt code of the comparator
ISR(ANA_COMP_vect) {
    uint8_t calip_timeout=0;
    uint8_t relsel=0;
    caliper.Valid = 0; //reset
    register __int24     _Reading;

    ACSR |= ( (1<<ACIS1)  | (1<<ACIS0) ); //change edge interrupt
    __asm__ __volatile__ ("nop\n\t");
    ACSR |= (1<<ACI); 
    
    for(relsel = 0; relsel<2;relsel++)
    {
      for(int i =0; i<24;i++)
      {
        ADMUX = 0x00;               //Back to ADC0 input      
        __asm__ __volatile__ ("nop\n\t");
        ACSR |= (1<<ACI); 
        while(! (ACSR & (1<<ACI)))
          {
            calip_timeout++;
            __asm__ __volatile__ ("nop\n\t");
            __asm__ __volatile__ ("nop\n\t");       
            if(!calip_timeout)
            {
              ACSR &= ~(1<<ACIS0); //change edge interrupt
              __asm__ __volatile__ ("nop\n\t");
              ACSR |= (1<<ACI); 
              return;               //Data didnt come, return and try again later
            }
          }   
  
        ADMUX = 0x01;               //Set ADC1 input
        __asm__ __volatile__ ("nop\n\t");
        __asm__ __volatile__ ("nop\n\t");
        __asm__ __volatile__ ("nop\n\t");
        __asm__ __volatile__ ("nop\n\t");     //SYNC                
        PORTB |= 0x01;  // XXXXXXXX | 00100000 = XX1XXXXX
        if ( (ACSR & (1 << ACO)))
        {
          _Reading = _Reading>>1;  
          _Reading &= 0x7FFFFF;
          PORTB &= ~0x02;  
        }
        else
        {
          _Reading = _Reading>>1;
          _Reading |= 0x800000;
          PORTB |= 0x02;   
        }
        
        calip_timeout=0;      
  
          PORTB &= ~0x01;  // 00100000 -> 11011111 & XXXXXXXX = XX0XXXXX  
      }
      caliper.Reading[relsel] = _Reading;
    }

    ADMUX = 0x00;               //Back to ADC0 input      
    __asm__ __volatile__ ("nop\n\t");
    ACSR |= (1<<ACI); 
    while(! (ACSR & (1<<ACI)))
    {
      calip_timeout++;
      __asm__ __volatile__ ("nop\n\t");
      __asm__ __volatile__ ("nop\n\t");
      if(!calip_timeout)
      {
        ACSR &= ~(1<<ACIS0); //change edge interrupt
        __asm__ __volatile__ ("nop\n\t");
        ACSR |= (1<<ACI); 
        return;               //Data didnt come, return and try again later
      }
    }       
    caliper.Valid = 1;      //Last Edge arrived, Reading is valid !             
    ACSR &= ~(1<<ACIS0); //change edge interrupt
    __asm__ __volatile__ ("nop\n\t");
    ACSR |= (1<<ACI); 
    caliper.LastEdge = millis();

}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
 
    pinMode(8, OUTPUT);
    pinMode(9, OUTPUT);
    
  ADCSRA &= ~(1<<ADEN);       //Disable ADC
  SFIOR |= (1<<ACME);   //Multiplexer to Comparator
  ADMUX = 0x00;               //ADC0 input
  ACSR = ((1<<ACBG) | (1<<ACIS1)  ); //Enable interrupt, falling edge (Rising from caliper), BandGap ref 
 
  delay(1000);
  Serial.println("Init done");
  ACSR |= (1<<ACIE);
  caliper.UpdatePeriod = 1000;
}

uint32_t  LastUpdate=0;
String LastUpdate_s;

void loop() {
  // put your main code here, to run repeatedly:

  if(Serial.available())
  {
    switch(Serial.read())
    {
      case  'r':
        Serial.println("Raw mode toggled!");
        caliper.Raw ^= 1;
        break;
      case  'a':
        Serial.println("Abs mode toggled!");
        caliper.Absolute ^=  1;
        break;
      case  'c':
        Serial.println("Continous mode toggled!");
        caliper.Continous ^=  1;
        break;  
      case  's':
        Serial.println("Timeout suppresion toggled!");
        caliper.Supress ^=  1;
        break;   
      case  'p':
        caliper.UpdatePeriod =  Serial.parseInt(SKIP_NONE);        
        Serial.print("Loaded period ");
        Serial.println(caliper.UpdatePeriod);
        break;             
      case '\r':
      case '\n':
        break;  
      case 'h':        
      default:
        Serial.println("Commands supported: r - toggle Raw and mm reading,  a - toggle Absolute and Relative recieving, c - toggle Continuous mode (send only changes or send periodicaly), pXXXXX - Continous mode update period in ms, s - Supress Timeouts ");
    }  
  }

  if( (millis() - caliper.LastEdge) >500)
  {
    caliper.Valid=0;
    if(!caliper.Supress)
    {
      Serial.println("Timeout!");
      delay(500);
    }
  }

  bool sendUpdate = false;
  String sendUpdate_s ;
  

  if(caliper.Valid)
  {
    if(caliper.Continous && ((millis() - LastUpdate ) > caliper.UpdatePeriod ))
    {
      LastUpdate = millis();
      sendUpdate_s = caliperToStr();
      sendUpdate = true;
      
    }
    else if (!caliper.Continous)
    {
      sendUpdate_s = caliperToStr();
      if(sendUpdate_s != LastUpdate_s)
      {
        sendUpdate = true;  
        LastUpdate_s = sendUpdate_s;
      }
    }
    if(sendUpdate)
      Serial.println(sendUpdate_s);
    
/*    Serial.print("ABS:");   
    Serial.print((int32_t)caliper.Reading[0]);
    Serial.print("[-] / REL:");
    Serial.print(((double)caliper.Reading[1]/806.3));
    Serial.println("[mm]");*/
  }
 //delay(100);
 
}

String caliperToStr()
{
  if(caliper.Raw && caliper.Absolute)
    return String("ABSRAW:") + String((uint32_t)caliper.Reading[0]);    
  else if(caliper.Raw && !caliper.Absolute)
    return String("RELRAW:") + String((uint32_t)caliper.Reading[1]);    
  else  if(!caliper.Raw && caliper.Absolute)
    return String("ABS:") + String((double)caliper.Reading[0]/806.3);    
  else
    return String("REL:") + String((double)caliper.Reading[1]/806.3);    
}

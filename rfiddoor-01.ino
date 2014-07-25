/* Arduino RC522 RFID Door Unlocker
 * 
 * Credits
 *  
 * Idea based on Brett Martin's project
 * http://www.instructables.com/id/Arduino-RFID-Door-Lock/
 * www.pcmofo.com
 *
 * MFRC522 Library from miguelbalboa
 * https://github.com/miguelbalboa/rfid
 * 
 * Arduino Forum Member luisilva for His Great Help
 * http://forum.arduino.cc/index.php?topic=257036.0
 * http://forum.arduino.cc/index.php?action=profile;u=198897
 *
 * I have not personally asked for permission to use code from them
 * or how to license this work.
 * 
 * "PICC" short for Proximity Integrated Circuit Card (RFID Tags)
 */
 
#include <EEPROM.h>  // We are going to read and write PICC's UID from/to EEPROM
#include <SPI.h>      // RC522 Module uses SPI protocol
#include <MFRC522.h>   // Library for Mifare RC522 Devices

// #include <Servo.h>

/* Servos can lock and unlock door locks
 * There are examples out there
 * May be we want to use a servo
 */


/* For visualizing whats going on hardware
 * we need some leds and 
 * to control door lock a relay 
 * (or some other hardware) 
 * 
 */
 
#define redLed 5
#define greenLed 8
#define blueLed 7
#define relay 6

//define buzzer ? maybe we want that

/* We need to define some boolean ???
 * and string ??? bytes ??
 *
 * I think it is not secure to only use PICC's UID
 * to verify PICC's user who wants to unlock a door.
 *
 * MFRC522 Library also let us to use some authentication
 * mechanism, writing blocks and reading back
 * and there is great example piece of code
 * about reading and writing PICCs
 * here > http://makecourse.weebly.com/week10segment1.html
 *
 * If you serious about coding and security
 * you should really check Mıfare's datasheet
 * We are going to use completely INSECURE way to
 * Unlock a door. 
 * 
 * Also there are always security
 * issues if there is a "LOCK" actually.
 *
 */

boolean match = false; // initialize card match to false
boolean programMode = false;

byte storedCard[6];   // Stores an ID read from EEPROM
byte readCard[6];           // We are going to store scanned PICC's UID

byte masterCard[6] = {0x47,0x9c,0x85,0xb5}; // Define master PICC's UID


/* We need to define MFRC522's pins and create instance
 * These pins for Uno, look MFRC522 Library for
 * pin configuration for other Arduinos.
 */
 
#define SS_PIN 10
#define RST_PIN 9
MFRC522 mfrc522(SS_PIN, RST_PIN);	// Create MFRC522 instance.


///////////////////////////////////////// Setup ///////////////////////////////////
void setup() {
  
  /* 
   * Arduino Pin Configuration
   */
  pinMode(redLed, OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(blueLed, OUTPUT);
  pinMode(relay, OUTPUT);
  
  /*
   * Protocol Configuration
   */
  Serial.begin(9600);	 // Initialize serial communications with PC
  SPI.begin();           // MFRC522 Hardware uses SPI protocol
  mfrc522.PCD_Init();    // Initialize MFRC522 Hardware
  Serial.println("##### RFID Door Unlocker #####");
  Serial.println("");
  Serial.println("Waiting PICCs to bo scanned :)"); 
}


///////////////////////////////////////// Main Loop ///////////////////////////////////

void loop ()
{
   int successRead;

   // this block is only to show to the user the mode
   if (programMode) {
      programModeOn(); // Program Mode cycles through RGB waiting to read a new card      
   }
   else {
      normalModeOn(); // Normal mode, blue Power LED is on, all others are off
   }
   
   do {
      successRead = getID(); // Get the ID, sets readCard = to the read ID
   } while (!successRead); //the program will not go further while you not get a successful read
   
   if (programMode) {
      programMode = false;  // next time will enter in normal mode
      if ( findID(readCard) )
      {
        Serial.println("I know this PICC, so removing");
        deleteModeOn();
        delay(1000);
        deleteID(readCard);
        Serial.println("Removed- Exiting Program Mode");
      }
      else
      {
        Serial.println("I do not know this PICC, adding...");
        writeID(readCard);
        Serial.println("Added - Exiting Program Mode");
      }
   }
   else {  // Normal MODE
      if ( isMaster(readCard) ) {
         programMode = true;
         Serial.println("Hello Master - Entered Program Mode");
         Serial.println("Scan a PICC to add or remove to EEPROM");
      }
      else {
         
         if ( findID(readCard) ) // If not, see if the card is in the EEPROM
         {
            Serial.println("Welcome, You shall pass");
            openDoor(2); // If it is, open the door lock
         }
         else
         {
            Serial.println("I do not know you, go away");
            failed(); // If not, show that the ID was not valid
         }
      }
   }
   
}   


///////////////////////////////////////// Get PICC's UID ///////////////////////////////////
int getID() {
  
  /* 
   * Getting ready for Reading PICCs
   */
   
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
		return 0;
	}
  
  if ( ! mfrc522.PICC_ReadCardSerial()) {
		return 0;
	}
	
  /*
   *  Now we are ready to read PICCs
   */
    

    Serial.println("Scanned PICC's UID:");
    for (byte i = 0; i < mfrc522.uid.size; i++) {
    	readCard[i] = mfrc522.uid.uidByte[i];
        Serial.print(readCard[i], HEX);		
    }
    Serial.println("");
    
    mfrc522.PICC_HaltA();
    return 1;
  
}




///////////////////////////////////////// Program Mode Leds ///////////////////////////////////
void programModeOn() {
  
  digitalWrite(redLed, LOW); // Make sure blue LED is off
  digitalWrite(greenLed, LOW); // Make sure blue LED is off
  digitalWrite(blueLed, HIGH); // Make sure green LED is on
  delay(200);
  
  digitalWrite(redLed, LOW); // Make sure blue LED is off
  digitalWrite(greenLed, HIGH); // Make sure blue LED is on
  digitalWrite(blueLed, LOW); // Make sure green LED is off
  delay(200);
  
  digitalWrite(redLed, HIGH); // Make sure blue LED is on
  digitalWrite(greenLed, LOW); // Make sure blue LED is off
  digitalWrite(blueLed, LOW); // Make sure green LED is off
  delay(200);

}

///////////////////////////////////////// Normal Mode Leds  ///////////////////////////////////
void normalModeOn () {

  digitalWrite(blueLed, HIGH); // Power pin ON and ready to read card
  digitalWrite(redLed, LOW); // Make sure Green LED is off
  digitalWrite(greenLed, LOW); // Make sure Red LED is off
  digitalWrite(relay, LOW); // Make sure Door is Locked 
  
}  


//////////////////////////////////////// Read an ID from EEPROM //////////////////////////////
void readID( int number ) {

  int start = (number * 5 ) - 4; // Figure out starting position
  //Serial.print("Start: ");
  //Serial.print(start);
  //Serial.print("\n\n");
   for ( int i = 0; i < 5; i++ ) // Loop 5 times to get the 5 Bytes
    {
     storedCard[i] = EEPROM.read(start+i); // Assign values read from EEPROM to array
     /*
      Serial.print("Read [");
      Serial.print(start+i);
      Serial.print("] [");
      Serial.print(storedCard[i], HEX);
      Serial.print("] \n");
     */
    }
}


///////////////////////////////////////// Add UID to EEPROM   ///////////////////////////////////
void writeID( byte a[] ) {
    
	if ( !findID( a ) ) // Before we write to the EEPROM, check to see if we have seen this card before!
       {
         int num = EEPROM.read(0); // Get the numer of used spaces, position 0 stores the number of ID cards
          /* 
          Serial.print("Num: ");
          Serial.print(num);
          Serial.print(" \n");
          */
          int start = ( num * 5 ) + 1; // Figure out where the next slot starts
          num++; // Increment the counter by one
          EEPROM.write( 0, num ); // Write the new count to the counter
          for ( int j = 0; j < 5; j++ ) // Loop 5 times
            {
              EEPROM.write( start+j, a[j] ); // Write the array values to EEPROM in the right position
              /*
              Serial.print("W[");
              Serial.print(start+j);
              Serial.print("] Value [");
              Serial.print(a[j], HEX);
              Serial.print("] \n");
              */
            }
			successWrite();
        } 
    else
        {
          failedWrite();
        }

}

///////////////////////////////////////// Remove ID from EEPROM   ///////////////////////////////////
void deleteID( byte a[] ) {
    if ( !findID( a ) ) // Before we delete from the EEPROM, check to see if we have this card!
        {
           failedWrite(); // If not
        }
    else 
        {
          int num = EEPROM.read(0); // Get the numer of used spaces, position 0 stores the number of ID cards
          int slot; // Figure out the slot number of the card
          int start;// = ( num * 5 ) + 1; // Figure out where the next slot starts
          int looping; // The number of times the loop repeats
          int j;

          int count = EEPROM.read(0); // Read the first Byte of EEPROM that
          // Serial.print("Count: "); // stores the number of ID's in EEPROM
          // Serial.print(count);
          //Serial.print("\n");
          slot = findIDSLOT( a ); //Figure out the slot number of the card to delete
          start = (slot * 5) - 4;
          looping = ((num - slot) * 5);
          num--; // Decrement the counter by one
          EEPROM.write( 0, num ); // Write the new count to the counter

          for ( j = 0; j < looping; j++ ) // Loop the card shift times
            {
              EEPROM.write( start+j, EEPROM.read(start+5+j)); // Shift the array values to 5 places earlier in the EEPROM
              /*
              Serial.print("W[");
              Serial.print(start+j);
              Serial.print("] Value [");
              Serial.print(a[j], HEX);
              Serial.print("] \n");
              */
            }
          for ( int k = 0; k < 5; k++ ) //Shifting loop
            {
              EEPROM.write( start+j+k, 0);
            }
    successDelete();


        }

}

///////////////////////////////////////// Check Bytes   ///////////////////////////////////
boolean checkTwo ( byte a[], byte b[] ) {
  if ( a[0] != NULL ) // Make sure there is something in the array first
  match = true; // Assume they match at first

    for ( int k = 0; k < 5; k++ ) // Loop 5 times
       {
          /*
          Serial.print("[");
          Serial.print(k);
          Serial.print("] ReadCard [");
          Serial.print(a[k], HEX);
          Serial.print("] StoredCard [");
          Serial.print(b[k], HEX);
          Serial.print("] \n");
          */
          if ( a[k] != b[k] ) // IF a != b then set match = false, one fails, all fail
           match = false;
        }
  if ( match ) // Check to see if if match is still true
        {    
          //Serial.print("Strings Match! \n"); 
          return true; // Return true
        }
  else  {
          //Serial.print("Strings do not match \n"); 
         return false; // Return false
        }
}


///////////////////////////////////////// Find Slot   ///////////////////////////////////
int findIDSLOT( byte find[] ) {
	int count = EEPROM.read(0); // Read the first Byte of EEPROM that
	// Serial.print("Count: "); // stores the number of ID's in EEPROM
	// Serial.print(count);
	//Serial.print("\n");
		for ( int i = 1; i <= count; i++ ) // Loop once for each EEPROM entry
		{
			readID(i); // Read an ID from EEPROM, it is stored in storedCard[6]
				if( checkTwo( find, storedCard ) ) // Check to see if the storedCard read from EEPROM 
					{ // is the same as the find[] ID card passed		
						//Serial.print("We have a matched card!!! \n");
						return i; // The slot number of the card
						break; // Stop looking we found it
					} 
		}
	}

///////////////////////////////////////// Find ID From EEPROM   ///////////////////////////////////
boolean findID( byte find[] ) {
	int count = EEPROM.read(0); // Read the first Byte of EEPROM that
	// Serial.print("Count: "); // stores the number of ID's in EEPROM
	// Serial.print(count);
	//Serial.print("\n");
		for ( int i = 1; i <= count; i++ ) // Loop once for each EEPROM entry
			{
				readID(i); // Read an ID from EEPROM, it is stored in storedCard[6]
				if( checkTwo( find, storedCard ) ) // Check to see if the storedCard read from EEPROM 
			{ // is the same as the find[] ID card passed
				//Serial.print("We have a matched card!!! \n");
				return true;
				break; // Stop looking we found it
			}
				else // If not, return false
			{
				//Serial.print("No Match here.... \n");
			}			

			}
return false;
}

///////////////////////////////////////// Write Success to EEPROM   ///////////////////////////////////
// Flashes the green LED 3 times to indicate a successful write to EEPROM
void successWrite()
{
	Serial.end();
	
	digitalWrite(blueLed, LOW); // Make sure blue LED is off
	digitalWrite(redLed, LOW); // Make sure red LED is off
	digitalWrite(greenLed, LOW); // Make sure green LED is on
	delay(200);
	
	digitalWrite(greenLed, HIGH); // Make sure green LED is on
	delay(200);
	
	digitalWrite(greenLed, LOW); // Make sure green LED is off
	delay(200);
	
	digitalWrite(greenLed, HIGH); // Make sure green LED is on
	delay(200);
	
	digitalWrite(greenLed, LOW); // Make sure green LED is off
	delay(200);
	
	digitalWrite(greenLed, HIGH); // Make sure green LED is on
	delay(200);
	
	Serial.begin(9600);
}

///////////////////////////////////////// Write Failed to EEPROM   ///////////////////////////////////
// Flashes the red LED 3 times to indicate a failed write to EEPROM
void failedWrite()
{
	Serial.end();
	
	digitalWrite(blueLed, LOW); // Make sure blue LED is off
	digitalWrite(redLed, LOW); // Make sure red LED is on
	digitalWrite(greenLed, LOW); // Make sure green LED is off
	delay(200);
	
	digitalWrite(redLed, HIGH); // Make sure red LED is on
	delay(200);
	
	digitalWrite(redLed, LOW); // Make sure red LED is off
	delay(200);
	
	digitalWrite(redLed, HIGH); // Make sure red LED is on
	delay(200);
	
	digitalWrite(redLed, LOW); // Make sure red LED is off
	delay(200);
	
	digitalWrite(redLed, HIGH); // Make sure red LED is on
	delay(200);
	
	Serial.begin(9600); 
}

///////////////////////////////////////// Success Remove UID From EEPROM  ///////////////////////////////////
// Flashes the blue LED 3 times to indicate a success delete to EEPROM
void successDelete()
{
	Serial.end();
	
	digitalWrite(blueLed, LOW); // Make sure blue LED is off
	digitalWrite(redLed, LOW); // Make sure red LED is off
	digitalWrite(greenLed, LOW); // Make sure green LED is on
	delay(200);
	
	digitalWrite(blueLed, HIGH); // Make sure blue LED is off
	delay(200);
	
	digitalWrite(blueLed, LOW); // Make sure blue LED is off
	delay(200);
	
	digitalWrite(blueLed, HIGH); // Make sure blue LED is off
	delay(200);
	
	digitalWrite(blueLed, LOW); // Make sure blue LED is off
	delay(200);
	
	digitalWrite(blueLed, HIGH); // Make sure blue LED is off
	delay(200);
	
	Serial.begin(9600);
}
///////////////////////////////////////// Check readCard is masterCard   ///////////////////////////////////
boolean isMaster( byte test[] ) 
{
	if ( checkTwo( test, masterCard ) )
		return true;
		else
		return false;
} 

///////////////////////////////////////// Delete Mode Leds   ///////////////////////////////////
void deleteModeOn()
{
	digitalWrite(blueLed, LOW); // Make sure blue LED is off
	digitalWrite(redLed, HIGH); // Make sure red LED is on
	digitalWrite(greenLed, LOW); // Make sure green LED is off
	delay(200);
	
	digitalWrite(blueLed, HIGH); // Make sure blue LED is on
	digitalWrite(redLed, LOW); // Make sure red LED is off
	digitalWrite(greenLed, LOW); // Make sure green LED is off
	delay(200);
}

///////////////////////////////////////// Unlock Door   ///////////////////////////////////
void openDoor( int setDelay )
{

	setDelay *= 1000; // Sets delay in seconds

	Serial.end();

	digitalWrite(blueLed, LOW); // Turn off blue LED
	digitalWrite(redLed, LOW); // Turn off red LED
	digitalWrite(greenLed, HIGH); // Turn on green LED
	digitalWrite(relay, HIGH); // Unlock door!

	delay(setDelay); // Hold door lock open for 2 seconds

	digitalWrite(relay, LOW); // Relock door

	delay(setDelay); // Hold green LED om for 2 more seconds

	digitalWrite(greenLed, LOW);	// Turn off green LED
	
	Serial.begin(9600);
}

///////////////////////////////////////// Failed Access  ///////////////////////////////////
void failed()
{
	Serial.end();
	
	digitalWrite(greenLed, LOW); // Make sure green LED is off
	digitalWrite(blueLed, LOW); // Make sure blue LED is off
	
	// Blink red fail LED 3 times to indicate failed key
	digitalWrite(redLed, HIGH); // Turn on red LED
	delay(1200); 
	Serial.begin(9600);
}

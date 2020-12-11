  #include <Wire.h>
  #include "PN532_I2C.h"
  #include "PN532.h"
  #include "NfcAdapter.h"

  PN532_I2C pn532i2c(Wire);
  PN532 nfc(pn532i2c);
//#endif

static const int LIGHT_NUM_G = 12;
static const int LIGHT_NUM_R = 2;
static const int LIGHT_NUM_LOCK = 14;

static const uint8_t placeID = 14;
static int timer = 1;
static int firstTime = 0;

static const size_t A_ID_COUNT = 2, A_ID_LEN_MAX = 7, A_ID_S = A_ID_LEN_MAX + 1;
static uint8_t* idknown = new uint8_t[A_ID_COUNT * (A_ID_LEN_MAX + 1)];
/*Boolean array to simulate server responses
0 - Activation request
1 - Messure user&ID request
2 - Timeout
3 - Available*/
boolean servReq[] = { 0, 0, 0, 1 };
boolean firstTry = true;


void setup(void) {
  Serial.begin(115200);
  Serial.println("Hello!");
  pinMode(LIGHT_NUM_G, OUTPUT);
  pinMode(LIGHT_NUM_R, OUTPUT);
  pinMode(LIGHT_NUM_LOCK, OUTPUT);
  init_idknown();
  nfc.begin();
  
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

  nfc.setPassiveActivationRetries(0xFF);
  nfc.SAMConfig();
  Serial.println("Waiting for an ISO14443A card");
}

// fill user id array
void init_idknown()
{
  memset(idknown, 0, A_ID_COUNT * (A_ID_LEN_MAX + 1));
  int i = 0, S = A_ID_S;
  idknown[i*S + 0] = 4;
  idknown[i*S + 1] = 186; idknown[i*S + 2] = 62; idknown[i*S + 3] = 221; idknown[i*S + 4] = 133;
  i = 1;
  idknown[i*S + 0] = 4;
  idknown[i*S + 1] = 217; idknown[i*S + 2] = 80; idknown[i*S + 3] = 3; idknown[i*S + 4] = 229;
}



void activWorkplace()
{
  Serial.println("Workplace a lock");
  digitalWrite(LIGHT_NUM_G, HIGH);
  digitalWrite(LIGHT_NUM_LOCK, HIGH);
  delay(1000);
  addTime();
  //digitalWrite(LIGHT_NUM_LOCK, LOW);
}

boolean checkActivationRequest()
{
  return servReq[0];
}

void checkTimeRequest()
{
  if(timer==firstTime)
  {
    Serial.println(firstTime);
    Serial.println(" seconds left");
    delay(5);
    Serial.println("Time out");
    deactivate();
  }
  else
  {
    delay(5);
    Serial.println(firstTime);
    Serial.println(" seconds left");
    
  }
}

void addTime ()
{
  firstTime+=4;
  Serial.println("Time added!");
}

boolean checkavailability ()
{
  return   servReq[3];
}

void deactivate()
{
    delay(1000);
    Serial.print("Fail");
    digitalWrite(LIGHT_NUM_R, HIGH);
    delay(1000);
    digitalWrite(LIGHT_NUM_R, LOW);
    delay(500);
    digitalWrite(LIGHT_NUM_R, HIGH);
    delay(500);
    digitalWrite(LIGHT_NUM_R, LOW);
    digitalWrite(LIGHT_NUM_G, LOW);
    digitalWrite(LIGHT_NUM_LOCK, LOW);
    firstTry = true;
}

void userIDcheck(uint8_t *uid)
{
    uint8_t *bookID = new uint8_t[4];
    bookID[0] = 186;
    bookID[1] = 62;
    bookID[2] = 221;
    bookID[3] = 133;
    if (memcmp (bookID, uid, 4) == 0)
    {
      activWorkplace();
      delete [] bookID;
    }
    else
    {
      deactivate();
      delete [] bookID;
    } 
}

void loop(void) {
  
  boolean success = false;
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  if (checkActivationRequest()!= false)
  {
    activWorkplace();
  }
  else
  {
      success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &uid[0], &uidLength);
  }
  if (success != false & firstTry == true)
  {
    // log
    Serial.println("\nFound a card!");
    Serial.print("UID Length: "); Serial.print(uidLength, DEC); Serial.println(" bytes");
    Serial.print("UID Value: ");
    for (uint8_t i = 0; i < uidLength; i++)
    { Serial.print(" "); Serial.print(uid[i]); }
    Serial.println("");
    firstTry = false;
    success = false;
    
    userIDcheck(uid);
  } 
   
  else if (success != false & firstTry == false)
   {
    if(checkavailability()!=false)
     {
           uint8_t *bookID = new uint8_t[4];
           bookID[0] = 186;
           bookID[1] = 62;
           bookID[2] = 221;
           bookID[3] = 133;
           if (memcmp (bookID, uid, 4) == 0)
          {
             addTime();
             delete [] bookID;
           }
           else
           {
             delete [] bookID;
           }
           success = false;
           delay(1000);
     }  
   }
  else if (firstTime >= 1)
   {
      checkTimeRequest();
      --firstTime;
   }
 else
  {
    delay(1000);
    // PN532 probably timed out waiting for a card
    Serial.println("\nTimed out waiting for a card");
  }
}

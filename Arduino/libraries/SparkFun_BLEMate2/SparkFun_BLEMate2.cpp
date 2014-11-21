/****************************************************************
Core management functions for BC118 modules.

Based on the code I developed for the BC127.

This code is beerware; if you use it, please buy me (or any other
SparkFun employee) a cold beverage next time you run into one of
us at the local.

30 Oct 2014- Mike Hord, SparkFun Electronics

Code developed in Arduino 1.0.6, on an Arduino Pro 5V.
****************************************************************/

#include "SparkFun_BLEMate2.h"
#include <Arduino.h>

// Constructor. All we really need to do is link the user's Stream instance to
//  our local reference.
BLEMate2::BLEMate2(Stream *sp)
{
  _serialPort = sp;
  _numAddresses = 0;
}

// The only way to get the true full address of the module is to check the
//  module's firmware version with the "VER" command. The stdCmd() function
//  isn't really useful here; we'll take our cue from the BLEScan() function.
BLEMate2::opResult BLEMate2::addressQuery(String &address)
{
  // We're going to assume a failure to find the appropriate string, but a
  //  response of some kind. We'll call that a MODULE_ERROR.
  opResult result  = MODULE_ERROR;
  String buffer = "";
  String EOL = String("\n\r");
  
  knownStart();
  
  // Send the command and wait for it to complete.
  _serialPort->print("VER\r");
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the command. Bog-standard Arduino stuff.
  unsigned long loopStart = millis();
  unsigned long loopTimeout = 2000;
  
  // Oooookaaaayyy...now the fun part. A state machine that parses the input
  //  from the Bluetooth module!
  while (loopStart + loopTimeout > millis())
  {
    // Grow the current buffered data, until we receive the EOL string.    
    if (_serialPort->available() >0)
    {
      buffer.concat(char(_serialPort->read()));
    }
    if (buffer.endsWith(EOL))
    {
      // There are several possibilities for return values:
      //  1. ERR - indicates a problem with the module. Either we're in the
      //           wrong state to be trying to do this (we're not central,
      //           not idle, or both) or there's a syntax error.
      //  2. Bluetooth Address xxxxxxxxxxxx - this is the one we want to
      //           match. HOWEVER, there are *other* input strings after
      //           issuing the VER command.
      //  3. BlueCreation Copyright 2012-2014
      //  4. www.bluecreation.com
      //  5. Melody Smart vxxxxxxx
      //  6. Build: xxxxxxxxx
      //  7. OK
      // The important string is number 2, and of course it comes last.
      //  Thus, we want to discard any string that doesn't start with "Bluet".
      //  We also need to return upon "OK".
      if (buffer.startsWith("ER")) 
      {
        return MODULE_ERROR;
      }
      if (buffer.startsWith("Bluet")) // The address has been found!
      {
        // The returned device string looks like this:
        //  Bluetooth Address xxxxxxxxxxxx         
        // We can ignore the other stuff, and the first stuff, and just
        //  report the address. 
        address = buffer.substring(18,30);
        buffer = "";
        result = SUCCESS;
      }
      else if (buffer.startsWith("OK"))
      {
        return result;
      }
      else // buffer is not a valid value
      {
        buffer = "";
      }
    }
  }
  // This command is always going to return TIMEOUT; the BC118 doesn't report
  //  anything when it stops sending out data. It just...stops.
  return TIMEOUT_ERROR;
}

// Change the baud rate. Doesn't take effect until write/reset cycle, so you
//  can get the "OK" message after changing the setting.
BLEMate2::opResult BLEMate2::setBaudRate(unsigned int newSpeed)
{
  // Temp for the string value you'll want to send out to the module.
  String speedString = "";

  // The BC118 doesn't want a nice, human readable string; it wants a 16-bit
  //  unsigned int represented as a string. 
  // Here we'll convert things appropriately from the numeric speed input
  //  (2400, 9600, 19200, 38400, 57600) to a string which the BC118 recognizes
  //  as being the parameter for that string.
  switch(newSpeed)
  {
    case 2400:
      speedString = "000A";
      break;
    case 9600:
      speedString = "0028";
      break;
    case 19200:
      speedString = "004E";
      break;
    case 38400:
      speedString = "009E";
      break;
    case 57600:
      speedString = "00EB";
      break;
    default:
      return INVALID_PARAM;
  }

  // Because this doesn't take effect until after a write/reset, stdSetParam()
  //  works perfectly.
  return stdSetParam("UART", speedString);
}

// There are several commands that look for either OK or ERROR; let's abstract
//  support for those commands to one single private function, to save memory.
BLEMate2::opResult BLEMate2::stdCmd(String command)
{
  String buffer;
  String EOL = String("\n\r");
  
  knownStart(); // Clear the serial buffer in the module and the Arduino.
  
  _serialPort->print(command);
  _serialPort->print("\r");
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the command. Bog-standard Arduino stuff.
  unsigned long startTime = millis();
    
  // This is our timeout loop. We'll give the module 3 seconds.
  while ((startTime + 3000) > millis())
  {
    // Grow the current buffered data, until we receive the EOL string.    
    if (_serialPort->available() > 0) buffer.concat(char(_serialPort->read()));

    if (buffer.endsWith(EOL))
    {
      if (buffer.startsWith("ER")) return MODULE_ERROR;
      if (buffer.startsWith("OK")) return SUCCESS;
      buffer = "";
    }    
  }
  return TIMEOUT_ERROR;
}

// Similar to the command function, let's do a set parameter genrealization.
BLEMate2::opResult BLEMate2::stdSetParam(String command, String param)
{
  String buffer;
  String EOL = String("\n\r");
  
  knownStart();  // Clear Arduino and module serial buffers.
  
  _serialPort->print("SET ");
  _serialPort->print(command);
  _serialPort->print("=");
  _serialPort->print(param);
  _serialPort->print("\r");
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the reset. Bog-standard Arduino stuff.
  unsigned long startTime = millis();
  
  // This is our timeout loop. We'll give the module 2 seconds to reset.
  while ((startTime + 2000) > millis())
  {
    // Grow the current buffered data, until we receive the EOL string.    
    if (_serialPort->available() >0) buffer.concat(char(_serialPort->read()));

    if (buffer.endsWith(EOL))
    {
      if (buffer.startsWith("ER")) return MODULE_ERROR;
      else if (buffer.startsWith("OK")) return SUCCESS;
      buffer = "";
    }    
  }
  return TIMEOUT_ERROR;
}

// Also, do a get paramater generalization. This is, of course, a bit more
//  difficult; we need to return both the result (SUCCESS/ERROR) and the
//  string returned.
BLEMate2::opResult BLEMate2::stdGetParam(String command, String &param)
{
  String buffer;
  String EOL = String("\n\r");
  
  knownStart();  // Clear the serial buffers.
  
  _serialPort->print("GET ");
  _serialPort->print(command);
  _serialPort->print("\r");
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the get command. Bog-standard Arduino stuff.
  unsigned long loopStart = millis();
  
  // This is our timeout loop. We'll give the module 2 seconds to get the value.
  while (loopStart + 2000 > millis())
  {
    // Grow the current buffered data, until we receive the EOL string.    
    if (_serialPort->available() >0) buffer.concat(char(_serialPort->read()));

    // Each time we encounter an EOL, we'll parse the current buffer.
    if (buffer.endsWith(EOL))
    {
      // ER and OK are simple enough- success or failure.
      if (buffer.startsWith("ER")) return MODULE_ERROR;
      if (buffer.startsWith("OK")) return SUCCESS;
      // BUT if the buffer starts with the command value, we'll want to extract
      //  the value returned by the module. As an example, "get ADDR" will 
      //  cause the module to return with "ADDR=value\n\rOK\n\r"
      if (buffer.startsWith(command))
      {
        // First, get the portion of the string beginning after the command
        //  string + 1 (to account for that equals sign).
        param = buffer.substring(command.length()+1);
        // Then, trim the whitespace (/n/r) off the buffer.
        param.trim();
      }
      buffer = "";
    }    
  }
  // We don't expect this operation to take too long, so we can return a
  //  timeout if we get here.
  return TIMEOUT_ERROR;
}

// Function to put the module into BLE Central Mode.
BLEMate2::opResult BLEMate2::BLECentral()
{
  return stdSetParam("CENT", "ON");
}

BLEMate2::opResult BLEMate2::BLEPeripheral()
{
  return stdSetParam("CENT", "OFF");
}

// Issue the "RESTORE" command over the serial port to the BC118. This will
//  reset the device to factory default settings, which is a good thing to do
//  once in a while.
BLEMate2::opResult BLEMate2::restore()
{
  return stdCmd("RTR");
}

// Issue the "WRITE" command over the serial port to the BC118. This will
//  save the current settings to NVM, so they will be applied after a reset
//  or power cycle.
BLEMate2::opResult BLEMate2::writeConfig()
{
  return stdCmd("WRT");
}

// Issue the "RESET" command over the serial port to the BC118. If it works, 
//  we expect to see a string that looks something like this:
//    Melody Smart v2.6.0
//    BlueCreation Copyright 2012 - 2014
//    www.bluecreation.com
//    READY
// If there is some sort of error, the module will respond with
//    ERR
// We'll buffer characters until we see an EOL (\n\r), then check the string.
BLEMate2::opResult BLEMate2::reset()
{
  String buffer;
  String EOL = String("\n\r");
  
  knownStart();
  
  // Now issue the reset command.
  _serialPort->print("RST");
  _serialPort->print("\r");
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the reset. Bog-standard Arduino stuff.
  unsigned long resetStart = millis();
  
  // This is our timeout loop. We'll give the module 6 seconds to reset.
  while ((resetStart + 6000) > millis())
  {
    // Grow the current buffered data, until we receive the EOL string.    
    if (_serialPort->available() > 0) 
    {
      char temp = _serialPort->read();
      buffer.concat(temp);
    }
    
    if (buffer.endsWith(EOL))
    {
      // If ERR or READY, we've finished the reset. Otherwise, just discard
      //  the data and wait for the next EOL.
      if (buffer.startsWith("ER")) return MODULE_ERROR;
      if (buffer.startsWith("RE")) 
      {
        stdCmd("SCN OFF"); // When we come out of reset, we *could* be
                           //  in scan mode. We don't want that; it's too
                           //  random and noisy.
        delay(500);        // Let the scanning noise complete.
        while(_serialPort->available())
        {
          _serialPort->read();
        } 
        return SUCCESS;
      }
      buffer = "";
    }    
  }
  return TIMEOUT_ERROR;
}

// Create a known state for the module to start from. If a partial command is
//  already in the module's buffer, we can purge it by sending an EOL to the
//  the module. If not, we'll just get an error.
BLEMate2::opResult BLEMate2::knownStart()
{
  String EOL = String("\n\r");
  String buffer = "";
  
  _serialPort->print("\r");
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the reset. Bog-standard Arduino stuff.
  unsigned long startTime = millis();
  
  // This is our timeout loop. We're going to give our module 1s to come up
  //  with a new character, and return with a timeout failure otherwise.
  while (buffer.endsWith(EOL) != true)
  {
    // Purge the serial data received from the module, along with any data in
    //  the buffer at the time this command was sent.
    if (_serialPort->available() > 0) 
    {
      buffer.concat(char(_serialPort->read()));
      startTime = millis();
    }
    if ((startTime + 1000) < millis()) return TIMEOUT_ERROR;
  }
  if (buffer.startsWith("ERR")) return SUCCESS;
  else return SUCCESS;
}

// For sendData, we have three possible options that we'll consider.
//  1. User wants to send a constant string.
//  2. User wants to send a variable string, encoded as a String object.
//  3. User wants to send an array of characters.
// From a data standpoint, 1 and 2 are just subsets of three, so we'll
//  write most of the functionality into 3 and call it from 2, and call
//  2 from 1.
BLEMate2::opResult BLEMate2::sendData(const char *dataBuffer)
{
  String newBuffer = dataBuffer;
  return sendData(newBuffer);
}

BLEMate2::opResult BLEMate2::sendData(String &dataBuffer)
{
  // First, we'll need to figure out the length of the string.
  int bufLength = dataBuffer.length()+1;

  // Now dynamically allocate an array of the appropriate size.
  char *charArray = new char [bufLength];

  // Copy over the data.
  dataBuffer.toCharArray(charArray, bufLength);

  // Call the byte array function
  opResult result = sendData(charArray, bufLength);

  // de-allocate the memory.
  delete charArray;

  return result;
}

// Now, byte array.
BLEMate2::opResult BLEMate2::sendData(char *dataBuffer, byte dataLen)
{
  String EOL = String("\n\r");
  
  // BLE is a super low bandwidth protocol. The BC118 is only going to allow
  //  you to drop 20 bytes in central mode, or 125 bytes in peripheral mode.
  //  I don't want to burden the user with that, unduly, so I'm going to chop
  //  up their data and send it out in smaller blocks.
   
  // Thus, the first quetion is: am I in central mode, or not?
  boolean inCentralMode;
  amCentral(inCentralMode);

  // What we're now going to do is to build a String object with our buffer
  //  contents and then hit send on that buffer when it reaches a the length
  //  limited by the mode.

  byte outBufLenLimit = 20;
  if (!inCentralMode)
  {
    outBufLenLimit = 125;
  }

  byte inBufPtr = 0;
  byte outBufLen = 0;
  byte dataLeft = dataLen;

  opResult result = SUCCESS;
  while (inBufPtr < dataLen)
  {
    if (dataLeft < outBufLenLimit)
    {
      outBufLenLimit = dataLeft;
    }
    dataLeft -= outBufLenLimit;
    String outBuf;
    while (outBufLen < outBufLenLimit)
    {
      outBuf.concat(dataBuffer[inBufPtr++]);
      outBufLen++;
    }
    outBuf = "SND " + outBuf + "\r";
    result = stdCmd(outBuf);
    outBufLen = 0;
  }
  return result;
}

// We may at some point not know whether we're a central or peripheral
//  device; that's important information, so we should be able to query
//  the module regarding that. We're not going to store that info, however, 
//  since the whole point is to get it "from the horse's mouth" rather than
//  trusting that our software is in sync with the state of the module.
BLEMate2::opResult BLEMate2::amCentral(boolean &inCentralMode)
{
  String buffer;
  String EOL = String("\n\r");
  
  knownStart(); // Clear the serial buffer in the module and the Arduino.
  
  _serialPort->print("STS\r");
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the command. Bog-standard Arduino stuff.
  unsigned long startTime = millis();
    
  // This is our timeout loop. We'll give the module 3 seconds.
  while ((startTime + 3000) > millis())
  {
    // Grow the current buffered data, until we receive the EOL string.    
    if (_serialPort->available() > 0) 
    {
      buffer.concat(char(_serialPort->read()));
    }
    if (buffer.endsWith(EOL))
    {
      if (buffer.startsWith("ER")) 
      {
        return MODULE_ERROR;
      }
      else if (buffer.startsWith("OK")) 
      {
        return SUCCESS;
      }
      else if (buffer.startsWith("STS")) 
      {
        if (buffer.charAt(4) == 'C')
        {
          inCentralMode = true;
        }
        else
        {
          inCentralMode = false;
        }
      } 
      buffer = "";
    }    
  }
  return TIMEOUT_ERROR;
}


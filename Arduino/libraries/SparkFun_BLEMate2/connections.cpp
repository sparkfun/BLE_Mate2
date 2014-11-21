/****************************************************************
Connection management functions for BC118 modules.

This is based on the code I wrote for the BC127 module.

This code is beerware; if you use it, please buy me (or any other
SparkFun employee) a cold beverage next time you run into one of
us at the local.

30 Oct 2014- Mike Hord, SparkFun Electronics

Code developed in Arduino 1.0.6, on an Arduino Pro 5V.
****************************************************************/

#include "SparkFun_BLEMate2.h"
#include <Arduino.h>

// BLEAdvertise() and BLENoAdvertise() turn advertising on and off for this
//  module. Advertising must be turned on for another BLE device to detect the
//  module, and the module *must* be a peripheral for advertising to work (see
//  the "BLEPeripheral()" function.
BLEMate2::opResult BLEMate2::BLEAdvertise()
{
  return stdCmd("ADV ON");
}

BLEMate2::opResult BLEMate2::BLENoAdvertise()
{
  return stdCmd("ADV OFF");
}

// With the BC118, *scan* is a much more important thing that with the BC127.
//  It's a "state", and in order to initiate a connection as a central device,
//  the BC118 *must* be in scan state.
// Timeout is not inherent to the scan command, either; that's a separate 
//  parameter that needs to be set.
BLEMate2::opResult BLEMate2::BLEScan(unsigned int timeout)
{

  // Let's assume that we find nothing; we'll call that a REMOTE_ERROR and
  //  report that to the user. Should we find something, we'll report success.
  opResult result = REMOTE_ERROR;
  String buffer = "";
  String addressTemp;
  String EOL = String("\n\r");
  
  String timeoutString = String(timeout, DEC);
  stdSetParam("SCNT", timeoutString);
  for (byte i = 0; i <5; i++) _addresses[i] = "";
  _numAddresses = 0;
    
  knownStart();
  
  // Now issue the scan command. 
  _serialPort->print("SCN ON\r");
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the command. Bog-standard Arduino stuff.
  unsigned long loopStart = millis();
  
  // Calculate a timeout value that's a tish longer than the module will
  //  use. This is our catch-all, so we don't sit in this loop forever waiting
  //  for input that will never come from the module.
  unsigned long loopTimeout = timeout*1300;
  
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
      // There are two possibilities for return values:
      //  1. ERR - indicates a problem with the module. Either we're in the
      //           wrong state to be trying to do this (we're not central,
      //           not idle, or both) or there's a syntax error.
      //  2. SCN=X 12charaddrxx xxxxxxxxxxxxxxx\n\r
      //           In this case, all we care about is the content from char
      //           6 to 18.
      // Note the lack of any kind of completion string! The module just stops
      //  reporting when done, and we'll never know if it doesn't find anything
      //  to report.
      if (buffer.startsWith("ER")) 
      {
        return MODULE_ERROR;
      }
      else if (buffer.startsWith("SC")) // An address has been found!
      {
        // The returned device string looks like this:
        //  scn=? 12charaddrxx bunch of other stuff\n\r
        // We can ignore the other stuff, and the first stuff, and just
        //  report the address. 
        addressTemp = buffer.substring(6,18);
        buffer = "";
        if (_numAddresses == 0) 
        {
          _addresses[0] = addressTemp;
          _numAddresses++;
          result = SUCCESS;
        }
        else // search the list for this address, and append if it's not in
             //  the list and the list isn't too long.
        {
          for (byte i = 0; i < _numAddresses; i++)
          {
            if (addressTemp == _addresses[i])
            {
              addressTemp = "x";
              break;
            }
          }
          if (addressTemp != "x")
          {
            _addresses[_numAddresses++] = addressTemp;
          }
          if (_numAddresses == 5) return SUCCESS;
        }
      }
      else // buffer is not a valid value
      {
        buffer = "";
      }
    }
  }
  // result will either have been unchanged (we didn't see anything) and
  //  be REMOTE_ERROR or will have been changed to SUCCESS when we saw our
  //  first address.
  return result;
}

// connect by index
//  Attempts to connect to one of the Bluetooth devices which has an address
//  stored in the _addresses array.
BLEMate2::opResult BLEMate2::connect(byte index)
{
  if (index >= _numAddresses) return INVALID_PARAM;
  else return connect(_addresses[index]);
}

// connect by address
//  Attempts to connect to one of the Bluetooth devices which has an address
//  stored in the _addresses array.
BLEMate2::opResult BLEMate2::connect(String address)
{
  // Before we go any further, we'll do a simple error check on the incoming
  //  address. We know that it should be 12 hex digits, all uppercase; to
  //  minimize execution time and code size, we'll only check that it's 12
  //  characters in length.
  if (address.length() != 12) return INVALID_PARAM;

  // We need a buffer to store incoming data.
  String buffer;
  String EOL = String("\n\r"); // This is just handy.
  
  
  knownStart(); // Purge serial buffers on both the module and the Arduino.
  
  // The module has to be in SCAN mode for the CON command to work.
  //  We can't use BLEScan() b/c it's a blocking function.
  _serialPort->print("SCN ON\r");
  
  // Now issue the inquiry command.
  _serialPort->print("CON "); 
  _serialPort->print(address);
  _serialPort->print(" 0\r");
  // We need to wait until the command finishes before we start looking for a
  //  response; that's what flush() does.
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the connect command. Bog-standard Arduino stuff.
  unsigned long connectStart = millis();
  
  buffer = "";

  // The timeout on this is 5 seconds; that may be a bit long.
  while (connectStart + 5000 > millis())
  {
    // Grow the current buffered data, until we receive the EOL string.    
    if (_serialPort->available() >0) buffer.concat(char(_serialPort->read()));
    
    if (buffer.endsWith(EOL))
    {
      if (buffer.startsWith("ERR")) return MODULE_ERROR;
      if (buffer.startsWith("RPD")) return SUCCESS;
      buffer = "";    
    }
  }
  return TIMEOUT_ERROR;
}

// Gets an address from the array of stored addresses. The return value allows
//  the user to check on whether there was in fact a valid address at the
//  requested index.
BLEMate2::opResult BLEMate2::getAddress(byte index, String &address)
{
  if (index+1 > _numAddresses)
  {
    String tempString = "";
    address = tempString;
    return INVALID_PARAM;
  }
  else address = _addresses[index];
  return SUCCESS;
}

// Gets the number of addresses we've found.
byte BLEMate2::numAddresses()
{
  return _numAddresses;
}



BLEMate2::opResult BLEMate2::connectionState()
{
  return TIMEOUT_ERROR;
}

BLEMate2::opResult BLEMate2::disconnect()
{
  String buffer;
  String EOL = String("\n\r"); // This is just handy.
  
  knownStart();
  _serialPort->print("DCN\r"); 
  _serialPort->flush();
  
  // We're going to use the internal timer to track the elapsed time since we
  //  issued the connect command. Bog-standard Arduino stuff.
  unsigned long disconnectStart = millis();
  
  buffer = "";

  // The timeout on this is 5 seconds; that may be a bit long.
  while (disconnectStart + 5000 > millis())
  {
    // Grow the current buffered data, until we receive the EOL string.    
    if (_serialPort->available() >0) buffer.concat(char(_serialPort->read()));
    
    if (buffer.endsWith(EOL))
    {
      if (buffer.startsWith("ERR")) return MODULE_ERROR;
      if (buffer.startsWith("DCN")) 
      {
        stdCmd("SCN OFF"); 
        return SUCCESS;
      }
      buffer = "";    
    }
  }
  return TIMEOUT_ERROR;
}

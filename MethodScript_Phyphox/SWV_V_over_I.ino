// SWV (connect to WE B of PalmSens Dummy Cell = 10k ohm resistor)

#include <Arduino.h>
#include "MathHelpers.h"
#include "MSComm.h"
#include <phyphoxBle.h> 

int _nDataPoints = 0;
char _versionString[30];

static bool s_printSent = false;
static bool s_printReceived = false;
char const * CMD_VERSION_STRING = "t\n";
float currentValue = 0.0;//////////////////////////////////////////////////////////////////////////////////////
  float voltageValue = 0.0;//////////////////////////////////////////////////////////////////////////////////////

// SWV MethodSCRIPT
char const * SWV_ON_10KOHM = 

"e\n"
"var c\n"
"var p\n"
"var f\n"
"var g\n"
"set_pgstat_chan 1\n"
"set_pgstat_mode 0\n"
"set_pgstat_chan 0\n"
"set_pgstat_mode 2\n"
"set_max_bandwidth 1k\n"
"set_range_minmax da -525m 25m\n"
"set_range ba 2950u\n"
"set_autoranging ba 59n 2950u\n"
"set_e 0m\n"
"cell_on\n"
"meas_loop_ca p c 0 500m 10\n"
"pck_start\n"
"pck_end\n"
"endloop\n"
"meas_loop_swv p c f g  0 -500m 4m 25m 25\n"
"pck_start\n"
"pck_add p\n"
"pck_add c\n"
//"pck_add f\n"
//"pck_add g\n"
"pck_end\n"
"endloop\n"
"on_finished:\n"
"cell_off\n"
"\n";


// The MethodSCRIPT communication object.
MSComm _msComm;

// We have to give MSComm some functions to communicate with the EmStat Pico (in MSCommInit).
// However, because the C compiler doesn't understand C++ classes,
// we must wrap the write/read functions from the Serial class in a normal function first.
// We are using Serial and Serial1 here, but you can use any serial port.
int write_wrapper(char c)
{
  if (s_printSent) {
    // Send all data to PC if required for debugging purposes (s_printReceived to be set to true)
    Serial.write(c);
  }
  // Write a character to the device
  return Serial1.write(c);
}

int read_wrapper()
{
  // Read a character from the device
  int c = Serial1.read();

  if (s_printReceived && (c != -1)) { // -1 means no data
    // Send all received data to PC if required for debugging purposes (s_printReceived to be set to true)
    Serial.write(c);
  }
  return c;
}

// Verify if connected to EmStat Pico by checking the version string contains the "espico"
int VerifyESPico()
{
  int i = 0;
  int isConnected = 0;
  RetCode code;

  SendScriptToDevice(CMD_VERSION_STRING);
  while (!Serial1.available()) {
    // Wait until data is available on Serial1
  }
  while (Serial1.available()) {
    code = ReadBuf(&_msComm, _versionString);
    if (code == CODE_VERSION_RESPONSE) {
      if (strstr(_versionString, "espbl") != NULL) {
        Serial.println("EmStat Pico is connected in boot loader mode.");
        isConnected = 0;

      // Verify if the device is EmStat Pico by looking for "espico" in the version response
      } else if (strstr(_versionString, "espico") != NULL) {
        Serial.println("Connected to EmStat Pico");
        isConnected = 1;
      }

    //Read until end of response and break
    } else if(strstr(_versionString, "*\n") != NULL) {
      break;

    } else {
      Serial.println("Connected device is not EmStat Pico");
      isConnected = 0;
    }
    delay(20);
  }
  return isConnected;
}



//%% Part of the MSComm initialization
//%% The purpose of this function is to send the provided script text (the one in SWV...) to the EmStat Pico device using the _msComm communication object. 
//%% The WriteStr function is likely responsible for sending the script text over a serial connection (Serial1 in your case) to the EmStat Pico, 
//%%   allowing you to control the device and perform various operations or measurements specified in the script.

// The MethodSCRIPT is sent to the device through the Serial1 port
void SendScriptToDevice(char const * scriptText)
{
  WriteStr(&_msComm, scriptText);
}




///
/// Print one MethodSCRIPT output subpackage on the console.
///
/// parameters:
///   subpackage The subpackage to process
///
void PrintSubpackage(const MscrSubPackage subpackage)
{
  // Format and print the subpackage value
  // This is a bit bulky, but does nothing more than call printf with a format that is
  // sensible for the `variable type` of the subpackage.


  // float currentValue = 0.0;//////////////////////////////////////////////////////////////////////////////////////
  // float voltageValue = 0.0;//////////////////////////////////////////////////////////////////////////////////////
  //float voltageOverCurrent = 0.0;////////////////////////////////////////////////////////////////////////////////

  //%% can probably delete vacant cases
  switch(subpackage.variable_type) {

  case MSCR_VT_POTENTIAL:
  case MSCR_VT_POTENTIAL_CE:
  case MSCR_VT_POTENTIAL_SE:
  case MSCR_VT_POTENTIAL_RE:
  case MSCR_VT_POTENTIAL_GENERIC1:
  case MSCR_VT_POTENTIAL_GENERIC2:
  case MSCR_VT_POTENTIAL_GENERIC3:
  case MSCR_VT_POTENTIAL_GENERIC4:
  case MSCR_VT_POTENTIAL_WE_VS_CE:
    Serial.print("\tE[V]: ");
    Serial.print(sci(subpackage.value, 3));
    //voltageValue = subpackage.value;  // Ensure the value is not const ////////////////////////////////
    break;

  case MSCR_VT_CURRENT:
  case MSCR_VT_CURRENT_GENERIC1:
  case MSCR_VT_CURRENT_GENERIC2:
  case MSCR_VT_CURRENT_GENERIC3:
  case MSCR_VT_CURRENT_GENERIC4:
    Serial.print("\tI[A]: ");
    Serial.print(sci(subpackage.value, 3));
    currentValue = subpackage.value;  // Ensure the value is not const ////////////////////////////////
    //PhyphoxBLE::write(currentValue); // Send current value to Phyphox /////////////////////////////////////////
    break;

  case MSCR_VT_ZREAL:
    Serial.print("\tZreal[ohm]: ");
    Serial.print(sci(subpackage.value, 3));
    break;

  case MSCR_VT_ZIMAG:
    Serial.print("\tZimag[ohm]: ");
    Serial.print(sci(subpackage.value, 3));
    break;

  case MSCR_VT_CELL_SET_POTENTIAL:
    Serial.print("\tE set[V]: ");
    Serial.print(sci(subpackage.value, 3));
    voltageValue = subpackage.value; 
      //PhyphoxBLE::write(voltageValue, currentValue);  // Ensure the value is not const ////////////////////////////////
    break;

  case MSCR_VT_CELL_SET_CURRENT:
    Serial.print("\tI set[A]: ");
    Serial.print(sci(subpackage.value, 3));
    break;

  case MSCR_VT_CELL_SET_FREQUENCY:
    Serial.print("\tF set[Hz]: ");
    Serial.print(sci(subpackage.value, 3));
    break;

  case MSCR_VT_CELL_SET_AMPLITUDE:
    Serial.print("\tA set[V]: ");
    Serial.print(sci(subpackage.value, 2));
    break;

  case MSCR_VT_UNKNOWN:
  default:
    char formatted_srt[64];
    snprintf(formatted_srt, 64, "\t?%d?[?] %16.3f ", subpackage.variable_type, subpackage.value);
    Serial.print(formatted_srt);
  }

  // Print metadata
  // Note a value of <0 indicates it was provided in the MethodSCRIPT output

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Calculate and print V/I ratio if both values are available
  //if (currentValue != 0.0 ) {   //&& voltageValue != 0.0
    //float voltageOverCurrent = voltageValue / currentValue;
    //Serial.print("\tV/I [ohm]: ");
    //Serial.print(voltageOverCurrent, 3);
  PhyphoxBLE::write(voltageValue, currentValue); // Send V/I ratio to Phyphox
  //}


  // `Status` field metadata
  if (subpackage.metadata.status >= 0) {
    char const * status_str;
    if (subpackage.metadata.status == 0) {
      status_str = StatusToString((Status)0);
    } else {
      // The `status` field is a bitmask, so we have to check every bit separately.
      // Only print the first flag that was set to keep the output readable.
      for (int i = 0; i < 31; i++) {
        if ((subpackage.metadata.status & (1 << i)) != 0) {
          status_str = StatusToString((Status)(1 << i));
          break;
        }
      }
    }

    char formatted_srt[64];
    snprintf(formatted_srt, 64, "\tstatus: %-16s \t", status_str);
    Serial.print(formatted_srt);
  }

  // `current range` metadata
  if (subpackage.metadata.current_range >= 0) {
    char const * current_range_str = current_range_to_string(subpackage.metadata.current_range);
    char formatted_srt[64];
    snprintf(formatted_srt, 64, "CR: %-20s \t", current_range_str);
    Serial.print(formatted_srt);
  }
}


// The entry point of the Arduino code
void setup()
{  
  // Init serial ports
  // Serial is the Arduino serial port communicating with the PC
  Serial.begin(230400);
  // Serial1 is the port on EmStat Pico dev board communicating with the Arduino
  Serial1.begin(230400);
  // Wait until the Serial port is active
  while (!Serial) {
    /* do nothing */
  }

  if (s_printReceived == true) {
    Serial.println("s_printReceived");
  }


//%%Here's where MSComm is initialized

  //Init MSComm struct (one for every EmStat Pico).
  RetCode code = MSCommInit(&_msComm, &write_wrapper, &read_wrapper);
  if (code == CODE_OK) {
    if (VerifyESPico()) {
      SendScriptToDevice(SWV_ON_10KOHM); //Send the "SWV_ON_10KOHM" MethodSCRIPT to the device
    }
  }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  PhyphoxBLE::start("phyphox device");                 //Start the BLE server
  // Define a new Phyphox BLE experiment
  PhyphoxBleExperiment currentVoltExperiment;

  // Set experiment properties
  currentVoltExperiment.setTitle("current vs voltage");
  currentVoltExperiment.setCategory("Arduino Experiments");
  currentVoltExperiment.setDescription("This experiment will plot the measured current over voltage.");

  // Define a view for the experiment
  PhyphoxBleExperiment::View currentVoltView;
  currentVoltView.setLabel("CurrentVoltData"); // Create a "view"

  // Define a graph for the experiment
  PhyphoxBleExperiment::Graph currentVoltGraph; // Create a graph which will plot current over time
  currentVoltGraph.setLabel("Current VS Voltage");
  currentVoltGraph.setUnitX("V");
  currentVoltGraph.setUnitY("A");
  currentVoltGraph.setLabelX("Voltage");
  currentVoltGraph.setLabelY("Current");

  // Assign Channels: which data is plotted on x or y axis
  currentVoltGraph.setChannel(1, 2);

  // Attach graph to view
  currentVoltView.addElement(currentVoltGraph);
  // Attach view to experiment
  currentVoltExperiment.addView(currentVoltView);
  // Attach experiment to BLE server
  PhyphoxBLE::addExperiment(currentVoltExperiment);
}



int package_nr = 0;



//%% "loop()" is the main loop that makes this code work.
//%% In it, "code" is important, because it indicates stages of the measurement (RetCode code = ReceivePackage(&_msComm, &package);)...

//%% The "ReceivePackage" function likely handles reading data from a serial port (Serial1 in your case) and interpreting it according to a specific protocol used by the EmStat Pico device. 
//%% Depending on the outcome of the operation, code will be set to a value defined by the RetCode enumeration, 
//%%   which typically includes values like CODE_OK for success or various error codes for different types of failures.

void loop()
{
  // put your main code here, to run repeatedly:
  MscrPackage package;
  char current[20];
  char readingStatus[16];
  // If we have any buffered messages waiting for us
  while (Serial1.available()) {
    // Read from the device and parses the response
    RetCode code = ReceivePackage(&_msComm, &package);
    switch (code) {
    case CODE_RESPONSE_BEGIN: // Measurement response begins
      Serial.println();
      Serial.print("Response begin");
      Serial.println();
      break;
    case CODE_MEASURING:
      Serial.println();
      Serial.print("Measuring...");
      Serial.println();
      package_nr = 0;
      break;
    case CODE_OK: // Received valid package, print it.
      if (package_nr++ == 0) {
        Serial.println();
        Serial.println("Receiving measurement response:");
      }
      // Print package index (starting at 1 on the console)
      Serial.print(package_nr);
      // Print all subpackages in
      for (int i = 0; i < package.nr_of_subpackages; i++) {
        PrintSubpackage(package.subpackages[i]);
      }
      Serial.println();
      break;
    case CODE_MEASUREMENT_DONE: // Measurement loop complete
      Serial.println();
      Serial.println("Measurement completed.");
      break;
    case CODE_RESPONSE_END: // Measurement response end
      Serial.print(package_nr);
      Serial.println(" data point(s) received.");
      break;
    default: // Failed to parse or identify package
      Serial.println();
      Serial.print("Failed to parse package: ");
      Serial.println(code);
    }
  }
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //float randomNumber = random(0,100); //Generate random number in the range 0 to 100
  //PhyphoxBLE::write(randomNumber);     //Send value to phyphox
  //delay(50);                          //Shortly pause before repeating
  PhyphoxBLE::poll();                  //IMPORTANT: In contrast to other devices, poll() needs to be called periodically on the 33 IoT
}





//PhyphoxBLE::write needed for printing under the Case of the desired output
//PhyphoxBLE start and definitions of new experiment needed under void setup()
//PhyphoxBLE::poll() needed for running the experiement
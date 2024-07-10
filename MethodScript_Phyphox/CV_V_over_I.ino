//it will repeat experiment when testing with no source?
//also the voltage value is changing back and forth is it normal?
//if no there's probbaly something we have to change in the method script


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


// CV MethodSCRIPT
char const * CV = "e\n"
                  "var c\n"
                  "var p\n"
                  "set_pgstat_chan 1\n"
                  "set_pgstat_mode 0\n"
                  "set_pgstat_chan 0\n"
                  "set_pgstat_mode 2\n"
                  "set_max_bandwidth 20\n"
                  "set_range_minmax da -400m 400m\n"
                  "set_range ba 590u\n"
                  "set_autoranging ba 59n 590u\n"
                  "set_e -400m\n"
                  "cell_on\n"
                  "meas_loop_cv p c -400m -400m 400m 10m 50m nscans(3)\n" 
                  "pck_start\n"
                  "pck_add p\n"     //anything to change here?
                  "pck_add c\n"
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
    break;

  case MSCR_VT_CURRENT:
  case MSCR_VT_CURRENT_GENERIC1:
  case MSCR_VT_CURRENT_GENERIC2:
  case MSCR_VT_CURRENT_GENERIC3:
  case MSCR_VT_CURRENT_GENERIC4:
    Serial.print("\tI[A]: ");
    Serial.print(sci(subpackage.value, 3));
    currentValue = subpackage.value;  // Ensure the value is not const ////////////////////////////////
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
    voltageValue = subpackage.value; ////////////////////////////////////////////////////////////////////
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

  PhyphoxBLE::write(voltageValue, currentValue); // Send V/I ratio to Phyphox//////////////////////////////////////////

  // Print metadata
  // Note a value of <0 indicates it was provided in the MethodSCRIPT output

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

  //Init MSComm struct (one for every EmStat Pico).
  RetCode code = MSCommInit(&_msComm, &write_wrapper, &read_wrapper);
  if (code == CODE_OK) {
    if (VerifyESPico()) {
      SendScriptToDevice(CV); //Send the "CV" MethodSCRIPT to the device
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
  PhyphoxBLE::poll();             ///////////////////////////////////////////////////////////////////////////////////////////
}

//PhyphoxBLE::write needed for printing under the Case of the desired output
//PhyphoxBLE start and definitions of new experiment needed under void setup()
//PhyphoxBLE::poll() needed for running the experiement

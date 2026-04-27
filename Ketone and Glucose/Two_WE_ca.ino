//it will repeat experiment when testing with no source?
//also the voltage value is changing back and forth is it normal?
//if no there's probbaly something we have to change in the method script
//USE THIS ONE - OMEED 2/10/2026


#include <Arduino.h>
#include "MathHelpers.h"
#include "MSComm.h"
#include <phyphoxBle.h>
#define DEVICE_TYPE DT_ESPICO

///////////////////////////////////////////////////////////////////////////////////////

int _nDataPoints = 0;
char _versionString[30];

static bool s_printSent = false;
static bool s_printReceived = false;
char const * CMD_VERSION_STRING = "t\n";
float current1 = 0.0;//////////////////////////////////////////////////////////////////////////////////////
float current2 = 0.0;//////////////////////////////////////////////////////////////////////////////////////
//int we_num = 0;////////////////////////////////////////
int channel_num = 1;//////////////////////////////////
//int expected_datapoint = 151;/////////////////////////////////////////


// CA_ON_10KOHM MethodSCRIPT
char const * CA_ON_10KOHM = "e\n"
                              "var c\n"
                              "var p\n"
                              "var b\n"////////////////////////////////////////////////
                              //////////////////////////////////////////////////////
                              //enable bipot on ch 1 
                              "set_pgstat_chan 1\n" 
                              //set the selected channel to bipot mode 
                              "set_pgstat_mode 5\n" 
                              //set bp mode to offset or constant 
                              "set_poly_we_mode 1\n"
                              //set offset or constant voltage 
                              //The mode of the additional working electrode: 
                              //0 = fixed mode (Additional WE is kept fixed at the specified potential) 
                              //1 = offset mode (Additional WE will follow the main WE at a specified offset potential)
                              "set_e 100m\n" ////////////////////could change
                              //set the current-range of the secondary WE 
                              "set_range ba 590n\n"


                              "set_pgstat_chan 0\n"
                              "set_pgstat_mode 2\n"
                              "set_max_bandwidth 40\n"
                              "set_range_minmax da 200m 200m\n"
                              "set_range ba 59n\n"
                              "set_autoranging ba 59n 590n\n"
                              "set_e 200m\n"
                              "cell_on\n"
                              "wait 1\n"
                              "meas_loop_ca p c 200m 8 60100000m poly_we(1 b)\n"
                              "pck_start\n"
                              "pck_add p\n"
                              "pck_add c\n"
                              "pck_add b\n"/////////////////////////////////////////////
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
    //////////////////////////////////////////////////
    //if still in first iteration, then else(print first value)  
    //if in second iteration, then print second value
    //for this to be true, we need to know 
    // we_num += 1;
    // if (we_num > expected_datapoint){
      if (channel_num ==1){
        channel_num = 2;
        current1 = subpackage.value;
      }
      else{
        channel_num = 1;
        current2 = subpackage.value;
      }
      PhyphoxBLE::write(current1, current2); 
    // else{
    //   current1 = subpackage.value;
    //   PhyphoxBLE::write(voltage1, current1); 
    // }
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
//////////////////////////////////////////////////////////////////////////////////////////////////


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
  if (subpackage.metadata.range >= 0) {
    char const * range_str = range_to_string(subpackage.metadata.range, (VarType)subpackage.variable_type);
    char formatted_srt[64];
    snprintf(formatted_srt, 64, "CR: %-20s \t",range_str);
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
  //while (!Serial) {//////////////////////////////////////////////////////////////////////////////////////////////
    /* do nothing */
  //}

  if (s_printReceived == true) {
    Serial.println("s_printReceived");
  }

  //Init MSComm struct (one for every EmStat Pico).
  RetCode code = MSCommInit(&_msComm, DEVICE_TYPE, &write_wrapper, &read_wrapper);
  if (code == CODE_OK) {
    if (VerifyESPico()) {
      SendScriptToDevice(CA_ON_10KOHM); //Send the "CA_ON_10KOHM" MethodSCRIPT to the device
    }
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // Start the BLE server
  PhyphoxBLE::start("NBEStat");                 //Start the BLE server
  // Define a new Phyphox BLE experiment
  PhyphoxBleExperiment currentExperiment;

  // Set experiment properties
  currentExperiment.setTitle("2WE_Ket_Glu");
  currentExperiment.setCategory("Arduino Experiments");
  currentExperiment.setDescription("This experiment will plot the measured current over time.");

  // Define a view for the experiment
  PhyphoxBleExperiment::View currentView;
  currentView.setLabel("CurrentData"); // Create a "view"

  // Define a graph for the experiment
  PhyphoxBleExperiment::Graph currentGraph; // Create a graph which will plot current over time
  currentGraph.setLabel("Ketone vs. Time");
  currentGraph.setUnitX("s");
  currentGraph.setUnitY("A");
  currentGraph.setLabelX("Time");
  currentGraph.setLabelY("Current");

  currentGraph.setChannel(0,1);/////////////////////////////////////////////////////
  currentGraph.setColor("ff00ff");
  currentGraph.setLinewidth(5);//if you dont select a linewidth, a width of 1 is used by default

  //////////////////////////////////////////////////////////////////////////////////////////
  PhyphoxBleExperiment::Value WE1;         //Creates a value-box.
  WE1.setLabel("Current");                  //Sets the label
  WE1.setPrecision(12);                     //The amount of digits shown after the decimal point.
  WE1.setUnit("A");                        //The physical unit associated with the displayed value.
  WE1.setColor("ff00ff");                  //Sets font color. Uses a 6 digit hexadecimal value in "quotation marks".
  WE1.setChannel(1);
  //WE1.setXMLAttribute("size=\"2\"");

  // Define a graph for the experiment
  PhyphoxBleExperiment::Graph currentGraph1; // Create a graph which will plot current over time
  currentGraph1.setLabel("Glucose vs. Time");
  currentGraph1.setUnitX("s");
  currentGraph1.setUnitY("A");
  currentGraph1.setLabelX("Time");
  currentGraph1.setLabelY("Current");

  currentGraph1.setChannel(0,2);///////////////////////////////////////////////////
  currentGraph1.setColor("00ff00");
  currentGraph1.setLinewidth(5);//if you dont select a linewidth, a width of 1 is used by default
  
  PhyphoxBleExperiment::Value WE2;         //Creates a value-box.
  WE2.setLabel("Current");                  //Sets the label
  WE2.setPrecision(12);                     //The amount of digits shown after the decimal point.
  WE2.setUnit("A");                        //The physical unit associated with the displayed value.
  WE2.setColor("00ff00");                  //Sets font color. Uses a 6 digit hexadecimal value in "quotation marks".
  WE2.setChannel(2);

  // Assign Channels: which data is plotted on x or y axis

  // Attach graph to view
  currentView.addElement(currentGraph);
  currentView.addElement(WE1);
  currentView.addElement(currentGraph1);
  currentView.addElement(WE2);
  // Attach view to experiment
  currentExperiment.addView(currentView);
  // Attach experiment to BLE server
  PhyphoxBLE::addExperiment(currentExperiment);

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
  PhyphoxBLE::poll();            
}

//PhyphoxBLE::write needed for printing under the Case of the desired output
//PhyphoxBLE start and definitions of new experiment needed under void setup()
//PhyphoxBLE::poll() needed for running the experiement

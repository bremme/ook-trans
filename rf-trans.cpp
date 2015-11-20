#include <iostream>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>


#include <wiringPi.h>

#include "RemoteTransmitter.h"

#define RT_RAW_PRO    -1
#define RT_ACTION_PRO 0

namespace po = boost::program_options;

using namespace std;

// prototypes
void  setupPins(int pin, bool verbose);
int   getSystemCode(po::variables_map& vm);
int   getDeviceCode(po::variables_map& vm);
int   getState(std::string& cmd);
bool  displayHelp(po::variables_map& vm, po::options_description& desc);
bool  displayVersion(po::variables_map& vm, char **argv);
void  stringCodeToTrits(std::string& cmd, unsigned char trits[12]);
const double SW_VERSION = 1.0;

int main(int argc, char **argv) {



  // required options
  int protocol;
  int pin;
  std::string cmd;
  bool verbose    = false;

	po::options_description desc(
  "Usage:\trf-trans -g PIN -p PROTOCOL [-s] [SYSTEM-CODE] [-d] [DEVICE-CODE] COMMAND\n\
  \trf-trans -g PIN -p PROTOCOL COMMAND\n\
  Send encoded messages to 433 MHz wall sockets\n\n\
  OPTIONS");

	desc.add_options()
		("help,h",                                          "Show this help message")
		("version",                                         "Show version number")
		("verbose,v",                                       "Vebose output")
		("gpio-pin,g",    po::value<int>(&pin)->required(), "GPIO pin (wiringPi)")
		("protocol,p",    po::value<int>(&protocol)->required(),"RF Protocol (0,1,2 or -1)")
		("system-code,s", po::value<int>(),                 "System code")
		("device-code,d", po::value<int>(),                 "Device code")
		("pulse-us,P",    po::value<unsigned int>(),        "Pulse in micro seconds (1/4 period)")
		// ("raw-trits,T",   po::value< std::string >(),    "Raw code in trits")
		// ("raw-bits,B",    po::value< std::string >(),    "Raw code in bits")
		// ("raw-dec,D",     po::value<unsigned int>(),        "Raw code in decimal (from trits)")
    ("command,c",     po::value< std::string >(&cmd)->required(),"State or raw command")
	;

  // Map positional parameters to their tag valued types
  // (e.g. –input-file parameters)
  po::positional_options_description p;
  p.add("command", -1);

  // build variables map
	po::variables_map vm;
	try {
		po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
	} catch (std::exception &e) {
		cout << endl << argv[0] << ":\t" << e.what() << "\n\n";
		cout << desc << endl;
    return EXIT_FAILURE;
	};


  // display help
  if ( displayHelp(vm, desc) ) { 	return EXIT_SUCCESS; };
  // display version
  if ( displayVersion(vm, argv) ) { return EXIT_SUCCESS; };

  // validate input
  try {
    po::notify(vm);
  } catch (std::exception &e) {
    cout << endl << argv[0] << ":\t" << e.what() << "\n\n";
		cout << desc << endl;
    return EXIT_FAILURE;
  }

  // set verbose mode
  if ( vm.count("verbose")) { verbose = true; };

  // load wiringPi
	if( wiringPiSetup() == -1) {
		printf("WiringPi setup failed. Is wiringPi properly installed?");
		return EXIT_FAILURE;
	};

  // setup pins
  setupPins(pin, verbose);

  if( verbose ) {
    cout << "Protocol " << protocol << " selected\n";
  };

  // action protocol
  if( protocol == RT_ACTION_PRO ) {
    // action protocol needs: system code, device code, and state

    if (!vm.count("system-code") || !vm.count("device-code") ) {
      cout << "For protocol " << protocol << " '-s [--system-code] and -d [--device-code]' are required\n\n";
      return EXIT_FAILURE;
    };

    int systemCode = getSystemCode(vm);
    int deviceCode = getDeviceCode(vm);
    int state      = getState(cmd);

    if(verbose) {
      cout << "Sending:\t" << "system code: " << systemCode << " device code: " << deviceCode << " state: " << state << endl;
    };
    ActionTransmitter transmitter(pin);
    transmitter.sendSignal(systemCode, deviceCode + 65, (state == 1)?true:false);

  } else if (protocol == -1) {

    // validate required arguments:
    if ( !vm.count("pulse-us") ) {
      cout << "For protocol " << protocol << " '-P [--pulse-us] is required\n\n";
      return EXIT_FAILURE;
    };

    unsigned int pulseUs = vm["pulse-us"].as<unsigned int>();

    // raw length can be 12 trits OR 24 bits or decimal (shorter than 12)
    if( cmd.length() == 12 ) {
      unsigned char trits[12];
      stringCodeToTrits(cmd, trits);
      RemoteTransmitter transmitter(pin, pulseUs, 4);
      transmitter.sendTelegram( trits );
    } else if ( cmd.length() == 24 ) {

    } else if ( cmd.length() < 12 ) {

    } else {
      cout << "A raw code has to be either 12 trits, 24 bits or a decimal number\n\n";
      return EXIT_FAILURE;
    };
  }

	return EXIT_SUCCESS;
}

void stringCodeToTrits(std::string& cmd, unsigned char trits[12]) {
  for(short int i = 0; i < cmd.length(); i++) {
    trits[i] = (cmd[i] == '0')?0:((cmd[i] == '1')?1:2);
  };
};

void setupPins(int pin, bool verbose) {
  if(verbose) {
    cout << "Setting pin " << pin << " to OUTPUT\n";
    cout << "Writing LOW to " << pin << endl;
  };
  pinMode(pin, OUTPUT);   // setup pin
  digitalWrite(pin, LOW); // write low
};

int getSystemCode(po::variables_map& vm) {
  return vm["system-code"].as<int>();
};

int getDeviceCode(po::variables_map& vm) {
  return vm["device-code"].as<int>();
};

int getState(std::string& cmd) {
  // vector<string> cmd = vm["command"].as< vector<string> >();
  int state = ( cmd[0] == '0')?0:1;
  return state;
};

bool displayHelp(po::variables_map& vm, po::options_description& desc) {
  if (vm.count("help")) {
		cout << desc << "\n";
		return true;
	};
  return false;
};

bool displayVersion(po::variables_map& vm, char **argv) {
  if (vm.count("version")) {
    cout << argv[0] << "\tversion " << SW_VERSION << "\n";
    return true;
  };
  return false;
}

/*
Usage: 433-switch [OPTIONS] [STATE]
Send 433Mhz codes

  -g  --gpio=       gpio data pin (wiring pi numbering)
	-p, --protocol=		set protocol
	-s, --system=		set system code
	-d, --device=		set device

	-P, --period=		set period us
	-T, --raw-tris=		send raw trits
	-B, --raw-bits=		send raw bits

Examples:
	433-trans -p 1 -s 30 -d 0 1	   Send protocol message
	433-trans -T 2111120222202		 Send raw trits
	433-trans -D 26456				Send raw dec

send protocol message (have to speciffy protocol)
protocol is a combination of period us and position of the bits
	-p 1 30 0 1
	-p 2 5 1

send raw message
	-R 1010101010101

*/

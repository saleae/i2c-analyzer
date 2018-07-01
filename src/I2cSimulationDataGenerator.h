#ifndef SERIAL_SIMULATION_DATA_GENERATOR
#define SERIAL_SIMULATION_DATA_GENERATOR

#include <AnalyzerHelpers.h>
#include "I2cAnalyzerSettings.h"
#include <stdlib.h>

class I2cSimulationDataGenerator
{
public:
	I2cSimulationDataGenerator();
	~I2cSimulationDataGenerator();

	void Initialize( U32 simulation_sample_rate, I2cAnalyzerSettings* settings );
	U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );

protected:
	I2cAnalyzerSettings* mSettings;
	U32 mSimulationSampleRateHz;
	U8 mValue;

protected:	//I2c specific
			//functions
	void CreateI2cTransaction( U8 address, I2cDirection direction, U8 data );
	void CreateI2cByte( U8 data, I2cResponse reply );
	void CreateBit( BitState bit_state );
	void CreateStart();
	void CreateStop();
	void CreateRestart();
	void SafeChangeSda( BitState bit_state );

protected: //vars
	ClockGenerator mClockGenerator;

	SimulationChannelDescriptorGroup mI2cSimulationChannels;
	SimulationChannelDescriptor* mSda;
	SimulationChannelDescriptor* mScl;
};
#endif //UNIO_SIMULATION_DATA_GENERATOR

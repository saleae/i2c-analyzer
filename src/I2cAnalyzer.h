#ifndef SERIAL_ANALYZER_H
#define SERIAL_ANALYZER_H

#include <Analyzer.h>
#include "I2cAnalyzerResults.h"
#include "I2cSimulationDataGenerator.h"

class I2cAnalyzerSettings;
class I2cAnalyzer : public Analyzer2
{
public:
	I2cAnalyzer();
	virtual ~I2cAnalyzer();
	virtual void SetupResults();
	virtual void WorkerThread();

	virtual U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channels );
	virtual U32 GetMinimumSampleRateHz();

	virtual const char* GetAnalyzerName() const;
	virtual bool NeedsRerun();


#pragma warning( push )
#pragma warning( disable : 4251 ) //warning C4251: 'SerialAnalyzer::<...>' : class <...> needs to have dll-interface to be used by clients of class

protected: //functions
	void AdvanceToStartBit();
	void GetByte();
	bool GetBit( BitState& bit_state, U64& sck_rising_edge );
	bool GetBitPartOne( BitState& bit_state, U64& sck_rising_edge, U64& frame_end_sample );
	bool GetBitPartTwo();
	void RecordStartStopBit();

protected: //vars
	std::auto_ptr< I2cAnalyzerSettings > mSettings;
	std::auto_ptr< I2cAnalyzerResults > mResults;
	AnalyzerChannelData* mSda;
	AnalyzerChannelData* mScl;

	I2cSimulationDataGenerator mSimulationDataGenerator;
	bool mSimulationInitilized;

	//Serial analysis vars:
	U32 mSampleRateHz;
	bool mNeedAddress;
	std::vector<U64> mArrowLocations;

#pragma warning( pop )
};

extern "C" ANALYZER_EXPORT const char* __cdecl GetAnalyzerName();
extern "C" ANALYZER_EXPORT Analyzer* __cdecl CreateAnalyzer( );
extern "C" ANALYZER_EXPORT void __cdecl DestroyAnalyzer( Analyzer* analyzer );

#endif //SERIAL_ANALYZER_H

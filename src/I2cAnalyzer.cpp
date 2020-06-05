#include "I2cAnalyzer.h"
#include "I2cAnalyzerSettings.h"
#include <AnalyzerChannelData.h>

I2cAnalyzer::I2cAnalyzer() : Analyzer2(), mSettings( new I2cAnalyzerSettings() ), mSimulationInitilized( false )
{
    SetAnalyzerSettings( mSettings.get() );
}

I2cAnalyzer::~I2cAnalyzer()
{
    KillThread();
}

void I2cAnalyzer::SetupResults()
{
    mResults.reset( new I2cAnalyzerResults( this, mSettings.get() ) );
    SetAnalyzerResults( mResults.get() );
    mResults->AddChannelBubblesWillAppearOn( mSettings->mSdaChannel );
}

void I2cAnalyzer::WorkerThread()
{
    mSampleRateHz = GetSampleRate();
    mNeedAddress = true;

    mSda = GetAnalyzerChannelData( mSettings->mSdaChannel );
    mScl = GetAnalyzerChannelData( mSettings->mSclChannel );

    AdvanceToStartBit();
    mScl->AdvanceToNextEdge(); // now scl is low.

    for( ;; )
    {
        GetByte();
        CheckIfThreadShouldExit();
    }
}

void I2cAnalyzer::GetByte()
{
    mArrowLocations.clear();
    U64 value;
    DataBuilder byte;
    byte.Reset( &value, AnalyzerEnums::MsbFirst, 8 );
    U64 starting_sample = 0;
    U64 potential_ending_sample = 0;

    for( U32 i = 0; i < 8; i++ )
    {
        BitState bit_state;
        U64 scl_rising_edge;
        bool result = GetBitPartOne( bit_state, scl_rising_edge, potential_ending_sample );
        result &= GetBitPartTwo();
        if( result == true )
        {
            mArrowLocations.push_back( scl_rising_edge );
            byte.AddBit( bit_state );

            if( i == 0 )
                starting_sample = scl_rising_edge;
        }
        else
        {
            return;
        }
    }

    BitState ack_bit_state;
    U64 scl_rising_edge;
    S64 last_valid_sample = mScl->GetSampleNumber();
    bool result = GetBitPartOne( ack_bit_state, scl_rising_edge, potential_ending_sample ); // GetBit( ack_bit_state, scl_rising_edge );

    FrameV2 framev2;
    char* framev2Type = nullptr;

    Frame frame;
    frame.mStartingSampleInclusive = starting_sample;
    frame.mEndingSampleInclusive = result ? potential_ending_sample : last_valid_sample;
    frame.mData1 = U8( value );

    if( !result )
    {
        framev2.AddString( "error", "missing ack/nak" );
        frame.mFlags = I2C_MISSING_FLAG_ACK;
    }
    else
    {
        bool ack = ack_bit_state == BIT_LOW;

        // true == ack, false == nak
        framev2.AddBoolean( "ack", ack );
        if( ack )
        {
            frame.mFlags = I2C_FLAG_ACK;
        }
    }

    if( mNeedAddress == true && result == true ) // if result is false, then we have already recorded a stop bit and toggled mNeedAddress
    {
        mNeedAddress = false;
        bool is_read = value & 0x01;
        U8 address = value >> 1;
        frame.mType = I2cAddress;
        framev2Type = "address";
        framev2.AddByte( "address", address );
        framev2.AddBoolean( "read", is_read );
    }
    else
    {
        frame.mType = I2cData;
        framev2Type = "data";
        framev2.AddByte( "data", value );
    }

    mResults->AddFrame( frame );
    mResults->AddFrameV2( framev2, framev2Type, starting_sample, result ? potential_ending_sample : last_valid_sample );

    U32 count = mArrowLocations.size();
    for( U32 i = 0; i < count; i++ )
    {
        mResults->AddMarker( mArrowLocations[ i ], AnalyzerResults::UpArrow, mSettings->mSclChannel );
    }

    mResults->CommitResults();

    result &= GetBitPartTwo();
}

bool I2cAnalyzer::GetBit( BitState& bit_state, U64& sck_rising_edge )
{
    // SCL must be low coming into this function
    mScl->AdvanceToNextEdge(); // posedge
    sck_rising_edge = mScl->GetSampleNumber();
    mSda->AdvanceToAbsPosition( sck_rising_edge ); // data read on SCL posedge

    bit_state = mSda->GetBitState();
    bool result = true;

    // this while loop is only important if you need to be careful and check for things that that might happen at the very end of a data
    // set, and you don't want to get stuck waithing on a channel that never changes.
    while( mScl->DoMoreTransitionsExistInCurrentData() == false )
    {
        // there are no more SCL transtitions, at least yet.
        if( mSda->DoMoreTransitionsExistInCurrentData() == true )
        {
            // there ARE some SDA transtions, let's double check to make sure there's still no SDA activity
            auto next_data_edge = mSda->GetSampleOfNextEdge();
            if( mScl->WouldAdvancingToAbsPositionCauseTransition( next_data_edge - 1 ) )
            {
                break;
            }

            // ok, for sure we can advance to the next SDA edge without running past any SCL events.
            mSda->AdvanceToNextEdge();

            RecordStartStopBit();
            result = false;
        }
    }

    mScl->AdvanceToNextEdge(); // negedge; we'll leave the clock here
    while( mSda->WouldAdvancingToAbsPositionCauseTransition( mScl->GetSampleNumber() - 1 ) == true )
    {
        // clock is high -- SDA changes indicate start, stop, etc.
        mSda->AdvanceToNextEdge();
        RecordStartStopBit();
        result = false;
    }

    return result;
}

bool I2cAnalyzer::GetBitPartOne( BitState& bit_state, U64& sck_rising_edge, U64& frame_end_sample )
{
    // SCL must be low coming into this function
    mScl->AdvanceToNextEdge(); // posedge
    sck_rising_edge = mScl->GetSampleNumber();
    frame_end_sample = sck_rising_edge;
    mSda->AdvanceToAbsPosition( sck_rising_edge ); // data read on SCL posedge

    bit_state = mSda->GetBitState();

    // clock is on the rising edge, and data is at the same location.

    while( mScl->DoMoreTransitionsExistInCurrentData() == false )
    {
        // there are no more SCL transtitions, at least yet.
        if( mSda->DoMoreTransitionsExistInCurrentData() == true )
        {
            // there ARE some SDA transtions, let's double check to make sure there's still no SDA activity
            auto next_data_edge = mSda->GetSampleOfNextEdge();
            if( mScl->WouldAdvancingToAbsPositionCauseTransition( next_data_edge - 1 ) )
            {
                break;
            }

            // ok, for sure we can advance to the next SDA edge without running past any SCL events.
            mSda->AdvanceToNextEdge();
            mScl->AdvanceToAbsPosition( mSda->GetSampleNumber() ); // clock is still high, we're just moving it to the stop condition here.
            RecordStartStopBit();
            return false;
        }
    }

    // ok, so there are more transitions on the clock channel, so the above code path didn't run.
    U64 sample_of_next_clock_falling_edge = mScl->GetSampleOfNextEdge();
    while( mSda->WouldAdvancingToAbsPositionCauseTransition( sample_of_next_clock_falling_edge - 1 ) == true )
    {
        // clock is high -- SDA changes indicate start, stop, etc.
        mSda->AdvanceToNextEdge();
        mScl->AdvanceToAbsPosition( mSda->GetSampleNumber() ); // advance the clock to match the SDA channel.
        RecordStartStopBit();
        return false;
    }

    if( mScl->DoMoreTransitionsExistInCurrentData() == true )
    {
        frame_end_sample = mScl->GetSampleOfNextEdge();
    }

    return true;
}

bool I2cAnalyzer::GetBitPartTwo()
{
    // the sda and scl should be synced up, and we are either on a stop/start condition (clock high) or we're on a regular bit( clock high).
    // we also should not expect any more start/stop conditions before the next falling edge, I beleive.

    // move to next falling edge.
    bool result = true;
    mScl->AdvanceToNextEdge();
    while( mSda->WouldAdvancingToAbsPositionCauseTransition( mScl->GetSampleNumber() - 1 ) == true )
    {
        // clock is high -- SDA changes indicate start, stop, etc.
        mSda->AdvanceToNextEdge();
        RecordStartStopBit();
        result = false;
    }
    return result;
}

void I2cAnalyzer::RecordStartStopBit()
{
    bool start = mSda->GetBitState() == BIT_LOW;
    if( start )
    {
        // negedge -> START / restart
        mResults->AddMarker( mSda->GetSampleNumber(), AnalyzerResults::Start, mSettings->mSdaChannel );

        FrameV2 framev2;
        mResults->AddFrameV2( framev2, "start", mSda->GetSampleNumber(), mSda->GetSampleNumber() + 1 );
    }
    else
    {
        // posedge -> STOP
        mResults->AddMarker( mSda->GetSampleNumber(), AnalyzerResults::Stop, mSettings->mSdaChannel );
    }

    mNeedAddress = true;
    mResults->CommitPacketAndStartNewPacket();
    mResults->CommitResults();

    if( !start )
    {
        FrameV2 framev2;
        mResults->AddFrameV2( framev2, "stop", mSda->GetSampleNumber(), mSda->GetSampleNumber() + 1 );
    }
}

void I2cAnalyzer::AdvanceToStartBit()
{
    for( ;; )
    {
        mSda->AdvanceToNextEdge();

        if( mSda->GetBitState() == BIT_LOW )
        {
            // SDA negedge
            mScl->AdvanceToAbsPosition( mSda->GetSampleNumber() );
            if( mScl->GetBitState() == BIT_HIGH )
                break;
        }
    }
    mResults->AddMarker( mSda->GetSampleNumber(), AnalyzerResults::Start, mSettings->mSdaChannel );
}

bool I2cAnalyzer::NeedsRerun()
{
    return false;
}

U32 I2cAnalyzer::GenerateSimulationData( U64 minimum_sample_index, U32 device_sample_rate,
                                         SimulationChannelDescriptor** simulation_channels )
{
    if( mSimulationInitilized == false )
    {
        mSimulationDataGenerator.Initialize( GetSimulationSampleRate(), mSettings.get() );
        mSimulationInitilized = true;
    }

    return mSimulationDataGenerator.GenerateSimulationData( minimum_sample_index, device_sample_rate, simulation_channels );
}

U32 I2cAnalyzer::GetMinimumSampleRateHz()
{
    return 2000000;
}

const char* I2cAnalyzer::GetAnalyzerName() const
{
    return "I2C";
}

const char* GetAnalyzerName()
{
    return "I2C";
}

Analyzer* CreateAnalyzer()
{
    return new I2cAnalyzer();
}

void DestroyAnalyzer( Analyzer* analyzer )
{
    delete analyzer;
}

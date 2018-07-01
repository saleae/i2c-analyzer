#include "I2cAnalyzerResults.h"
#include <AnalyzerHelpers.h>
#include "I2cAnalyzer.h"
#include "I2cAnalyzerSettings.h"
#include <iostream>
#include <sstream>
#include <stdio.h>

I2cAnalyzerResults::I2cAnalyzerResults( I2cAnalyzer* analyzer, I2cAnalyzerSettings* settings )
:	AnalyzerResults(),
	mSettings( settings ),
	mAnalyzer( analyzer )
{
}

I2cAnalyzerResults::~I2cAnalyzerResults()
{
}

void I2cAnalyzerResults::GenerateBubbleText( U64 frame_index, Channel& /*channel*/, DisplayBase display_base )  //unrefereced vars commented out to remove warnings.
{
	//we only need to pay attention to 'channel' if we're making bubbles for more than one channel (as set by AddChannelBubblesWillAppearOn)
	ClearResultStrings();
	Frame frame = GetFrame( frame_index );

	char ack[32];
	if( ( frame.mFlags & I2C_FLAG_ACK ) != 0 )
		snprintf( ack, sizeof(ack), "ACK" );
	else if( ( frame.mFlags & I2C_MISSING_FLAG_ACK ) != 0 )
		snprintf( ack, sizeof( ack ), "Missing ACK/NAK" );
	else
		snprintf( ack, sizeof(ack), "NAK" );

	if( frame.mType == I2cAddress )
	{
		char number_str[128];
		switch( mSettings->mAddressDisplay )
		{
		case NO_DIRECTION_7:
			AnalyzerHelpers::GetNumberString( frame.mData1 >> 1, display_base, 7, number_str, 128 );
			break;
		case NO_DIRECTION_8:
			AnalyzerHelpers::GetNumberString( frame.mData1 & 0xFE, display_base, 8, number_str, 128 );
			break;
		case YES_DIRECTION_8:
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
			break;
		}

		I2cDirection direction;
		if( ( frame.mData1 & 0x1 ) != 0 )
			direction = I2C_READ;
		else
			direction = I2C_WRITE;

		if( direction == I2C_READ )
		{
			std::stringstream ss;
			AddResultString( "R" );

			ss << "R[" << number_str << "]";
			AddResultString( ss.str().c_str() );
			ss.str("");

			ss << "Read [" << number_str << "]";
			AddResultString( ss.str().c_str() );
			ss.str("");

			ss << "Read [" << number_str << "] + " << ack;
			AddResultString( ss.str().c_str() );
			ss.str("");	

			ss << "Setup Read to [" << number_str << "] + " << ack;
			AddResultString( ss.str().c_str() );
		}else
		{
			std::stringstream ss;
			ss << "W[" << number_str << "]";
			AddResultString( ss.str().c_str() );
			ss.str("");

			ss << "Write [" << number_str << "]";
			AddResultString( ss.str().c_str() );
			ss.str("");

			ss << "Write [" << number_str << "] + " << ack;
			AddResultString( ss.str().c_str() );
			ss.str("");	

			ss << "Setup Write to [" << number_str << "] + " << ack;
			AddResultString( ss.str().c_str() );
		}
	}else
	{
		char number_str[128];
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );

		AddResultString( number_str );

		std::stringstream ss;
		ss << number_str << " + " << ack;
		AddResultString( ss.str().c_str() );
	}													
}

void I2cAnalyzerResults::GenerateExportFile( const char* file, DisplayBase display_base, U32 /*export_type_user_id*/ )
{
	//export_type_user_id is only important if we have more than one export type.

	std::stringstream ss;
	void* f = AnalyzerHelpers::StartFile( file );;

	U64 trigger_sample = mAnalyzer->GetTriggerSample();
	U32 sample_rate = mAnalyzer->GetSampleRate();

	ss << "Time [s],Packet ID,Address,Data,Read/Write,ACK/NAK" << std::endl;

	char address[128] = "";
	char rw[128] = "";
	U64 num_frames = GetNumFrames();
	for( U32 i=0; i < num_frames; i++ )
	{
		Frame frame = GetFrame( i );

		if( frame.mType == I2cAddress )
		{
			switch( mSettings->mAddressDisplay )
			{
			case NO_DIRECTION_7:
				AnalyzerHelpers::GetNumberString( frame.mData1 >> 1, display_base, 7, address, 128 );
				break;
			case NO_DIRECTION_8:
				AnalyzerHelpers::GetNumberString( frame.mData1 & 0xFE, display_base, 8, address, 128 );
				break;
			case YES_DIRECTION_8:
				AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, address, 128 );
				break;
			}
			if( ( frame.mData1 & 0x1 ) != 0 )
				snprintf( rw, sizeof(rw), "Read" );
			else
				snprintf( rw, sizeof(rw), "Write" );

			//check to see if the address packet is NAKed. If it is, we need to export the line here.
			if( ( frame.mFlags & I2C_FLAG_ACK ) == 0 )
			{
				char ack[32];
				if( (frame.mFlags & I2C_MISSING_FLAG_ACK) != 0 )
					snprintf( ack, sizeof( ack ), "Missing ACK/NAK" );
				else					
					snprintf( ack, sizeof( ack ), "NAK" );
				//we need to write out the line here.
				char time[128];
				AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time, 128 );

				ss << time << ",," << address << "," << "" << "," << rw << "," << ack << std::endl;
				AnalyzerHelpers::AppendToFile( ( U8* )ss.str( ).c_str( ), ss.str( ).length( ), f );
				ss.str( std::string( ) );
			}
				
		}
		else
		{
			char time[128];
			AnalyzerHelpers::GetTimeString( frame.mStartingSampleInclusive, trigger_sample, sample_rate, time, 128 );
			
			char data[128];
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, data, 128);

			char ack[32];
			if( ( frame.mFlags & I2C_FLAG_ACK ) != 0 )
				snprintf( ack, sizeof(ack), "ACK" );
			else if( ( frame.mFlags & I2C_MISSING_FLAG_ACK ) != 0 )
				snprintf( ack, sizeof( ack ), "Missing ACK/NAK" );
			else
				snprintf( ack, sizeof(ack), "NAK" );
				

			U64 packet_id = GetPacketContainingFrameSequential( i ); 
			if( packet_id != INVALID_RESULT_INDEX )
				ss << time << "," << packet_id << "," << address << "," << data << "," << rw << "," << ack << std::endl;
			else
				ss << time << ",," << address << "," << data << "," << rw << "," << ack << std::endl;
		}

		AnalyzerHelpers::AppendToFile( (U8*)ss.str().c_str(), ss.str().length(), f );
		ss.str( std::string() );
					
					
		if( UpdateExportProgressAndCheckForCancel( i, num_frames ) == true )
		{
			AnalyzerHelpers::EndFile( f );
			return;
		}
	}

	UpdateExportProgressAndCheckForCancel( num_frames, num_frames );
	AnalyzerHelpers::EndFile( f );
}

void I2cAnalyzerResults::GenerateFrameTabularText( U64 frame_index, DisplayBase display_base )
{
    ClearTabularText();

	Frame frame = GetFrame( frame_index );

	char ack[32];
	if( ( frame.mFlags & I2C_FLAG_ACK ) != 0 )
		snprintf( ack, sizeof(ack), "ACK" );
	else if( ( frame.mFlags & I2C_MISSING_FLAG_ACK ) != 0 )
		snprintf( ack, sizeof( ack ), "Missing ACK/NAK" );
	else
		snprintf( ack, sizeof(ack), "NAK" );

	if( frame.mType == I2cAddress )
	{
		char number_str[128];
		switch( mSettings->mAddressDisplay )
		{
		case NO_DIRECTION_7:
			AnalyzerHelpers::GetNumberString( frame.mData1 >> 1, display_base, 7, number_str, 128 );
			break;
		case NO_DIRECTION_8:
			AnalyzerHelpers::GetNumberString( frame.mData1 & 0xFE, display_base, 8, number_str, 128 );
			break;
		case YES_DIRECTION_8:
			AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
			break;
		}

		I2cDirection direction;
		if( ( frame.mData1 & 0x1 ) != 0 )
			direction = I2C_READ;
		else
			direction = I2C_WRITE;

		if( direction == I2C_READ )
		{
			std::stringstream ss;
			ss << "Setup Read to [" << number_str << "] + " << ack;
			AddTabularText( ss.str().c_str() );
		}else
		{
			std::stringstream ss;
			ss << "Setup Write to [" << number_str << "] + " << ack;
			AddTabularText( ss.str().c_str() );
		}
	}else
	{
		char number_str[128];
		AnalyzerHelpers::GetNumberString( frame.mData1, display_base, 8, number_str, 128 );
		std::stringstream ss;
		ss << number_str << " + " << ack;
		AddTabularText( ss.str().c_str() );
	}									
}

void I2cAnalyzerResults::GeneratePacketTabularText( U64 /*packet_id*/, DisplayBase /*display_base*/ )  //unrefereced vars commented out to remove warnings.
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

void I2cAnalyzerResults::GenerateTransactionTabularText( U64 /*transaction_id*/, DisplayBase /*display_base*/ )  //unrefereced vars commented out to remove warnings.
{
	ClearResultStrings();
	AddResultString( "not supported" );
}

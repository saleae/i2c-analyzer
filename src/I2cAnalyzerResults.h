#ifndef SERIAL_ANALYZER_RESULTS
#define SERIAL_ANALYZER_RESULTS

#include <AnalyzerResults.h>

#define I2C_FLAG_ACK ( 1 << 0 )
#define I2C_MISSING_FLAG_ACK ( 1 << 1 )

enum I2cFrameType
{
    I2cAddress,
    I2cData
};

class I2cAnalyzer;
class I2cAnalyzerSettings;

class I2cAnalyzerResults : public AnalyzerResults
{
  public:
    I2cAnalyzerResults( I2cAnalyzer* analyzer, I2cAnalyzerSettings* settings );
    virtual ~I2cAnalyzerResults();

    virtual void GenerateBubbleText( U64 frame_index, Channel& channel, DisplayBase display_base );
    virtual void GenerateExportFile( const char* file, DisplayBase display_base, U32 export_type_user_id );

    virtual void GenerateFrameTabularText( U64 frame_index, DisplayBase display_base );
    virtual void GeneratePacketTabularText( U64 packet_id, DisplayBase display_base );
    virtual void GenerateTransactionTabularText( U64 transaction_id, DisplayBase display_base );

  protected: // functions
  protected: // vars
    I2cAnalyzerSettings* mSettings;
    I2cAnalyzer* mAnalyzer;
};

#endif // SERIAL_ANALYZER_RESULTS

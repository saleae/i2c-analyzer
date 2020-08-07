#ifndef I2C_ANALYZER_SETTINGS
#define I2C_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

enum I2cDirection
{
    I2C_READ,
    I2C_WRITE
};
enum I2cResponse
{
    I2C_ACK,
    I2C_NAK
};

enum AddressDisplay
{
    NO_DIRECTION_7,
    NO_DIRECTION_8,
    YES_DIRECTION_8
};

class I2cAnalyzerSettings : public AnalyzerSettings
{
  public:
    I2cAnalyzerSettings();
    virtual ~I2cAnalyzerSettings();

    virtual bool SetSettingsFromInterfaces();
    virtual void LoadSettings( const char* settings );
    virtual const char* SaveSettings();

    void UpdateInterfacesFromSettings();

    Channel mSdaChannel;
    Channel mSclChannel;

  protected:
    std::auto_ptr<AnalyzerSettingInterfaceChannel> mSdaChannelInterface;
    std::auto_ptr<AnalyzerSettingInterfaceChannel> mSclChannelInterface;
};

#endif // I2C_ANALYZER_SETTINGS

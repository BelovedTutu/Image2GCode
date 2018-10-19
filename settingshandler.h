#ifndef SETTINGSHANDLER_H
#define SETTINGSHANDLER_H

#include <QObject>
#include <QStringList>

class SettingsHandler : public QObject
{
    Q_OBJECT
public:
    explicit SettingsHandler(QObject *parent = nullptr);
    //enum engraving mode
    enum e3DREngravingMode {
        LineByLine = 0,
        EdgeTracing = 1
    };

    void copy_that(const SettingsHandler& in_handler); // copy constructor

    //setters
    void SetMinLaserPwr(const int& mlp) { minLaserPwr = mlp; }
    void SetMaxLaserPwr(const int& mlp) { maxLaserPwr = mlp; }
    void SetLaserCmd(const QString& cmd) { LaserCmd = cmd; }
    void SetLaserOnOffCommand(const QStringList& onoffcmd) { LaserOnOffCommand = onoffcmd; }
    void SetEngravingMode(const SettingsHandler::e3DREngravingMode& mode) { EngravingMode = mode; }
    void SetXSettings(const float* xset) { XSettings[0] = *xset;XSettings[1] = *(xset+1);XSettings[2] = *(xset+2); }
    void SetYSettings(const float* yset) { YSettings[0] = *yset;YSettings[1] = *(yset+1);YSettings[2] = *(yset+2); }
    void SetZSettings(const float* zset) { ZSettings[0] = *zset;ZSettings[1] = *(zset+1);ZSettings[2] = *(zset+2); }
    void SetUsesZAxis(const bool& bUse) { usingZAxes = bUse; }
    void SetUsingImperialUnit(const bool& bUsesImp) { usingImperialUnit = bUsesImp; }
    void SetResolution(const float& res) { resol = res; }

    void SetStartGCode(const QStringList& start) { StartGCode = start; }
    void SetEndGCode(const QStringList& end) { EndGCode = end; }
    void SetFeedrate(const double& fr) { feedrate = fr; }
    void SetFilterValue(const double& fv) { filterVal = fv; }
    void SetBaseThicknessValue(const float& btv) { baseThicknessVal = btv; }
    void SetMaterialThickness(const float& mt) { MaterialThickness = mt; }
    void SetLensFocus(const float& lf) { LensFocus = lf; }

    //getters
    int GetMinLaserPwr() const { return minLaserPwr; }
    int GetMaxLaserPwr() const { return maxLaserPwr; }
    QString GetLaserCmd() const { return LaserCmd; }\
    QStringList GetLaserOnOffCommand() const { return LaserOnOffCommand; }
    SettingsHandler::e3DREngravingMode GetEngravingMode() const { return EngravingMode; }
    const float* GetXSettings() const { return &XSettings[0]; }
    const float* GetYSettings() const { return &YSettings[0]; }
    const float* GetZSettings() const { return &ZSettings[0]; }
    bool GetUsesZAxis() const { return usingZAxes; }
    bool GetUsingImperialUnit() const { return usingImperialUnit; }
    float GetResolution() const { return resol; }

    QStringList GetStartGCode() const { return StartGCode; }
    QStringList GetEndGCode() const { return EndGCode; }
    double GetFeedrate() const { return feedrate; }
    double GetFilterValue() const { return filterVal; }
    float GetBaseThicknessValue() const { return baseThicknessVal; }
    float GetMaterialThickness() const { return MaterialThickness; }
    float GetLensFocus() const { return LensFocus; }
    int ImportSettingsFromXml(const QString& in_settings_path);
signals:

public slots:

private:
    // Machine Settings
    int minLaserPwr; // max power of laser in gcode; default 0
    int maxLaserPwr; // min power of laser in gcode; default 255
    QString LaserCmd; // the laser power control command in gcode, either S or Z; defualt is S
    QStringList LaserOnOffCommand; // the on \ off command use for the machine, defautl to M3 \ M5
    SettingsHandler::e3DREngravingMode EngravingMode; // 0 : engraving horizontally (default)
                       // 1 : tracing and engraving only the traced line (use for cut)
    float XSettings[3]; // X min; X Max and X Offset
    float YSettings[3]; // Y min; Y Max and Y Offset
    bool usingZAxes; // the machine using Z axes or not; default to true
    bool usingImperialUnit; // the machine use imperial unit or not; defautl to false
    float ZSettings[3]; // Z min; Z Max and Z Offset
    float resol;

    //GCode Settings
    QStringList StartGCode;
    QStringList EndGCode;
    double feedrate;
    double filterVal;
    float baseThicknessVal;
    float MaterialThickness;
    float LensFocus;
};

#endif // SETTINGSHANDLER_H

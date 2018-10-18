#include <QFile>
#include <QDomDocument>
#include "settingshandler.h"

SettingsHandler::SettingsHandler(QObject *parent) : QObject(parent)
{
    minLaserPwr = 255; // max power of laser in gcode; default 0
    maxLaserPwr = 0; // min power of laser in gcode; default 255
    LaserCmd = 'S'; // the laser power control command in gcode, either S or Z; defualt is S
    LaserOnOffCommand << "M3" << "M5"; // the on \ off command use for the machine, defautl to M3 \ M5
    EngravingMode = e3DREngravingMode::LineByLine;
    XSettings[0] = 0.0f;XSettings[1] =  200.0f;XSettings[2] =  0.0f; // X min; X Max and X Offset
    YSettings[0] = 0.0f;YSettings[1] =  200.0f;YSettings[2] =  0.0f; // Y min; Y Max and Y Offset
    usingZAxes = true; // the machine using Z axes or not; default to true
    usingImperialUnit = false; // the machine use imperial unit or not; defautl to false
    ZSettings[0] = 0.0f;ZSettings[1] = 10.0f;ZSettings[2] = 0.0f; // Z min; Z Max and Z Offset
    resol = 0.2f;
    baseThicknessVal = 0.0f;
    MaterialThickness = 0.0f;
    //GCode Settings
    feedrate = 1500;
    filterVal = 0;
}

void SettingsHandler::copy_that(const SettingsHandler& in_handler)
{
    SetEndGCode(in_handler.GetEndGCode());
    SetEngravingMode(in_handler.GetEngravingMode());
    SetFeedrate(in_handler.GetFeedrate());
    SetFilterValue(in_handler.GetFilterValue());
    SetLaserCmd(in_handler.GetLaserCmd());
    SetLaserOnOffCommand(in_handler.GetLaserOnOffCommand());
    SetMaxLaserPwr(in_handler.GetMaxLaserPwr());
    SetMinLaserPwr(in_handler.GetMinLaserPwr());
    SetResolution(in_handler.GetResolution());
    SetStartGCode(in_handler.GetStartGCode());
    SetUsesZAxis(in_handler.GetUsesZAxis());
    SetXSettings(in_handler.GetXSettings());
    SetYSettings(in_handler.GetYSettings());
    SetZSettings(in_handler.GetZSettings());
    SetBaseThicknessValue(in_handler.GetBaseThicknessValue());
    SetMaterialThickness(in_handler.GetMaterialThickness());
}

int SettingsHandler::ImportSettingsFromXml(const QString& in_settings_path)
{
    QDomDocument* m_MyDoc = new QDomDocument();
    QFile* inFile = new QFile(in_settings_path);
    if(!inFile->open(QFile::ReadOnly | QFile::Text)){
        return -1;
    } else {
        m_MyDoc->setContent(inFile);
        QDomElement rootNode = m_MyDoc->documentElement();
        for (QDomElement node = rootNode.firstChild().toElement();!(node.isNull());node = node.nextSibling().toElement())
        {
            if(node.nodeName() == "StartGCode") {
                StartGCode = node.text().split('%');
            }
            else if(node.nodeName() == "EndGCode") {
                EndGCode =  node.text().split('%');
            }
            else if(node.nodeName() == "feedrate") {
                feedrate = node.text().toDouble();
            }
            else if(node.nodeName() == "filterVal") {
                filterVal = node.text().toDouble();
            }
            else if(node.nodeName() == "minLaserPwr") {
                minLaserPwr = node.text().toInt();
            }
            else if(node.nodeName() == "maxLaserPwr") {
                maxLaserPwr = node.text().toInt();
            }
            else if(node.nodeName() == "LaserCmd") {
                LaserCmd = node.text();
            }
            else if(node.nodeName() == "LaserOnCmd") {
                LaserOnOffCommand[0] = node.text();
            }
            else if(node.nodeName() == "LaserOffCmd") {
                LaserOnOffCommand[1] = node.text();
            }
            else if(node.nodeName() == "EngravingMode") {
                EngravingMode = (SettingsHandler::e3DREngravingMode)node.text().toInt();
            }
            else if(node.nodeName() == "XMin") {
                XSettings[0] = node.text().toFloat();
            }
            else if(node.nodeName() == "XMax") {
                XSettings[1] = node.text().toFloat();
            }
            else if(node.nodeName() == "XOffset") {
                XSettings[2] = node.text().toFloat();
            }
            else if(node.nodeName() == "YMin") {
                YSettings[0] = node.text().toFloat();
            }
            else if(node.nodeName() == "YMax") {
                YSettings[1] = node.text().toFloat();
            }
            else if(node.nodeName() == "YOffset") {
                YSettings[2] = node.text().toFloat();
            }
            else if(node.nodeName() == "ZMin") {
                ZSettings[0] = node.text().toFloat();
            }
            else if(node.nodeName() == "ZMax") {
                ZSettings[1] = node.text().toFloat();
            }
            else if(node.nodeName() == "ZOffset") {
                ZSettings[2] = node.text().toFloat();
            }
            else if(node.nodeName() == "resol") {
                resol = node.text().toFloat();
            }
            else if(node.nodeName() == "usingZAxes") {
                usingZAxes = (node.text().toInt() == 1);
            }
            else if(node.nodeName() == "usingImperialUnit") {
                usingImperialUnit = (node.text().toInt() == 1);
            }
            else if(node.nodeName() == "BaseThickness") {
                baseThicknessVal = (node.text().toInt() == 1);
            }
            else if(node.nodeName() == "MaterialThickness") {
                MaterialThickness = (node.text().toInt() == 1);
            }
        }
        inFile->close();
        delete inFile;
        return 0;
    }
}

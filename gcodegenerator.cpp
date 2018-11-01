#include <QFile>
#include <QTextStream>
#include <QColor>
#include <QImageWriter>
#include <QDir>
#include <QProcess>
#include <QTemporaryDir>

#include "gcodegenerator.h"
#include "SVGReader/svgreader.h"

GCodeGenerator::GCodeGenerator(const QString& in_image_path,\
                               const QString& out_path,
                               const SettingsHandler& in_handler,
                               QObject *parent) : QObject(parent)
{
    m_Settings.copy_that(in_handler);
    m_image.load(in_image_path);
    m_image = m_image.mirrored(true,true);
    m_GCodeOutPath = out_path;
}

float GCodeGenerator::interpolate(float val, int minRange, int maxRange)
{
   int difRange = maxRange - minRange;
   // rescale the rgb value val from 0..255 to minRange..maxRange
   return (float)(minRange + val * difRange / 255);
}

void GCodeGenerator::GenerateGCodeFromImage()
{
    QStringList fullGCodeOut = _PrivGenerateGCodeFromImage();
    if (!m_GCodeOutPath.contains(".gcode")) m_GCodeOutPath += ".gcode";
    QFile fout(m_GCodeOutPath);
    if (fout.open(QFile::WriteOnly | QFile::Text)) {
        QTextStream out(&fout);
        for ( QStringList::Iterator it = fullGCodeOut.begin(); it != fullGCodeOut.end(); ++it )
                        out << *it << "\n";
    }
    fout.close();
}

QStringList GCodeGenerator::_PrivGenerateGCodeFromImage()
{
    QStringList outCode;
    outCode << _PrivPreGenGCode();
    switch (m_Settings.GetEngravingMode())
    {
    case 0:
        outCode << _GenerateGCodeLineByLine();
        break;
    case 1:
        outCode << _GenerateGCodeTracingEdge();
        break;
    }
    outCode << _PrivPostGenGCode();
    return outCode;
}

QStringList GCodeGenerator::_PrivPreGenGCode()
{
    QStringList gCodeOut;
    gCodeOut << m_Settings.GetStartGCode();
    gCodeOut << m_Settings.GetLaserOnOffCommand().at(1);
    gCodeOut << QString(";filter at %1").arg(m_Settings.GetFilterValue(),0,'i',0);
    gCodeOut << QString("G1F%1").arg(m_Settings.GetFeedrate(),0,'i',0);
    return gCodeOut;
}

QStringList GCodeGenerator::_PrivPostGenGCode()
{
    QStringList gCodeOut;
    gCodeOut << m_Settings.GetEndGCode();
    return gCodeOut;
}

QStringList GCodeGenerator::_GenerateGCodeLineByLine()
{
    QStringList gCodeOut;
    gCodeOut << m_Settings.GetLaserOnOffCommand().at(0);
    QString cmd;
    QImage img = m_image;
    qDebug("Dimension are %d x %d",m_image.width(),m_image.height());
    //Convert to grayscale

    for (int ii = 0; ii < img.height(); ii++) {
        uchar* scan = img.scanLine(ii);
        int depth =4;
        for (int jj = 0; jj < img.width(); jj++) {

            QRgb* rgbpixel = reinterpret_cast<QRgb*>(scan + jj*depth);
            int gray = qGray(*rgbpixel);
            *rgbpixel = QColor(gray, gray, gray).rgba();
        }
    }
    // invert color, since GCode go from 0 (white) to 255 (black)
    // but grayscale go from 0 (black) to 255 (white)
    img.invertPixels(QImage::InvertRgb);

    bool LaserOn = false;
    float res = 0.2f;
    res = m_Settings.GetResolution();

    float CoordX = 0;
    float CoordY = 0;
    double CoordXInMM = 0.00f;
    double CoordYInMM = 0.00f;

    float XSets[3] = {*(m_Settings.GetXSettings()),*(m_Settings.GetXSettings()+1),*(m_Settings.GetXSettings()+2)};
    float YSets[3] = {*(m_Settings.GetYSettings()),*(m_Settings.GetYSettings()+1),*(m_Settings.GetYSettings()+2)};
    float ZSets[3] = {*(m_Settings.GetZSettings()),*(m_Settings.GetZSettings()+1),*(m_Settings.GetZSettings()+2)};
    // Quickly move the head to the offset of X and Y
    float startZVal = m_Settings.GetBaseThicknessValue() + m_Settings.GetMaterialThickness() + m_Settings.GetLensFocus();
    if (startZVal > ZSets[1])
    {

    } else {
        cmd = QString("G1Z%1").arg(startZVal,0,'f',2);
        gCodeOut << cmd;

        cmd = QString("G0X%1Y%2").arg(XSets[2],0,'f',2)
                                   .arg(YSets[2],0,'f',2);
        gCodeOut << cmd;

        CoordXInMM = XSets[2];
        CoordYInMM = YSets[2];

        while ((CoordX < img.width()-1)
               && (CoordXInMM < XSets[1]))
        {
            CoordX = (CoordXInMM - XSets[2]) * m_image.dotsPerMeterX() / 1000;
            while ((CoordY < img.height()-1)
                   && (CoordYInMM < YSets[1]))
            {
                CoordY = (CoordYInMM - YSets[2]) * m_image.dotsPerMeterY() / 1000;
                QRgb rgbpixel;
                if (((int)CoordX >= 0 && (int)CoordX < img.width()) && ((int)CoordY >= 0 && (int)CoordY < img.height()) )
                    rgbpixel = img.pixel((int)CoordX,(int)CoordY);
                float average = 0.0f;
                // get the grayscale, a.k.a the Laser power of the pixel on the scale 0..255
                average = qGray(rgbpixel);
                float LaserPwr = 0.0f;
                qDebug("processing %d x %d",(int)CoordX,(int)CoordY);
                // interpolate to find LaserPwr, rescale average to minLaserPwr..maxLaserPwr
                LaserPwr = interpolate(average,m_Settings.GetMinLaserPwr(),m_Settings.GetMaxLaserPwr());
                if (LaserPwr < m_Settings.GetMinLaserPwr())
                    LaserPwr = m_Settings.GetMinLaserPwr();
                else if (LaserPwr > m_Settings.GetMaxLaserPwr())
                    LaserPwr = m_Settings.GetMaxLaserPwr();

                // We only burn pixel that pass a certain threshold
                if (LaserPwr > m_Settings.GetFilterValue())
                {
                    if (!LaserOn) {
                        // If at the last pixel, the laser is off (which mean the last pixel does not pass the threshold, we do a G0 (quick move)
                        // to the current pixel
                        gCodeOut << QString("G0X%1Y%2").arg(CoordXInMM,0,'f',2).arg(CoordYInMM,0,'f',2);
                        //turn on Laser
                        gCodeOut << m_Settings.GetLaserOnOffCommand().at(0);
                        LaserOn = true;
                    }
                    cmd = QString("G1X%1Y%2S%3").arg(CoordXInMM,0,'f',2).arg(CoordYInMM,0,'f',2).arg(LaserPwr,0,'i',0);
                    gCodeOut << cmd;
                } else {
                    // if the last pixel should be burned, then we turn off the laser and skip this pixel
                    if(LaserOn) {
                        gCodeOut << m_Settings.GetLaserOnOffCommand().at(1);
                        LaserOn = false;
                    }
                }
                CoordYInMM+=res;
            }
            CoordXInMM+=res;
            do {
                CoordY = (CoordYInMM - YSets[2]) * m_image.dotsPerMeterY() / 1000;
                QRgb rgbpixel;
                if (((int)CoordX >= 0 && (int)CoordX < img.width()) && ((int)CoordY >= 0 && (int)CoordY < img.height()))
                    rgbpixel = img.pixel((int)CoordX,(int)CoordY);
                float average = 0.0f;
                average = qGray(rgbpixel);
                float LaserPwr = 0.0f;
                qDebug("processing %d x %d",(int)CoordX,(int)CoordY);
                LaserPwr = interpolate(average,m_Settings.GetMinLaserPwr(),m_Settings.GetMaxLaserPwr());
                if (LaserPwr < m_Settings.GetMinLaserPwr())
                    LaserPwr = m_Settings.GetMinLaserPwr();
                else if (LaserPwr > m_Settings.GetMaxLaserPwr())
                    LaserPwr = m_Settings.GetMaxLaserPwr();
                if (LaserPwr > m_Settings.GetFilterValue())
                {
                    if (!LaserOn) {
                        gCodeOut << QString("G0X%1Y%2").arg(CoordXInMM,0,'f',2).arg(CoordYInMM,0,'f',2);
                        gCodeOut << m_Settings.GetLaserOnOffCommand().at(0);
                        LaserOn = true;
                    }
                    cmd = QString("G1X%1Y%2S%3").arg(CoordXInMM,0,'f',2).arg(CoordYInMM,0,'f',2).arg(LaserPwr,0,'i',0);
                    gCodeOut << cmd;
                } else {
                    if(LaserOn) {
                        gCodeOut <<  m_Settings.GetLaserOnOffCommand().at(1);
                        LaserOn = false;
                    }
                }
                CoordYInMM-=res;
            } while (CoordYInMM - YSets[2] > 0);
            CoordXInMM+=res;
            CoordYInMM = YSets[2];
        }
        if (LaserOn) {
            gCodeOut << m_Settings.GetLaserOnOffCommand().at(1);
        }
    }
    return gCodeOut;
}

QStringList GCodeGenerator::_GenerateGCodeTracingEdge()
{
    QStringList gCodeOut;
    gCodeOut << m_Settings.GetLaserOnOffCommand()[1];
    QString cmd;
    cmd = QString("%1 %2%3").arg(m_Settings.GetLaserOnOffCommand()[0],m_Settings.GetLaserCmd()).arg(m_Settings.GetMaxLaserPwr());
    gCodeOut << cmd;

    QByteArray potrace_install_dir = qgetenv("POTRACE_INSTALL_DIR");
    QString path = !potrace_install_dir.isEmpty() ? QString::fromLatin1(potrace_install_dir) : QDir::currentPath();
    QString fullPathToSavePPM = QDir::tempPath() + "/R3DExportToppm.bmp";
    fullPathToSavePPM = QDir::toNativeSeparators(fullPathToSavePPM);
    QString fullPathToSaveSVG = QDir::tempPath() + "/R3DExportToSVG.svg";
    fullPathToSaveSVG = QDir::toNativeSeparators(fullPathToSaveSVG);

    QImageWriter writer(fullPathToSavePPM, "bmp");
    writer.write(m_image);
    QString program = "";
    program = path + "/tools/potrace/potrace.exe";
    program = QDir::toNativeSeparators(program);
    QStringList arguments;
    arguments.append(fullPathToSavePPM);
    arguments.append(QString("--svg"));
    arguments.append(QString("-o"));
    arguments.append(fullPathToSaveSVG);
    //arguments.append(QString("--tight"));
    arguments.append(QString("--flat"));
    arguments.append(QString("-u"));
    arguments.append(QString("1"));
    /*arguments.append(QString("--color"));
    arguments.append(QString("#ffffff"));
    arguments.append(QString("--fillcolor"));
    arguments.append(QString("#000000"));*/
    //arguments.append(QString("--opaque"));

    QProcess *myProcess = new QProcess(this);
    if (!program.isEmpty()) {
        myProcess->start(program, arguments);
        myProcess->waitForFinished();
        svgReader R3DReader;
        QFile svgout(fullPathToSaveSVG);
        if(svgout.open(QIODevice::ReadOnly)) {
            int ret = R3DReader.read(&svgout,m_Settings);
            if (ret == 0) {
                gCodeOut << R3DReader.getGCodeGenerated();
            }
        }
        svgout.close();
        QFile::remove(fullPathToSavePPM);
        QFile::remove(fullPathToSaveSVG);
    }
    else
    {
        qErrnoWarning("Fatal Error : Can't find potrace!\n" \
                          "Please contact our support at \n" \
                          "www.3draion.com");
    }
    return gCodeOut;
}

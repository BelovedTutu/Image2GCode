#ifndef GCODEGENERATOR_H
#define GCODEGENERATOR_H

#include <QObject>
#include <QImage>
#include "settingshandler.h"

class GCodeGenerator : public QObject
{
    Q_OBJECT
public:
    explicit GCodeGenerator(const QString& in_image_path,
                            const QString& out_path,\
                            const SettingsHandler& in_handler,
                            QObject *parent = nullptr);
    void GenerateGCodeFromImage();
    void UpdateSettings(const SettingsHandler& new_settings) { m_Settings.copy_that(new_settings); }
signals:

public slots:

private:
    float interpolate(float val, int minRange, int maxRange);
    QStringList _PrivGenerateGCodeFromImage();
    QStringList _GenerateGCodeLineByLine();
    QStringList _GenerateGCodeTracingEdge();
    SettingsHandler m_Settings;
    QImage m_image;
    QString m_GCodeOutPath;
};

#endif // GCODEGENERATOR_H

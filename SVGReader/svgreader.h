#ifndef SVGREADER_H
#define SVGREADER_H

#include <QObject>
#include <QXmlStreamReader>
#include <QTimer>
#include <QDoubleSpinBox>
#include "line.h"
#include "circle.h"
#include "qbezier.h"
#include "polygon.h"
#include "transform.h"
#include <QString>
#include <QFile>
#include "arc.h"
#include "basicpolygon.h"
#include "../settingshandler.h"

class svgReader : public QObject
{
    Q_OBJECT
public:
    explicit svgReader(QObject *parent = 0);
    //void openSvg();
    int read(QFile* file, const SettingsHandler& in_handler);
    void readSVG();
    //void updateBoundary(float &w, float&h, float x1, float x2, float y1, float y2);
    void generateGCode( const SettingsHandler& in_handler);
    QStringList getGCodeGenerated() {return m_gCodeGenerate;}
    QVector<double> parseSVGNumbers(QString cmd);
    QList<BasicPolygon*> searchPolygons(BasicPolygon *previous, QList<BasicPolygon*> fullLst);
private:
    QStringList m_gCodeGenerate;
    QXmlStreamReader xml;
    QList<Line*> lineList;
    QList<Circle*> circleList;
    QList<QBezier*> qBezierList;
    QList<BasicPolygon*> polyList;
    QList<Arc*> arcList;
    double elevation;
};

#endif // SVGREADER_H

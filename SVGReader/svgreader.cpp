#include "svgreader.h"

svgReader::svgReader(QObject *parent) :
    QObject(parent)
{
}

int svgReader::read(QFile* file, const SettingsHandler& in_handler)
{
    xml.setDevice(file);
    if (xml.readNextStartElement())
    {
        if(xml.name() == "svg")
        {
            readSVG();
            generateGCode(in_handler);
            return 0;
        }
        else
        {
            //QMessageBox::warning(this, tr("Svg2GCode"),tr("Not a valid SVG file"));
            return 2;
        }
    }
    return -1;
}

void svgReader::readSVG()
{
    lineList.clear();
    circleList.clear();
    qBezierList.clear();
    polyList.clear();
    arcList.clear();


    QStringList commandsUSED;

    float width = 0;
    float height = 0;

    QPointF offset = QPointF(0,0);

    QList< QList<AbstractTransform*> > groupTransformList;

    while (!xml.atEnd() && !xml.hasError())
    {

        QString name = xml.name().toString();
        if (xml.isStartElement())
        {
            qDebug() << "Cname " << name;
            if (name == "line")
            {
                //qDebug() << "element name: '" << name;
                QXmlStreamAttributes attrib = xml.attributes();

                QVector<QPointF> points;

                points << QPointF( attrib.value("x1").toDouble(), attrib.value("y1").toDouble()  );
                points << QPointF( attrib.value("x2").toDouble(), attrib.value("y2").toDouble() );

                Polygon * poly = new Polygon(points,false);



                //qDebug() << "line: " << line->p1() << line->p2();
                //lineList.push_back(line);

                QList<AbstractTransform*> elementTransformList;
                Transform trans;
                if(attrib.hasAttribute("transform"))
                {
                    elementTransformList = trans.getTransforms( attrib.value("transform").toString() );
                }
                QList< QList< AbstractTransform * > > tempTransformList;
                tempTransformList.append( groupTransformList );
                tempTransformList.push_back( elementTransformList );
                poly->applyTransformations( tempTransformList );

                polyList.push_back( poly );



            }
            else if(name == "g")
            {
                qDebug() << "God dammit, a group";
                QXmlStreamAttributes attrib = xml.attributes();
                QList<AbstractTransform*> elementTransformList;
                Transform trans;
                if(attrib.hasAttribute("transform"))
                {
                    elementTransformList = trans.getTransforms( attrib.value("transform").toString() );
                }

                groupTransformList.push_back( elementTransformList );

            }
            else if(name == "polygon")
            {
                QXmlStreamAttributes attrib = xml.attributes();
                QString pointsStr = attrib.value("points").toString();
                pointsStr.replace(",", " ");
                QVector<double> pointLst = parseSVGNumbers( pointsStr );
                if(pointLst.size() % 2 == 0) //Is the number of cooridnates even?
                {
                    QList<AbstractTransform*> elementTransformList;
                    Transform trans;
                    if(attrib.hasAttribute("transform"))
                    {
                        elementTransformList = trans.getTransforms( attrib.value("transform").toString() );
                    }

                    QVector<QPointF> points;
                    for(int i = 0; i<pointLst.size(); i+=2)
                    {
                        points.push_back( QPointF(pointLst[i], pointLst[i+1]) );
                    }

                    Polygon* poly = new Polygon(points);
                    QList< QList< AbstractTransform * > > tempTransformList;
                    tempTransformList.append( groupTransformList );
                    tempTransformList.push_back( elementTransformList );
                    poly->applyTransformations( tempTransformList );
                    polyList.push_back( poly );

                    qDebug() << "Polygon:" << pointsStr << points;
                }
                else
                {
                    qDebug() << "Error, odd number of coordinates for polygon: " << pointLst;
                }
            }
            else if(name == "polyline")
            {
                QXmlStreamAttributes attrib = xml.attributes();
                QString pointsStr = attrib.value("points").toString();
                pointsStr.replace(",", " ");
                QVector<double> pointLst = parseSVGNumbers( pointsStr );
                if(pointLst.size() % 2 == 0) //Is the number of cooridnates even?
                {
                    QList<AbstractTransform*> elementTransformList;
                    Transform trans;
                    if(attrib.hasAttribute("transform"))
                    {
                        elementTransformList = trans.getTransforms( attrib.value("transform").toString() );
                    }


                    QVector<QPointF> points;
                    for(int i = 0; i<pointLst.size(); i+=2)
                    {
                        points.push_back( QPointF(pointLst[i], pointLst[i+1]) );
                    }

                    Polygon* poly = new Polygon(points, false);
                    QList< QList< AbstractTransform * > > tempTransformList;
                    tempTransformList.append( groupTransformList );
                    tempTransformList.push_back( elementTransformList );
                    poly->applyTransformations( tempTransformList );
                    polyList.push_back( poly );

                    //qDebug() << "Polygon:" << pointsStr << points;
                }
                else
                {
                    qDebug() << "Error, odd number of coordinates for polygon: " << pointLst;
                }
            }
            else if(name == "ellipse")
            {
                //qDebug() << "element name: '" << name;
                QXmlStreamAttributes attrib = xml.attributes();
                float rx = attrib.value("rx").toFloat();
                float ry = attrib.value("rx").toFloat();

                QList<AbstractTransform*> elementTransformList;
                Transform trans;
                if(attrib.hasAttribute("transform"))
                {
                    elementTransformList = trans.getTransforms( attrib.value("transform").toString() );
                }

                /*Circle *circle = new Circle(attrib.value("cx").toFloat()-diameter/2, attrib.value("cy").toFloat()-diameter/2,
                            diameter,diameter);*/
                Circle *ellipse = new Circle(
                            QPointF(qAbs(attrib.value("cx").toDouble()), qAbs(attrib.value("cy").toDouble())),
                            24,
                            rx,
                            ry
                            );

                QList< QList< AbstractTransform * > > tempTransformList;
                tempTransformList.append( groupTransformList );
                tempTransformList.push_back( elementTransformList );
                ellipse->applyTransformations( tempTransformList );

                polyList.push_back(ellipse);
            }
            else if(name == "path")
            {
                //See this: http://www.w3.org/TR/SVG/paths.html#PathDataEllipticalArcCommands
                QXmlStreamAttributes attrib = xml.attributes();
                QString d = attrib.value("d").toString();
                qDebug( )  << "path: " << d << d.length();
                QStringList cmds;
                int i = 0;
                while(i < d.length())
                {

                    if(d[i] >= QChar('A') && d[i] != QChar('e')) //Captial A in ASCII
                    {
                        QString temp;
                        do
                        {
                            if(d[i] != ',')
                                temp.push_back( d[i] );
                            else
                                temp.push_back( ' ' );
                            i++;
                        } while(i < d.length() && (d[i] < QChar('A') || d[i] == QChar('e')) );
                        i--;
                        cmds << temp;
                    }
                    i++;
                }

                qDebug() << "CMDS" << cmds;

                qDebug() << "Constructing list of transform";
                QList<AbstractTransform*> elementTransformList;
                Transform trans;
                if(attrib.hasAttribute("transform"))
                {
                    elementTransformList = trans.getTransforms( attrib.value("transform").toString() );
                }
                QList< QList< AbstractTransform * > > tempTransformList;
                tempTransformList.append( groupTransformList );
                tempTransformList.push_back( elementTransformList );

                qDebug() << "TRANSFORMS: " << tempTransformList.size();
                foreach(QList<AbstractTransform*> at, tempTransformList)
                {
                    foreach(AbstractTransform* t, at)
                        qDebug() << "T: " << t->type();
                }

                qDebug() << "DONE.";

                QPointF cPos = QPointF(0,0); //CURRENT POSITION
                QPointF zPos = QPointF(0,0); //Position of CLOSE PATH, used for Z command
                QPointF sPos = QPointF(0,0); //Position for S/s command, the second point of previous curve.
                foreach(QString cmd, cmds)
                {
                    qDebug() << cmd[0];
                    commandsUSED.push_back( cmd.mid(0,1) );
                    if(cmd[0] == 'M') //Absolute MOVE
                    {
                        // If a moveto is followed by multiple pairs of coordinates,
                        //the subsequent pairs are treated as implicit lineto commands.
                        qDebug() << "M" << parseSVGNumbers(cmd);
                        QVector<double> num = parseSVGNumbers(cmd);
                        for(int i=0; i<num.size(); i+=2)
                        {
                            if(i == 0) //If it's only first move, don't draw
                            {
                                cPos = QPointF(num[i], num[i+1]);
                                zPos = QPointF(num[i], num[i+1]);
                            }
                            else //Multiple moves = lineto command
                            {
                                QVector<QPointF> points;
                                points << cPos << QPointF(num[i], num[i+1]);
                                Polygon *poly = new Polygon(points, false);
                                cPos =  QPointF(num[i], num[i+1]);
                                //Apperantly, only the first point counts as new Z value.
                                //Couldn't find that in documentation, though. Strange.
                                //zPos = cPos;

                                    poly->applyTransformations( tempTransformList );

                                polyList.push_back( poly );
                            }
                        }
                    }
                    if(cmd[0] == 'm') //Relative move
                    {
                        // If a moveto is followed by multiple pairs of coordinates,
                        //the subsequent pairs are treated as implicit lineto commands.
                        qDebug() << "m" << parseSVGNumbers(cmd);
                        QVector<double> num = parseSVGNumbers(cmd);
                        for(int i=0; i<num.size(); i+=2)
                        {
                            if(i == 0) //If it's only first move, don't draw
                            {
                                cPos = cPos + QPointF(num[i], num[i+1]);
                                zPos = cPos;
                            }
                            else //Multiple moves = lineto command
                            {
                                QVector<QPointF> points;
                                points << cPos <<cPos + QPointF(num[i], num[i+1]);
                                Polygon *poly = new Polygon(points, false);
                                cPos =  cPos + QPointF(num[i], num[i+1]);
                                //zPos = cPos;

                                    poly->applyTransformations( tempTransformList );
                                polyList.push_back( poly );
                            }
                        }
                    }
                    if(cmd[0] == 'H') //Absolute horizontal line
                    {
                        qDebug() << "H" << parseSVGNumbers(cmd);
                        QVector<double> num = parseSVGNumbers(cmd);
                        for(int i=0; i<num.size(); i+=1)
                        {
                            QVector<QPointF> points;
                            points << cPos <<  QPointF(num[i], cPos.y());
                            Polygon *poly = new Polygon(points, false);
                            cPos =  QPointF(num[i], cPos.y());

                                poly->applyTransformations( tempTransformList );
                            polyList.push_back( poly );
                        }
                    }
                    if(cmd[0] == 'h') //relative horizontal line
                    {
                        qDebug() << "h" << parseSVGNumbers(cmd);
                        QVector<double> num = parseSVGNumbers(cmd);
                        for(int i=0; i<num.size(); i+=1)
                        {
                            QVector<QPointF> points;
                            points << cPos <<cPos + QPointF(num[i],0);
                            Polygon *poly = new Polygon(points, false);
                            cPos =  cPos + QPointF(num[i], 0);

                                poly->applyTransformations( tempTransformList );
                            polyList.push_back( poly );
                        }
                    }
                    if(cmd[0] == 'V') //Absolute vertical line
                    {
                        qDebug() << "V" << parseSVGNumbers(cmd);
                        QVector<double> num = parseSVGNumbers(cmd);
                        for(int i=0; i<num.size(); i+=1)
                        {
                            QVector<QPointF> points;
                            points << cPos <<  QPointF(cPos.x(), num[i]);
                            Polygon *poly = new Polygon(points, false);
                            cPos =  QPointF(cPos.x() , num[i]);

                                poly->applyTransformations( tempTransformList );
                            polyList.push_back( poly );
                        }
                    }
                    if(cmd[0] == 'v') //relative vertical line
                    {
                        qDebug() << "v" << parseSVGNumbers(cmd);
                        QVector<double> num = parseSVGNumbers(cmd);
                        for(int i=0; i<num.size(); i+=1)
                        {
                            QVector<QPointF> points;
                            points << cPos <<cPos + QPointF(0 , num[i]);
                            Polygon *poly = new Polygon(points, false);
                            cPos =  cPos + QPointF(0, num[i]);

                                poly->applyTransformations( tempTransformList );
                            polyList.push_back( poly );
                        }
                    }
                    if(cmd[0] == 'A')
                    {
                        qDebug() << "A" << parseSVGNumbers(cmd);
                        QVector<double> num = parseSVGNumbers(cmd);
                        for(int i=0; i<num.size(); i+=7)
                        {
                            Arc * arc = new Arc(QPointF(num[i], num[i+1]), num[i+2], num[i+3], num[i+4],
                                    QPointF(num[i+5], num[i+6]), cPos);
                            cPos =  QPointF(num[i+5], num[i+6]);


                                arc->applyTransformations( tempTransformList );

                            //arcList.push_back( arc );
                                polyList.push_back( arc );
                        }
                    }
                    if(cmd[0] == 'a')
                    {
                        qDebug() << "a" << parseSVGNumbers(cmd);
                        QVector<double> num = parseSVGNumbers(cmd);
                        for(int i=0; i<num.size(); i+=7)
                        {
                            Arc * arc = new Arc(QPointF(num[i], num[i+1]), num[i+2], num[i+3], num[i+4],
                                    cPos+QPointF(num[i+5], num[i+6]), cPos);
                            cPos =  cPos + QPointF(num[i+5], num[i+6]);


                                arc->applyTransformations( tempTransformList );

                            //arcList.push_back( arc );
                                polyList.push_back( arc );
                        }
                    }
                    if(cmd[0] == 'L') //Line to Absolute
                    {
                        qDebug() << "L" << parseSVGNumbers(cmd);
                        QVector<double> num = parseSVGNumbers(cmd);
                        for(int i=0; i<num.size(); i+=2)
                        {
                            QVector<QPointF> points;
                            points << cPos <<  QPointF(num[i], num[i+1]);
                            Polygon *poly = new Polygon(points, false);
                            cPos =  QPointF(num[i], num[i+1]);


                                poly->applyTransformations( tempTransformList );

                            polyList.push_back( poly );
                        }
                    }
                    if(cmd[0] == 'l') // line to relative
                    {
                        qDebug() << "l" << parseSVGNumbers(cmd);
                        QVector<double> num = parseSVGNumbers(cmd);
                        for(int i=0; i<num.size(); i+=2)
                        {
                            QVector<QPointF> points;
                            points << cPos <<cPos + QPointF(num[i], num[i+1]);
                            Polygon *poly = new Polygon(points, false);
                            cPos =  cPos + QPointF(num[i], num[i+1]);


                                poly->applyTransformations( tempTransformList );

                            polyList.push_back( poly );
                        }
                    }
                    if(cmd[0] == 'C') //Absolute cubic Bezier curve
                    {
                        //I think bezier curve might be auto filled, basically automaticaly Z cmd is initiated
                        qDebug() << "C" << parseSVGNumbers(cmd);
                        QVector<double> num = parseSVGNumbers(cmd);
                        for(int i=0; i<num.size(); i+=6)
                        {
                            /*cPos = QPointF(num[i], num[i+1]);
                            zPos = QPointF(num[i], num[i+1]);*/
                            QVector<QPointF> points;
                            points << cPos << QPointF(num[i], num[i+1]) << QPointF(num[i+2], num[i+3]) << QPointF(num[i+4], num[i+5]);
                            QBezier *bezier = new QBezier(points, 32);
                            cPos =  QPointF(num[i+4], num[i+5]);
                            sPos = QPointF( num[i+2], num[i+3] );


                                bezier->applyTransformations( tempTransformList );

                            //qBezierList.push_back( bezier );
                                polyList.push_back( bezier );


                        }
                    }
                    if(cmd[0] == 'c') //Relative cubic Bezier curve
                    {
                        //I think bezier curve might be auto filled, basically automaticaly Z cmd is initiated
                        qDebug() << "c" << parseSVGNumbers(cmd);
                        QVector<double> num = parseSVGNumbers(cmd);
                        for(int i=0; i<num.size(); i+=6)
                        {
                            qDebug() << cPos;
                            /*cPos = QPointF(num[i], num[i+1]);
                            zPos = QPointF(num[i], num[i+1]);*/
                            QVector<QPointF> points;
                            points << cPos
                                   << cPos + QPointF(num[i], num[i+1])
                                   << cPos + QPointF(num[i+2], num[i+3])
                                   << cPos + QPointF(num[i+4], num[i+5]);

                            QBezier *bezier = new QBezier(points, 32);


                                bezier->applyTransformations( tempTransformList );

                            //qBezierList.push_back( bezier );
                            polyList.push_back( bezier );

                            cPos = cPos + QPointF(num[i+4], num[i+5]);
                            sPos = cPos + QPointF( num[i+2], num[i+3] );

                        }
                    }
                    if(cmd[0] == 'S') //Absolute shorthand cubic Bezier curve
                    {
                        //I think bezier curve might be auto filled, basically automaticaly Z cmd is initiated
                        qDebug() << "S" << parseSVGNumbers(cmd);
                        QVector<double> num = parseSVGNumbers(cmd);
                        for(int i=0; i<num.size(); i+=4)
                        {
                            qDebug() << cPos;
                            /*cPos = QPointF(num[i], num[i+1]);
                            zPos = QPointF(num[i], num[i+1]);*/
                            QVector<QPointF> points;
                            points << cPos
                                   << 2*cPos - sPos
                                   << QPointF(num[i], num[i+1])
                                   << QPointF(num[i+2], num[i+3]);

                            QBezier *bezier = new QBezier(points, 32);



                                bezier->applyTransformations( tempTransformList );

                            //qBezierList.push_back( bezier );
                                polyList.push_back( bezier );

                            sPos = QPointF(num[i], num[i+1]);
                            cPos = QPointF( num[i+2], num[i+3] );

                        }
                    }
                    if(cmd[0] == 's') //relative shorthand cubic Bezier curve
                    {
                        //I think bezier curve might be auto filled, basically automaticaly Z cmd is initiated
                        qDebug() << "s" << parseSVGNumbers(cmd);
                        QVector<double> num = parseSVGNumbers(cmd);
                        for(int i=0; i<num.size(); i+=4)
                        {
                            qDebug() << cPos;
                            /*cPos = QPointF(num[i], num[i+1]);
                            zPos = QPointF(num[i], num[i+1]);*/
                            QVector<QPointF> points;
                            points << cPos
                                   << 2*cPos - sPos
                                   << cPos + QPointF(num[i], num[i+1])
                                   << cPos + QPointF(num[i+2], num[i+3]);

                            QBezier *bezier = new QBezier(points, 32);


                                bezier->applyTransformations( tempTransformList );

                            //qBezierList.push_back( bezier );
                                polyList.push_back( bezier );

                            sPos = cPos + QPointF(num[i], num[i+1]);
                            cPos = cPos + QPointF( num[i+2], num[i+3] );

                        }
                    }
                    if(cmd[0] == 'z' || cmd[0] == 'Z')
                    {
                        qDebug() << "z";
                        QVector<QPointF> points;
                        points << cPos <<zPos;
                        Polygon *poly = new Polygon(points, false);

                        //Line * line = new Polygon(cPos, zPos); //Needs width at some point
                        cPos = zPos;


                            poly->applyTransformations( tempTransformList );

                        polyList.push_back( poly );
                        //this->lineList.push_back( line );
                    }
                }

            }
            else if (name == "circle")
            {
                //qDebug() << "element name: '" << name;
                QXmlStreamAttributes attrib = xml.attributes();
                float radius = attrib.value("r").toFloat();
                /*Circle *circle = new Circle(attrib.value("cx").toFloat()-diameter/2, attrib.value("cy").toFloat()-diameter/2,
                            diameter,diameter);*/
                Circle *circle = new Circle(
                            QPointF(qAbs(attrib.value("cx").toDouble()), qAbs(attrib.value("cy").toDouble())),
                            24,
                            radius,
                            radius
                            );

                QList<AbstractTransform*> elementTransformList;
                Transform trans;
                if(attrib.hasAttribute("transform"))
                {
                    elementTransformList = trans.getTransforms( attrib.value("transform").toString() );
                }

                QList< QList< AbstractTransform * > > tempTransformList;
                tempTransformList.append( groupTransformList );
                tempTransformList.push_back( elementTransformList );
                circle->applyTransformations( tempTransformList );

                polyList.push_back( circle );

            }
        }
        else if( xml.isEndElement() )
        {
            if(name == "g")
            {
                qDebug() << "God dammit, an END of group - ";
                groupTransformList.removeLast();
            }
        }

        //qDebug() << "ACTUAL XML ELEMENT: " << xml.name();
        xml.readNext();
    }

    if (xml.hasError())
    {
        qDebug() << "XML error: " << xml.errorString() << endl;
    }
    else if (xml.atEnd())
    {
        qDebug() << "Reached end, done" << endl;


        height  *= -1;

        /*foreach(Line* line, lineList)
        {
            line->setPoints( QPointF(line->p1().x(), height+line->p1().y()),
                             QPointF(line->p2().x(), height+line->p2().y()));
        }*/

        /*foreach(Circle* circle, circleList)
        {
            circle->setRect( circle->x(), height+circle->y(), circle->width(), circle->height() );
        }*/
    }
}

QVector<double> svgReader::parseSVGNumbers(QString cmd)
{
    cmd.replace(QRegExp("[a-df-zA-DF-Z]"), "");
    cmd.replace("-", " -");
    cmd.replace( "e -", "e-" );
    //qDebug( ) << "COMMAND " << cmd;
    cmd = cmd.trimmed();
    cmd = cmd.simplified();

    QStringList numStr = cmd.split(" ");
    QVector<double> num;
    foreach(QString str, numStr)
    {
        //num.push_back( qAbs(str.toDouble()) ); // <-- honestly that was stupido.
            num.push_back( str.toDouble() );
    }

    return num;
}


void svgReader::generateGCode(const SettingsHandler& in_handler)
{
    QStringList gcode;

    float maxZ = in_handler.GetMaterialThickness() + in_handler.GetLensFocus();
    float curZ = maxZ;
    //bool laserOn = true;
    while (curZ >= (maxZ - in_handler.GetMaterialThickness()))
    {
        foreach(QLineF *line, lineList)
        {
            gcode << QString("G0 X%1 Y%2 F%3").arg(line->x1() > 0 ? line->x1() : 0,0, 'f', 3).arg(line->y1() > 0 ? line->y1() : 0,0, 'f', 3).arg(in_handler.GetFeedrate(),0,'d',3);
            gcode << QString("G1 Z%1 F%2").arg(curZ,0, 'f', 3).arg(in_handler.GetFeedrate(),0,'d',3);
            gcode << QString("%1 %2%3").arg(in_handler.GetLaserOnOffCommand()[0]).arg(in_handler.GetLaserCmd()).arg(in_handler.GetMaxLaserPwr());
            gcode << QString("G1 X%1 Y%2 F%3").arg(line->x2() > 0 ? line->x2() : 0,0, 'f', 3).arg(line->y2() > 0 ? line->y2() : 0,0, 'f', 3).arg(in_handler.GetFeedrate(),0,'d',3);
            gcode << QString("%1").arg(in_handler.GetLaserOnOffCommand()[1]);
            gcode << QString("G1 Z%1 F%2").arg(*(in_handler.GetZSettings()+1),0, 'f', 3).arg(in_handler.GetFeedrate(),0,'d',3);
        }

        QList< QList<BasicPolygon*> > optimizedPoly;

        QList<BasicPolygon*> tempPoly = polyList;
        int polyCount = 0;
        while(tempPoly.size())
        {
            QList<BasicPolygon*> tmpPolyLst;

            tmpPolyLst = searchPolygons( tempPoly.first(), tempPoly );

            optimizedPoly.append(tmpPolyLst  );

            foreach(BasicPolygon* poly, tmpPolyLst)
            {
                tempPoly.removeOne( poly );
                polyCount++;
            }


        }

        foreach(QList<BasicPolygon*> list, optimizedPoly)
        {
            gcode << QString("G0 X%1 Y%2 F%3").arg(list.first()->getPolygon().at(0).x() > 0 ? list.first()->getPolygon().at(0).x() : 0 ,0, 'f', 3).arg(list.first()->getPolygon().at(0).y() > 0 ? list.first()->getPolygon().at(0).y() : 0,0, 'f', 3).arg(in_handler.GetFeedrate(),0,'d',3);
            gcode << QString("G1 Z%1 F%2").arg(curZ,0, 'f', 3).arg(in_handler.GetFeedrate(),0,'d',3);
            gcode << QString("%1 %2%3").arg(in_handler.GetLaserOnOffCommand()[0]).arg(in_handler.GetLaserCmd()).arg(in_handler.GetMaxLaserPwr());
            foreach(BasicPolygon * poly , list)
            {
                QPolygonF polygon = poly->getPolygon();

                for(int i=1; i<polygon.size(); i++)
                {
                    gcode << QString("G1 X%1 Y%2 F%3").arg(polygon[i].x() > 0 ? polygon[i].x() : 0,0, 'f', 3).arg(polygon[i].y() > 0 ? polygon[i].y() : 0,0, 'f', 3).arg(in_handler.GetFeedrate(),0,'d',3);
                }
            }
            gcode << QString("%1").arg(in_handler.GetLaserOnOffCommand()[1]);
            gcode << QString("G1 Z%1 F%2").arg(*(in_handler.GetZSettings()+1),0, 'f', 3).arg(in_handler.GetFeedrate(),0,'d',3);
        }



        foreach(Circle * circle , circleList)
        {
            QPolygonF polygon = circle->getPolygon();
            gcode << QString("G0 X%1 Y%2 F%3").arg(polygon[0].x() > 0 ? polygon[0].x() : 0,0, 'f', 3).arg(polygon[0].y() > 0 ? polygon[0].y() : 0,0, 'f', 3).arg(in_handler.GetFeedrate(),0,'d',3);
            gcode << QString("G1 Z%1 F%2").arg(curZ,0, 'f', 3).arg(in_handler.GetFeedrate(),0,'d',3);
            gcode << QString("%1 %2%3").arg(in_handler.GetLaserOnOffCommand()[0]).arg(in_handler.GetLaserCmd()).arg(in_handler.GetMaxLaserPwr());
            for(int i=1; i<polygon.size(); i++)
            {
                gcode << QString("G1 X%1 Y%2 F1800.000").arg(polygon[i].x() > 0 ? polygon[i].x() : 0,0, 'f', 3).arg(polygon[i].y() > 0 ? polygon[i].y() : 0,0, 'f', 3);
            }
            gcode << QString("%1").arg(in_handler.GetLaserOnOffCommand()[1]);
            gcode << QString("G1 Z%1 F%2").arg(*(in_handler.GetZSettings()+1),0, 'f', 3).arg(in_handler.GetFeedrate(),0,'d',3);
        }
        curZ -= in_handler.GetResolution();
    }

    m_gCodeGenerate = gcode;
}

QList<BasicPolygon*> svgReader::searchPolygons(BasicPolygon *previous, QList<BasicPolygon*> fullLst)
{
    QList<BasicPolygon*> lst;
    lst.append(previous);
    fullLst.removeOne( previous );
    foreach(BasicPolygon* poly, fullLst)
    {
        if(previous->getPolygon().last() == poly->getPolygon().first())
        {
            lst.append( searchPolygons(poly, fullLst) );
            break;
        }
    }
    return lst;
}

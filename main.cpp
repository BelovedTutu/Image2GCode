#include <QCoreApplication>
#include <QCommandLineParser>

#include "settingshandler.h"
#include "gcodegenerator.h"

#define DEFAULT_SETTINGS "./settings.xml"
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("Image2GCode");
    QCoreApplication::setApplicationVersion("Alpha 1.0");

    QCommandLineParser option_parser;
    option_parser.setApplicationDescription("Description: An application to convert an image to a gcode for laser/CNC");
    option_parser.addHelpOption();
    option_parser.addVersionOption();

    QCommandLineOption input_file(QStringList() << "i" << "input","Options for an input file","main: input option");
    option_parser.addOption(input_file);

    QCommandLineOption output_file(QStringList() << "o" << "output","Options for an outputxml file","main: output option");
    option_parser.addOption(output_file);

    QCommandLineOption settings_file(QStringList() << "s" << "settings","Options for a xml setting file","main: setting option");
    option_parser.addOption(settings_file);

    option_parser.process(a);
    QString in_file_path;
    QString out_file_path;
    QString settings_path;
    const QStringList args = option_parser.optionNames();
    if (argc < 3) {
        fprintf(stderr,"%s\n",qPrintable(QCoreApplication::translate("main","Error: lack of arguments to work with")));
        return -1;
    } else if (argc < 4) {
    	in_file_path = option_parser.value(input_file);
        out_file_path = option_parser.value(output_file);
        settings_path = DEFAULT_SETTINGS;
    } else {
        in_file_path = option_parser.value(input_file);
        out_file_path = option_parser.value(output_file);
        settings_path = option_parser.value(settings_file);
    }
    SettingsHandler settings_handler;
    int iRet = settings_handler.ImportSettingsFromXml(settings_path);
    if (iRet != 0) return iRet;
    GCodeGenerator gen(in_file_path,out_file_path,settings_handler);
    gen.GenerateGCodeFromImage();
    return 0;
}

#include "inferencelaunchspec.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <cassert>

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir root;
    assert(root.isValid());
    QDir dir(root.path());
    assert(dir.mkpath("model/RK3588"));
    QFile program(dir.filePath("rknn_yolov8_demo"));
    assert(program.open(QIODevice::WriteOnly));
    program.write("test"); program.close();
    program.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    QFile model(dir.filePath("model/RK3588/model.rknn"));
    assert(model.open(QIODevice::WriteOnly)); model.write("model"); model.close();
    QTemporaryDir appDir;
    QFile ini(QDir(appDir.path()).filePath("single_model.ini"));
    assert(ini.open(QIODevice::WriteOnly | QIODevice::Text));
    ini.write("[Inference]\n");
    ini.write("rknn_dir=" + root.path().toUtf8() + "\n");
    ini.write("program=rknn_yolov8_demo\n");
    ini.write("model=model/RK3588/model.rknn\n");
    ini.close();

    const LaunchSpecResult result = buildInferenceLaunchSpec(
        InferenceProfile::SingleModel, appDir.path());
    assert(result.ok);
    assert(result.spec.program == program.fileName());
    assert(result.spec.arguments.size() == 3);
    assert(result.spec.arguments[0] == model.fileName());
    assert(result.spec.arguments[1] == "/dev/video41");
    assert(result.spec.arguments[2] == "12");
}

#ifndef INFERENCELAUNCHSPEC_H
#define INFERENCELAUNCHSPEC_H
#include "inferenceprofile.h"
#include <QString>
#include <QStringList>

struct InferenceLaunchSpec {
    QString workingDirectory;
    QString program;
    QStringList arguments;
};
struct LaunchSpecResult {
    bool ok = false;
    InferenceLaunchSpec spec;
    QString error;
};
LaunchSpecResult buildInferenceLaunchSpec(
    InferenceProfile profile, const QString& applicationDirectory);
#endif

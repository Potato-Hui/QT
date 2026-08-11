#include "monitorapplication.h"
#include "inferenceprofile.h"

int main(int argc, char *argv[])
{
    return runMonitorApplication(argc, argv, InferenceProfile::SingleModel);
}

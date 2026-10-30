#ifndef QT_INSULATOR_QUANTIFIER_HPP
#define QT_INSULATOR_QUANTIFIER_HPP

#include "InsulatorQuantifier.hpp"

#include <QByteArray>
#include <QFile>
#include <QString>

#include <string>

namespace insulator {

class QtInsulatorQuantifier {
public:
    QuantificationResult processFromFiles(
        const QString& image_path,
        const QString& metadata_path,
        const QString& calibration_path,
        const QString& result_path,
        bool verbose = false,
        bool reuse_metadata_geometry = false) const {
        return quantifier_.processFromFiles(
            localPath(image_path),
            localPath(metadata_path),
            localPath(calibration_path),
            localPath(result_path),
            verbose,
            reuse_metadata_geometry);
    }

private:
    static std::string localPath(const QString& path) {
        const QByteArray encoded = QFile::encodeName(path);
        return std::string(encoded.constData(), static_cast<std::size_t>(encoded.size()));
    }

    InsulatorQuantifier quantifier_;
};

}  // namespace insulator

#endif

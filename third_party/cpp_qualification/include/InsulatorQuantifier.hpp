#ifndef INSULATOR_QUANTIFIER_HPP
#define INSULATOR_QUANTIFIER_HPP

#include <opencv2/core.hpp>

#include <limits>
#include <string>
#include <vector>

namespace insulator {

struct CalibrationModel {
    std::string version;
    std::string feature_name;
    std::string method;
    bool valid = false;
    double slope = std::numeric_limits<double>::quiet_NaN();
    double intercept = std::numeric_limits<double>::quiet_NaN();
};

struct DefectMask {
    cv::Mat mask;
    int instance_id = 0;
    int class_id = -1;
    std::string class_name;
    double confidence = std::numeric_limits<double>::quiet_NaN();
};

struct DiscResult {
    int id = 0;
    int instance_id = 0;
    int class_id = 0;
    std::string class_name = "insulator_disc";
    double confidence = std::numeric_limits<double>::quiet_NaN();
    std::string mask_quality;
    std::string geometry_validity;
    std::string local_failure;
    bool reference_eligible = false;
    bool frozen_geometry_present = false;
    double frozen_axis_position = std::numeric_limits<double>::quiet_NaN();
    double frozen_major_span = std::numeric_limits<double>::quiet_NaN();
    bool frozen_multi_axis_measurement_valid = false;
    double frozen_pose_ratio_q75 = std::numeric_limits<double>::quiet_NaN();
    double centroid_x = std::numeric_limits<double>::quiet_NaN();
    double centroid_y = std::numeric_limits<double>::quiet_NaN();
    double axis_position = std::numeric_limits<double>::quiet_NaN();
    double major_span = std::numeric_limits<double>::quiet_NaN();
    bool multi_axis_measurement_valid = false;
    double pose_ratio_q75 = std::numeric_limits<double>::quiet_NaN();
    double predicted_major_span = std::numeric_limits<double>::quiet_NaN();
    double coefficient = std::numeric_limits<double>::quiet_NaN();
    double ideal_area = std::numeric_limits<double>::quiet_NaN();
    int crack_area = 0;
    int pollution_area = 0;
    int flashover_area = 0;
    int broken_area = 0;
    double crack_ratio = 0.0;
    double pollution_ratio = 0.0;
    double flashover_ratio = 0.0;
    double broken_ratio = 0.0;
    double basic_damage_index = 0.0;
    double conservative_risk_index = 0.0;
};

struct QuantificationResult {
    double basic_mean = 0.0;
    double basic_max = 0.0;
    double basic_damage_index = 0.0;
    double conservative_mean = 0.0;
    double conservative_max = 0.0;
    double conservative_risk_index = 0.0;
    std::string overall_level;
    std::string recommendation;
    int highest_risk_disc_id = 0;
    int disc_count = 0;
    int defect_instance_count = 0;
    std::vector<DiscResult> discs;
};

class InsulatorQuantifier {
public:
    QuantificationResult processFromFiles(
        const std::string& image_path,
        const std::string& metadata_path,
        const std::string& calibration_path,
        const std::string& result_path,
        bool verbose = false,
        bool reuse_metadata_geometry = false) const;

    QuantificationResult processMasks(
        const std::vector<cv::Mat>& disc_masks,
        const std::vector<DiscResult>& disc_metadata,
        const std::vector<DefectMask>& defect_masks,
        const CalibrationModel& calibration,
        bool reuse_metadata_geometry = false) const;

private:
    void computeGating(std::vector<DiscResult>& discs,
                       const std::vector<cv::Mat>& disc_masks) const;
    void computeIdealAreas(std::vector<DiscResult>& discs,
                           std::vector<cv::Mat>& disc_masks,
                           const CalibrationModel& calibration,
                           bool reuse_metadata_geometry) const;
    QuantificationResult evaluateDamage(
        const std::vector<cv::Mat>& disc_masks,
        const std::vector<DiscResult>& discs,
        const std::vector<DefectMask>& defect_masks) const;
};

}  // namespace insulator

#endif

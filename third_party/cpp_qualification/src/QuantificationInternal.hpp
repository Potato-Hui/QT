#ifndef QUANTIFICATION_INTERNAL_HPP
#define QUANTIFICATION_INTERNAL_HPP

#include "InsulatorQuantifier.hpp"

#include <opencv2/core.hpp>

#include <cstddef>
#include <string>
#include <vector>

namespace insulator {
namespace detail {

struct BrokenAssignment {
    std::size_t defect_index = 0;
    int source_broken_instance_id = 0;
    int target_instance_id = 0;
    bool virtual_disc = false;
    cv::Point2d centroid;
    cv::Rect bbox;
    int intersection_area = 0;
    double intersection_ratio = 0.0;
    std::string assignment_mode;
};

struct BrokenAssociation {
    cv::Size image_size;
    std::vector<cv::Point2d> disc_centroids;
    std::vector<BrokenAssignment> assignments;
};

BrokenAssociation assignBrokenToDisc(
    const std::vector<cv::Mat>& disc_masks,
    const std::vector<DiscResult>& disc_metadata,
    const std::vector<DefectMask>& defect_masks);

void excludeBrokenReferences(
    std::vector<DiscResult>& discs,
    const BrokenAssociation& association);

void logFinalDiscList(
    const std::vector<DiscResult>& discs,
    const BrokenAssociation& association);

bool applyBrokenIdealAreas(
    std::vector<DiscResult>& discs,
    std::vector<cv::Mat>& disc_masks,
    const std::vector<DefectMask>& defect_masks,
    const CalibrationModel& calibration,
    const BrokenAssociation& association,
    bool theil_sen_available);

QuantificationResult evaluateDamageWithBroken(
    const std::vector<cv::Mat>& disc_masks,
    const std::vector<DiscResult>& discs,
    const std::vector<DefectMask>& defect_masks,
    const BrokenAssociation& association,
    bool no_reference_degraded);

}  // namespace detail
}  // namespace insulator

#endif

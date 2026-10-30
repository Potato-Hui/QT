#include "QuantificationInternal.hpp"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <limits>
#include <iostream>
#include <set>
#include <stdexcept>

namespace {

cv::Mat binaryMask(const cv::Mat& mask) {
    cv::Mat binary;
    cv::compare(mask, 0, binary, cv::CMP_GT);
    return binary;
}

cv::Point2d centroid(const cv::Mat& mask, const char* error) {
    const cv::Moments moments = cv::moments(mask, true);
    if (!std::isfinite(moments.m00) || moments.m00 <= 0.0) throw std::invalid_argument(error);
    return cv::Point2d(moments.m10 / moments.m00, moments.m01 / moments.m00);
}

double median(std::vector<double> values) {
    if (values.empty()) return std::numeric_limits<double>::quiet_NaN();
    std::sort(values.begin(), values.end());
    const std::size_t count = values.size();
    return count % 2 ? values[count / 2] : 0.5 * (values[count / 2 - 1] + values[count / 2]);
}

double medianNearestSpacing(const std::vector<cv::Point2d>& centers) {
    if (centers.size() < 2) return std::numeric_limits<double>::quiet_NaN();
    std::vector<double> nearest;
    nearest.reserve(centers.size());
    for (std::size_t i = 0; i < centers.size(); ++i) {
        double distance = std::numeric_limits<double>::infinity();
        for (std::size_t j = 0; j < centers.size(); ++j) if (i != j) {
            distance = std::min(distance, cv::norm(centers[i] - centers[j]));
        }
        if (std::isfinite(distance) && distance > 0.0) nearest.push_back(distance);
    }
    return median(nearest);
}

bool reasonableNearestOnStringAxis(const std::vector<cv::Point2d>& centers,
                                   const cv::Point2d& broken_center,
                                   std::size_t& target,
                                   bool& axis_stable) {
    axis_stable = false;
    if (centers.size() < 2) return false;
    cv::Mat points(static_cast<int>(centers.size()), 2, CV_64F);
    for (std::size_t i = 0; i < centers.size(); ++i) {
        points.at<double>(static_cast<int>(i), 0) = centers[i].x;
        points.at<double>(static_cast<int>(i), 1) = centers[i].y;
    }
    cv::PCA pca(points, cv::Mat(), cv::PCA::DATA_AS_ROW);
    const double eigenvalue = pca.eigenvalues.at<double>(0, 0);
    if (!std::isfinite(eigenvalue) || eigenvalue <= 1e-12) return false;
    cv::Point2d axis(pca.eigenvectors.at<double>(0, 0), pca.eigenvectors.at<double>(0, 1));
    const double norm = std::sqrt(axis.dot(axis));
    if (!std::isfinite(norm) || norm <= 1e-12) return false;
    axis *= 1.0 / norm;

    cv::Point2d string_center;
    for (const cv::Point2d& center : centers) string_center += center;
    string_center *= 1.0 / centers.size();
    const double broken_position = (broken_center - string_center).dot(axis);
    std::vector<double> positions;
    positions.reserve(centers.size());
    for (const cv::Point2d& center : centers) positions.push_back((center - string_center).dot(axis));
    std::vector<double> sorted_positions = positions;
    std::sort(sorted_positions.begin(), sorted_positions.end());
    std::vector<double> spacings;
    for (std::size_t i = 1; i < sorted_positions.size(); ++i) {
        const double spacing = sorted_positions[i] - sorted_positions[i - 1];
        if (std::isfinite(spacing) && spacing > 1e-12) spacings.push_back(spacing);
    }
    const double typical_spacing = median(spacings);
    if (!std::isfinite(typical_spacing) || typical_spacing <= 1e-12) return false;
    axis_stable = true;
    double nearest_axis_distance = std::numeric_limits<double>::infinity();
    double nearest_centroid_distance = std::numeric_limits<double>::infinity();
    target = 0;
    for (std::size_t i = 0; i < centers.size(); ++i) {
        const double axis_distance = std::abs(positions[i] - broken_position);
        const double centroid_distance = cv::norm(centers[i] - broken_center);
        if (axis_distance < nearest_axis_distance ||
            (std::abs(axis_distance - nearest_axis_distance) <= 1e-12 && centroid_distance < nearest_centroid_distance)) {
            nearest_axis_distance = axis_distance;
            nearest_centroid_distance = centroid_distance;
            target = i;
        }
    }
    const bool within_observed_axis_range = broken_position >= sorted_positions.front() &&
                                            broken_position <= sorted_positions.back();
    return std::isfinite(nearest_axis_distance) &&
           (within_observed_axis_range || nearest_axis_distance <= 0.50 * typical_spacing);
}

std::size_t nearestByCentroid(const std::vector<cv::Point2d>& centers,
                              const cv::Point2d& broken_center,
                              double& distance) {
    std::size_t target = 0;
    distance = std::numeric_limits<double>::infinity();
    for (std::size_t i = 0; i < centers.size(); ++i) {
        const double candidate = cv::norm(broken_center - centers[i]);
        if (candidate < distance) {
            distance = candidate;
            target = i;
        }
    }
    return target;
}

bool nearestExpandedBoundingBox(const std::vector<cv::Rect>& boxes,
                                const cv::Rect& broken_box,
                                const cv::Size& image_size,
                                std::size_t& target) {
    const int expansion = 15;
    double nearest_distance = std::numeric_limits<double>::infinity();
    bool found = false;
    const cv::Point2d broken_center(
        broken_box.x + 0.5 * broken_box.width,
        broken_box.y + 0.5 * broken_box.height);
    for (std::size_t i = 0; i < boxes.size(); ++i) {
        const cv::Rect& box = boxes[i];
        const int x1 = std::max(0, box.x - expansion);
        const int y1 = std::max(0, box.y - expansion);
        const int x2 = std::min(image_size.width, box.x + box.width + expansion);
        const int y2 = std::min(image_size.height, box.y + box.height + expansion);
        const cv::Rect expanded(x1, y1, x2 - x1, y2 - y1);
        if ((expanded & broken_box).area() <= 0) continue;
        const cv::Point2d box_center(box.x + 0.5 * box.width, box.y + 0.5 * box.height);
        const double distance = cv::norm(box_center - broken_center);
        if (distance < nearest_distance) {
            nearest_distance = distance;
            target = i;
            found = true;
        }
    }
    return found;
}

bool isBroken(const insulator::DefectMask& defect) {
    const bool by_id = defect.class_id == 4;
    const bool by_name = defect.class_name == "broken";
    if (by_id != by_name) throw std::invalid_argument("defect class id/name conflict");
    return by_id;
}

}  // namespace

namespace insulator {
namespace detail {

BrokenAssociation assignBrokenToDisc(const std::vector<cv::Mat>& disc_masks,
                                     const std::vector<DiscResult>& disc_metadata,
                                     const std::vector<DefectMask>& defect_masks) {
    if (disc_masks.size() != disc_metadata.size()) throw std::invalid_argument("disc masks/metadata mismatch");

    BrokenAssociation result;
    if (!disc_masks.empty()) result.image_size = disc_masks[0].size();
    result.disc_centroids.reserve(disc_masks.size());
    std::vector<cv::Mat> binary_discs;
    std::vector<cv::Rect> disc_boxes;
    binary_discs.reserve(disc_masks.size());
    disc_boxes.reserve(disc_masks.size());
    for (const cv::Mat& mask : disc_masks) {
        const cv::Mat binary = binaryMask(mask);
        binary_discs.push_back(binary);
        result.disc_centroids.push_back(centroid(binary, "invalid disc mask area"));
        std::vector<cv::Point> points;
        cv::findNonZero(binary, points);
        disc_boxes.push_back(cv::boundingRect(points));
    }

    const double spacing = medianNearestSpacing(result.disc_centroids);
    int broken_count = 0;
    for (const DefectMask& defect : defect_masks) if (isBroken(defect)) ++broken_count;
    std::cout << "[BrokenAssociation]\n"
              << "input_disc_count: " << disc_masks.size() << '\n'
              << "input_broken_count: " << broken_count << '\n';
    for (std::size_t defect_index = 0; defect_index < defect_masks.size(); ++defect_index) {
        const DefectMask& defect = defect_masks[defect_index];
        if (!isBroken(defect)) continue;
        if (defect.mask.type() != CV_8UC1) throw std::invalid_argument("defect mask size/type mismatch");
        if (result.image_size.empty()) result.image_size = defect.mask.size();
        if (defect.mask.size() != result.image_size) throw std::invalid_argument("defect mask size/type mismatch");

        const cv::Mat broken_mask = binaryMask(defect.mask);
        const int broken_area = cv::countNonZero(broken_mask);
        if (broken_area <= 0) throw std::invalid_argument("empty broken mask");

        BrokenAssignment assignment;
        assignment.defect_index = defect_index;
        assignment.source_broken_instance_id = defect.instance_id;
        assignment.target_instance_id = defect.instance_id;
        assignment.virtual_disc = true;
        assignment.assignment_mode = "VIRTUAL_DISC";
        assignment.centroid = centroid(broken_mask, "invalid broken mask area");
        std::vector<cv::Point> points;
        cv::findNonZero(broken_mask, points);
        assignment.bbox = cv::boundingRect(points);

        double maximum_overlap_fraction = 0.0;
        int maximum_intersection_area = 0;
        std::size_t overlap_target = 0;
        for (std::size_t i = 0; i < binary_discs.size(); ++i) {
            cv::Mat intersection;
            cv::bitwise_and(broken_mask, binary_discs[i], intersection);
            const int intersection_area = cv::countNonZero(intersection);
            const double overlap_fraction = intersection_area / static_cast<double>(broken_area);
            if (overlap_fraction > maximum_overlap_fraction) {
                maximum_overlap_fraction = overlap_fraction;
                maximum_intersection_area = intersection_area;
                overlap_target = i;
            }
        }
        assignment.intersection_area = maximum_intersection_area;
        assignment.intersection_ratio = maximum_overlap_fraction;

        if (!binary_discs.empty() && maximum_overlap_fraction >= 0.10) {
            assignment.target_instance_id = disc_metadata[overlap_target].instance_id;
            assignment.virtual_disc = false;
            assignment.assignment_mode = "INTERSECTION";
        } else if (!binary_discs.empty()) {
            std::size_t axis_target = 0;
            bool axis_stable = false;
            if (reasonableNearestOnStringAxis(
                    result.disc_centroids, assignment.centroid, axis_target, axis_stable)) {
                assignment.target_instance_id = disc_metadata[axis_target].instance_id;
                assignment.virtual_disc = false;
                assignment.assignment_mode = "CENTROID_AXIS_NEAREST";
            } else {
                double nearest_distance = std::numeric_limits<double>::infinity();
                const std::size_t nearest_target = nearestByCentroid(
                    result.disc_centroids, assignment.centroid, nearest_distance);
                const bool centroid_stable = std::isfinite(spacing) && spacing > 1e-12;
                if (centroid_stable && nearest_distance <= 0.50 * spacing) {
                    assignment.target_instance_id = disc_metadata[nearest_target].instance_id;
                    assignment.virtual_disc = false;
                    assignment.assignment_mode = "CENTROID_NEAREST";
                } else if (!axis_stable && !centroid_stable) {
                    std::size_t bbox_target = 0;
                    if (nearestExpandedBoundingBox(
                            disc_boxes, assignment.bbox, result.image_size, bbox_target)) {
                        assignment.target_instance_id = disc_metadata[bbox_target].instance_id;
                        assignment.virtual_disc = false;
                        assignment.assignment_mode = "BBOX_NEAREST";
                    }
                }
            }
        }
        std::cout << "[BrokenAssociation]\n"
                  << "broken_instance_id: " << defect.instance_id << '\n'
                  << "original_disc_candidate_count: " << disc_masks.size() << '\n'
                  << "intersection_area: " << assignment.intersection_area << '\n'
                  << "intersection_ratio: " << assignment.intersection_ratio << '\n'
                  << "assigned_disc_id: " << (assignment.virtual_disc ? -1 : assignment.target_instance_id) << '\n'
                  << "assignment_mode: " << assignment.assignment_mode << '\n';
        result.assignments.push_back(assignment);
    }
    return result;
}

void logFinalDiscList(const std::vector<DiscResult>& discs,
                      const BrokenAssociation& association) {
    std::cout << "[FinalDiscList]\n"
              << "disc_count: " << discs.size() << '\n';
    for (const DiscResult& disc : discs) {
        bool is_virtual = false;
        int source_broken_id = 0;
        for (const BrokenAssignment& assignment : association.assignments) {
            if (assignment.target_instance_id == disc.instance_id) {
                is_virtual = assignment.virtual_disc;
                source_broken_id = assignment.source_broken_instance_id;
                break;
            }
        }
        std::cout << "id: " << disc.id << '\n'
                  << "instance_id: " << disc.instance_id << '\n'
                  << "is_virtual: " << (is_virtual ? "true" : "false") << '\n'
                  << "source_broken_id: " << source_broken_id << '\n'
                  << "reference_eligible: " << (disc.reference_eligible ? "true" : "false") << '\n'
                  << "ideal_area: " << disc.ideal_area << '\n';
    }
}

void excludeBrokenReferences(std::vector<DiscResult>& discs,
                             const BrokenAssociation& association) {
    std::set<int> broken_disc_ids;
    for (const BrokenAssignment& assignment : association.assignments) {
        if (!assignment.virtual_disc) broken_disc_ids.insert(assignment.target_instance_id);
    }
    for (DiscResult& disc : discs) {
        if (broken_disc_ids.count(disc.instance_id) != 0) disc.reference_eligible = false;
    }
}

}  // namespace detail
}  // namespace insulator

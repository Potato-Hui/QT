#include "InsulatorQuantifier.hpp"
#include "QuantificationInternal.hpp"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {

double median(std::vector<double> values) {
    if (values.empty()) throw std::invalid_argument("median requires values");
    std::sort(values.begin(), values.end());
    const std::size_t n = values.size();
    return n % 2 ? values[n / 2] : 0.5 * (values[n / 2 - 1] + values[n / 2]);
}

double occupancy(const cv::Mat& mask, double x, double y) {
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const double tx = x - x0, ty = y - y0;
    double value = 0.0;
    for (int dy = 0; dy <= 1; ++dy) for (int dx = 0; dx <= 1; ++dx) {
        const int xx = x0 + dx, yy = y0 + dy;
        if (0 <= xx && xx < mask.cols && 0 <= yy && yy < mask.rows) {
            const double wx = dx ? tx : 1.0 - tx;
            const double wy = dy ? ty : 1.0 - ty;
            value += (mask.at<unsigned char>(yy, xx) != 0 ? 1.0 : 0.0) * wx * wy;
        }
    }
    return value;
}

bool rayLength(const cv::Mat& mask, const cv::Point2d& center, cv::Point2d direction, double& length) {
    const double norm = std::sqrt(direction.dot(direction));
    if (!(norm > 1e-12) || occupancy(mask, center.x, center.y) <= 0.5) return false;
    direction *= 1.0 / norm;
    const double maximum = std::hypot(static_cast<double>(mask.rows), static_cast<double>(mask.cols)) + 2.0;
    double inside = 0.0, outside = -1.0;
    for (double t = 0.25; t <= maximum; t += 0.25) {
        const cv::Point2d point = center + t * direction;
        if (occupancy(mask, point.x, point.y) <= 0.5) { outside = t; break; }
        inside = t;
    }
    if (outside < 0.0) return false;
    for (int i = 0; i < 24; ++i) {
        const double middle = 0.5 * (inside + outside);
        const cv::Point2d point = center + middle * direction;
        if (occupancy(mask, point.x, point.y) > 0.5) inside = middle; else outside = middle;
    }
    length = 0.5 * (inside + outside);
    return true;
}

cv::Point2d maskCentroid(const cv::Mat& mask) {
    const cv::Moments m = cv::moments(mask, true);
    if (!std::isfinite(m.m00) || m.m00 <= 0.0) throw std::invalid_argument("invalid disc mask area");
    return cv::Point2d(m.m10 / m.m00, m.m01 / m.m00);
}

cv::Point2d maskAxis(const cv::Mat& mask, const cv::Point2d& center) {
    cv::Mat points(cv::countNonZero(mask), 2, CV_64F);
    int row = 0;
    for (int y = 0; y < mask.rows; ++y) for (int x = 0; x < mask.cols; ++x) if (mask.at<unsigned char>(y, x)) {
        points.at<double>(row, 0) = x - center.x;
        points.at<double>(row, 1) = y - center.y;
        ++row;
    }
    if (row < 3) throw std::invalid_argument("disc PCA requires at least three pixels");
    cv::PCA pca(points, cv::Mat(), cv::PCA::DATA_AS_ROW);
    cv::Point2d axis(pca.eigenvectors.at<double>(0, 0), pca.eigenvectors.at<double>(0, 1));
    const double norm = std::sqrt(axis.dot(axis));
    if (!(norm > 1e-12)) throw std::invalid_argument("degenerate disc PCA");
    axis *= 1.0 / norm;
    if (axis.x < -1e-12 || (std::abs(axis.x) <= 1e-12 && axis.y < 0.0)) axis *= -1.0;
    return axis;
}

double linearQ75(std::vector<double> values) {
    if (values.empty()) throw std::invalid_argument("Q75 requires values");
    std::sort(values.begin(), values.end());
    const double position = 0.75 * static_cast<double>(values.size() - 1);
    const std::size_t lower = static_cast<std::size_t>(std::floor(position));
    const std::size_t upper = static_cast<std::size_t>(std::ceil(position));
    const double fraction = position - lower;
    return values[lower] * (1.0 - fraction) + values[upper] * fraction;
}

}  // namespace

namespace insulator {

void InsulatorQuantifier::computeIdealAreas(std::vector<DiscResult>& discs,
                                             std::vector<cv::Mat>& masks,
                                             const CalibrationModel& calibration,
                                             bool reuse) const {
    if (!calibration.valid || calibration.version != "PITCH_IDEAL_AREA_V1" ||
        calibration.feature_name != "pose_ratio_q75" || calibration.method != "LINEAR" ||
        !std::isfinite(calibration.slope) || !std::isfinite(calibration.intercept)) {
        throw std::invalid_argument("invalid Pitch calibration model");
    }
    if (discs.size() != masks.size() || discs.empty()) throw std::invalid_argument("disc metadata count mismatch");

    if (reuse) {
        for (std::size_t i = 0; i < discs.size(); ++i) {
            DiscResult& d = discs[i];
            if (!d.frozen_geometry_present || !std::isfinite(d.frozen_axis_position) ||
                (d.frozen_multi_axis_measurement_valid &&
                 (!std::isfinite(d.frozen_major_span) || d.frozen_major_span <= 0.0 ||
                  !std::isfinite(d.frozen_pose_ratio_q75)))) {
                throw std::invalid_argument("strict metadata geometry is missing or invalid");
            }
            d.axis_position = d.frozen_axis_position;
            d.major_span = d.frozen_major_span;
            d.multi_axis_measurement_valid = d.frozen_multi_axis_measurement_valid;
            d.pose_ratio_q75 = d.frozen_pose_ratio_q75;
        }
    } else {
        std::vector<cv::Point2d> centers(discs.size()), axes(discs.size());
        for (std::size_t i = 0; i < discs.size(); ++i) {
            centers[i] = maskCentroid(masks[i]);
            axes[i] = maskAxis(masks[i], centers[i]);
            discs[i].centroid_x = centers[i].x; discs[i].centroid_y = centers[i].y;
            double positive = 0.0, negative = 0.0;
            if (!rayLength(masks[i], centers[i], axes[i], positive) ||
                !rayLength(masks[i], centers[i], -axes[i], negative)) {
                discs[i].multi_axis_measurement_valid = false;
                continue;
            }
            discs[i].major_span = positive + negative;
            std::vector<double> widths;
            const cv::Point2d minor_axis(-axes[i].y, axes[i].x);
            const double ts[] = {-0.60,-0.45,-0.30,-0.15,0.0,0.15,0.30,0.45,0.60};
            for (double t : ts) {
                const cv::Point2d cross = centers[i] + (t * 0.5 * discs[i].major_span) * axes[i];
                if (occupancy(masks[i], cross.x, cross.y) <= 0.5) continue;
                double p = 0.0, n = 0.0;
                if (rayLength(masks[i], cross, minor_axis, p) && rayLength(masks[i], cross, -minor_axis, n)) widths.push_back(p + n);
            }
            discs[i].multi_axis_measurement_valid = widths.size() >= 5;
            if (discs[i].multi_axis_measurement_valid) discs[i].pose_ratio_q75 = linearQ75(widths) / discs[i].major_span;
        }
        cv::Mat points(static_cast<int>(centers.size()), 2, CV_64F);
        for (std::size_t i = 0; i < centers.size(); ++i) { points.at<double>(static_cast<int>(i),0)=centers[i].x; points.at<double>(static_cast<int>(i),1)=centers[i].y; }
        cv::Point2d string_center(0.0,0.0), string_axis(1.0,0.0);
        for (const cv::Point2d& p : centers) string_center += p;
        string_center *= 1.0 / centers.size();
        if (centers.size() > 1) {
            cv::PCA pca(points, cv::Mat(), cv::PCA::DATA_AS_ROW);
            string_axis = cv::Point2d(pca.eigenvectors.at<double>(0,0),pca.eigenvectors.at<double>(0,1));
            if (std::abs(string_axis.y) >= std::abs(string_axis.x)) { if (string_axis.y < 0.0) string_axis *= -1.0; }
            else if (string_axis.x < 0.0) string_axis *= -1.0;
        }
        for (std::size_t i=0;i<discs.size();++i) discs[i].axis_position=(centers[i]-string_center).dot(string_axis);
    }

    std::vector<std::size_t> order(discs.size());
    for (std::size_t i=0;i<order.size();++i) order[i]=i;
    std::stable_sort(order.begin(), order.end(), [&](std::size_t a,std::size_t b){return discs[a].axis_position<discs[b].axis_position;});
    std::vector<DiscResult> sorted_discs; std::vector<cv::Mat> sorted_masks;
    for (std::size_t index : order) { sorted_discs.push_back(discs[index]); sorted_masks.push_back(masks[index]); }
    discs.swap(sorted_discs); masks.swap(sorted_masks);
    for (std::size_t i=0;i<discs.size();++i) discs[i].id=static_cast<int>(i+1);

    std::vector<double> features, ref_x, ref_span;
    for (const DiscResult& d : discs) if (d.reference_eligible && d.multi_axis_measurement_valid &&
        std::isfinite(d.pose_ratio_q75) && std::isfinite(d.major_span) && d.major_span>0.0 && std::isfinite(d.axis_position)) {
        features.push_back(d.pose_ratio_q75); ref_x.push_back(d.axis_position); ref_span.push_back(d.major_span);
    }
    if (features.size() == 1) {
        const double coefficient = calibration.intercept + calibration.slope * features[0];
        const double predicted_major_span = ref_span[0];
        const double ideal_area = coefficient * predicted_major_span * predicted_major_span;
        if (!std::isfinite(coefficient) || coefficient <= 0.0 ||
            !std::isfinite(predicted_major_span) || predicted_major_span <= 0.0 ||
            !std::isfinite(ideal_area) || ideal_area <= 0.0) {
            throw std::invalid_argument("invalid ideal_area prediction");
        }
        for (DiscResult& d : discs) {
            d.coefficient = coefficient;
            d.predicted_major_span = predicted_major_span;
            d.ideal_area = ideal_area;
        }
        return;
    }
    if (features.size()<2) throw std::invalid_argument("insufficient reliable references");
    std::vector<double> slopes;
    for (std::size_t i=0;i<ref_x.size();++i) for(std::size_t j=i+1;j<ref_x.size();++j)
        if(ref_x[j]!=ref_x[i]) slopes.push_back((ref_span[j]-ref_span[i])/(ref_x[j]-ref_x[i]));
    if(slopes.empty()) throw std::invalid_argument("references require distinct axis positions");
    const double coefficient=calibration.intercept+calibration.slope*median(features);
    const double slope=median(slopes);
    std::vector<double> intercepts; for(std::size_t i=0;i<ref_x.size();++i) intercepts.push_back(ref_span[i]-slope*ref_x[i]);
    const double intercept=median(intercepts);
    if(!std::isfinite(coefficient)||coefficient<=0.0) throw std::invalid_argument("invalid ideal-area coefficient");
    for(DiscResult& d:discs){
        d.coefficient=coefficient; d.predicted_major_span=intercept+slope*d.axis_position;
        d.ideal_area=coefficient*d.predicted_major_span*d.predicted_major_span;
        if(!std::isfinite(d.predicted_major_span)||d.predicted_major_span<=0.0||!std::isfinite(d.ideal_area)||d.ideal_area<=0.0)
            throw std::invalid_argument("invalid ideal_area prediction");
    }
}

namespace detail {

namespace {

bool reliableReference(const DiscResult& disc, bool require_ideal_area) {
    if (!disc.reference_eligible || disc.geometry_validity != "VALID" ||
        !disc.multi_axis_measurement_valid || !std::isfinite(disc.pose_ratio_q75) ||
        !std::isfinite(disc.major_span) || disc.major_span <= 0.0 ||
        !std::isfinite(disc.axis_position)) return false;
    return !require_ideal_area || (std::isfinite(disc.ideal_area) && disc.ideal_area > 0.0);
}

double virtualAxisPosition(const std::vector<DiscResult>& discs, const cv::Point2d& point) {
    std::vector<cv::Point2d> centers;
    std::vector<double> positions;
    for (const DiscResult& disc : discs) {
        if (std::isfinite(disc.centroid_x) && std::isfinite(disc.centroid_y) &&
            std::isfinite(disc.axis_position)) {
            centers.push_back(cv::Point2d(disc.centroid_x, disc.centroid_y));
            positions.push_back(disc.axis_position);
        }
    }
    if (centers.empty()) return 0.0;
    if (centers.size() == 1) return positions[0];

    cv::Mat points(static_cast<int>(centers.size()), 2, CV_64F);
    for (std::size_t i = 0; i < centers.size(); ++i) {
        points.at<double>(static_cast<int>(i), 0) = centers[i].x;
        points.at<double>(static_cast<int>(i), 1) = centers[i].y;
    }
    cv::PCA pca(points, cv::Mat(), cv::PCA::DATA_AS_ROW);
    cv::Point2d axis(pca.eigenvectors.at<double>(0, 0), pca.eigenvectors.at<double>(0, 1));
    if (std::abs(axis.y) >= std::abs(axis.x)) { if (axis.y < 0.0) axis *= -1.0; }
    else if (axis.x < 0.0) axis *= -1.0;

    cv::Point2d center;
    for (const cv::Point2d& value : centers) center += value;
    center *= 1.0 / centers.size();
    std::vector<double> projections;
    projections.reserve(centers.size());
    double projection_mean = 0.0, position_mean = 0.0;
    for (std::size_t i = 0; i < centers.size(); ++i) {
        const double projection = (centers[i] - center).dot(axis);
        projections.push_back(projection);
        projection_mean += projection;
        position_mean += positions[i];
    }
    projection_mean /= projections.size();
    position_mean /= positions.size();
    double numerator = 0.0, denominator = 0.0;
    for (std::size_t i = 0; i < projections.size(); ++i) {
        numerator += (projections[i] - projection_mean) * (positions[i] - position_mean);
        denominator += (projections[i] - projection_mean) * (projections[i] - projection_mean);
    }
    if (!(denominator > 1e-12)) return position_mean;
    return position_mean + numerator / denominator * (((point - center).dot(axis)) - projection_mean);
}

double neighborIdealArea(const std::vector<DiscResult>& discs,
                         const std::vector<std::size_t>& references,
                         double axis_position) {
    std::size_t before = discs.size(), after = discs.size(), nearest = discs.size();
    double before_position = -std::numeric_limits<double>::infinity();
    double after_position = std::numeric_limits<double>::infinity();
    double nearest_distance = std::numeric_limits<double>::infinity();
    for (std::size_t index : references) {
        const double position = discs[index].axis_position;
        const double distance = std::abs(position - axis_position);
        if (distance < nearest_distance) { nearest_distance = distance; nearest = index; }
        if (position < axis_position && position > before_position) { before_position = position; before = index; }
        if (position > axis_position && position < after_position) { after_position = position; after = index; }
    }
    if (before != discs.size() && after != discs.size()) {
        return 0.5 * (discs[before].ideal_area + discs[after].ideal_area);
    }
    if (before != discs.size()) return discs[before].ideal_area;
    if (after != discs.size()) return discs[after].ideal_area;
    if (nearest != discs.size()) return discs[nearest].ideal_area;
    return 0.0;
}

std::string neighborReferenceSource(const std::vector<DiscResult>& discs,
                                    const std::vector<std::size_t>& references,
                                    double axis_position) {
    std::size_t before = discs.size(), after = discs.size(), nearest = discs.size();
    double before_position = -std::numeric_limits<double>::infinity();
    double after_position = std::numeric_limits<double>::infinity();
    double nearest_distance = std::numeric_limits<double>::infinity();
    for (std::size_t index : references) {
        const double position = discs[index].axis_position;
        const double distance = std::abs(position - axis_position);
        if (distance < nearest_distance) { nearest_distance = distance; nearest = index; }
        if (position < axis_position && position > before_position) { before_position = position; before = index; }
        if (position > axis_position && position < after_position) { after_position = position; after = index; }
    }
    std::ostringstream stream;
    if (before != discs.size() && after != discs.size()) {
        stream << "BEFORE_AFTER:" << discs[before].instance_id << ',' << discs[after].instance_id;
    } else if (before != discs.size()) {
        stream << "NEAREST:" << discs[before].instance_id;
    } else if (after != discs.size()) {
        stream << "NEAREST:" << discs[after].instance_id;
    } else if (nearest != discs.size()) {
        stream << "NEAREST:" << discs[nearest].instance_id;
    } else {
        stream << "NONE";
    }
    return stream.str();
}

double referenceCoefficient(const std::vector<DiscResult>& discs,
                            const std::vector<std::size_t>& references,
                            const CalibrationModel& calibration) {
    std::vector<double> features;
    for (std::size_t index : references) features.push_back(discs[index].pose_ratio_q75);
    const double coefficient = calibration.intercept + calibration.slope * median(features);
    if (!std::isfinite(coefficient) || coefficient <= 0.0) throw std::invalid_argument("invalid ideal-area coefficient");
    return coefficient;
}

void setIdealArea(DiscResult& disc, double ideal_area, double coefficient) {
    disc.coefficient = coefficient;
    disc.ideal_area = ideal_area;
    disc.predicted_major_span = std::sqrt(ideal_area / coefficient);
}

}  // namespace

bool applyBrokenIdealAreas(std::vector<DiscResult>& discs,
                           std::vector<cv::Mat>& disc_masks,
                           const std::vector<DefectMask>& defect_masks,
                           const CalibrationModel& calibration,
                           const BrokenAssociation& association,
                           bool theil_sen_available) {
    std::vector<std::size_t> references;
    for (std::size_t i = 0; i < discs.size(); ++i) {
        if (reliableReference(discs[i], theil_sen_available)) references.push_back(i);
    }

    double coefficient = 0.0;
    if (!references.empty()) {
        if (theil_sen_available) coefficient = discs[references[0]].coefficient;
        else {
            coefficient = referenceCoefficient(discs, references, calibration);
            for (std::size_t index : references) {
                setIdealArea(discs[index], coefficient * discs[index].major_span * discs[index].major_span, coefficient);
            }
        }
    }

    for (const BrokenAssignment& assignment : association.assignments) {
        if (assignment.virtual_disc) continue;
        for (DiscResult& disc : discs) if (disc.instance_id == assignment.target_instance_id) {
            if (!references.empty()) {
                const double ideal_area = neighborIdealArea(discs, references, disc.axis_position);
                if (ideal_area > 0.0) setIdealArea(disc, ideal_area, coefficient);
            } else {
                disc.coefficient = 0.0;
                disc.predicted_major_span = 0.0;
                disc.ideal_area = 0.0;
            }
            break;
        }
    }

    struct VirtualLogRecord {
        int source_broken_instance_id;
        cv::Point2d centroid;
        double estimated_ideal_area;
        std::string reference_source;
    };
    std::vector<VirtualLogRecord> virtual_logs;
    for (const BrokenAssignment& assignment : association.assignments) {
        if (!assignment.virtual_disc) continue;
        const DefectMask& defect = defect_masks[assignment.defect_index];
        DiscResult virtual_disc;
        virtual_disc.instance_id = defect.instance_id;
        virtual_disc.class_id = 0;
        virtual_disc.class_name = "insulator";
        virtual_disc.confidence = defect.confidence;
        virtual_disc.mask_quality = "ANOMALOUS";
        virtual_disc.geometry_validity = "INVALID";
        virtual_disc.local_failure = "OCCLUSION_OR_TRUNCATION";
        virtual_disc.reference_eligible = false;
        virtual_disc.centroid_x = assignment.centroid.x;
        virtual_disc.centroid_y = assignment.centroid.y;
        virtual_disc.axis_position = virtualAxisPosition(discs, assignment.centroid);
        std::string reference_source = "NONE";
        if (!references.empty()) {
            const double ideal_area = neighborIdealArea(discs, references, virtual_disc.axis_position);
            if (ideal_area > 0.0) setIdealArea(virtual_disc, ideal_area, coefficient);
            reference_source = neighborReferenceSource(discs, references, virtual_disc.axis_position);
        } else {
            virtual_disc.coefficient = 0.0;
            virtual_disc.predicted_major_span = 0.0;
            virtual_disc.ideal_area = 0.0;
        }
        discs.push_back(virtual_disc);
        disc_masks.push_back(cv::Mat::zeros(association.image_size, CV_8UC1));
        VirtualLogRecord record;
        record.source_broken_instance_id = assignment.source_broken_instance_id;
        record.centroid = assignment.centroid;
        record.estimated_ideal_area = virtual_disc.ideal_area;
        record.reference_source = reference_source;
        virtual_logs.push_back(record);
    }

    for (DiscResult& disc : discs) {
        if (!std::isfinite(disc.ideal_area) || disc.ideal_area < 0.0) {
            disc.coefficient = 0.0;
            disc.predicted_major_span = 0.0;
            disc.ideal_area = 0.0;
        }
    }

    std::vector<std::size_t> order(discs.size());
    for (std::size_t i = 0; i < order.size(); ++i) order[i] = i;
    std::stable_sort(order.begin(), order.end(), [&](std::size_t a, std::size_t b) {
        return discs[a].axis_position < discs[b].axis_position;
    });
    std::vector<DiscResult> sorted_discs;
    std::vector<cv::Mat> sorted_masks;
    for (std::size_t index : order) {
        sorted_discs.push_back(discs[index]);
        sorted_masks.push_back(disc_masks[index]);
    }
    discs.swap(sorted_discs);
    disc_masks.swap(sorted_masks);
    for (std::size_t i = 0; i < discs.size(); ++i) discs[i].id = static_cast<int>(i + 1);
    for (const VirtualLogRecord& record : virtual_logs) {
        int virtual_disc_id = 0;
        for (const DiscResult& disc : discs) {
            if (disc.instance_id == record.source_broken_instance_id) { virtual_disc_id = disc.id; break; }
        }
        std::cout << "[VirtualDiscCreated]\n"
                  << "virtual_disc_id: " << virtual_disc_id << '\n'
                  << "source_broken_instance_id: " << record.source_broken_instance_id << '\n'
                  << "centroid: (" << record.centroid.x << ", " << record.centroid.y << ")\n"
                  << "estimated_ideal_area: " << record.estimated_ideal_area << '\n'
                  << "reference_source: " << record.reference_source << '\n';
    }
    return !references.empty();
}

}  // namespace detail

}  // namespace insulator

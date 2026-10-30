#include "InsulatorQuantifier.hpp"
#include "QuantificationInternal.hpp"

#include <opencv2/imgcodecs.hpp>

#include <cmath>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <stdexcept>

namespace {
std::string directoryOf(const std::string& path){std::size_t p=path.find_last_of("/\\");return p==std::string::npos?".":path.substr(0,p);}
std::string joinPath(const std::string& base,const std::string& relative){if(relative.empty()||relative.find("..")!=std::string::npos||relative[0]=='/'||relative[0]=='\\'||relative.find(':')!=std::string::npos)throw std::invalid_argument("mask path escapes metadata directory");return base+"/"+relative;}
bool validClassName(int class_id,const std::string& class_name){const char* names[]={"insulator","crack","pollution","flashover","broken"};return class_id>=0&&class_id<=4&&(class_name==names[class_id]||(class_id==0&&class_name=="insulator_disc"));}
const char* const kJsonNull="__opencv_json_null__";
std::string readOpenCvJson(const std::string& path){
    std::ifstream input(path.c_str(),std::ios::binary);if(!input)throw std::runtime_error("JSON read failed: "+path);
    std::ostringstream stream;stream<<input.rdbuf();const std::string source=stream.str();std::string result;result.reserve(source.size());
    bool quoted=false,escaped=false;
    for(std::size_t i=0;i<source.size();){
        const char ch=source[i];
        if(quoted){result.push_back(ch);if(escaped)escaped=false;else if(ch=='\\')escaped=true;else if(ch=='\"')quoted=false;++i;continue;}
        if(ch=='\"'){quoted=true;result.push_back(ch);++i;continue;}
        if(i+4<=source.size()&&source.compare(i,4,"null")==0){result+='\"';result+=kJsonNull;result+='\"';i+=4;continue;}
        result.push_back(ch);++i;
    }
    return result;
}
double optionalDouble(const cv::FileNode& node){if(node.empty()||node.isNone()||(node.isString()&&static_cast<std::string>(node)==kJsonNull))return std::numeric_limits<double>::quiet_NaN();return static_cast<double>(node);}
bool validCalibration(const insulator::CalibrationModel& calibration){return calibration.valid&&calibration.version=="PITCH_IDEAL_AREA_V1"&&calibration.feature_name=="pose_ratio_q75"&&calibration.method=="LINEAR"&&std::isfinite(calibration.slope)&&std::isfinite(calibration.intercept);}
void writeResult(const std::string& path,const insulator::QuantificationResult& result){
    cv::FileStorage fs(path,cv::FileStorage::WRITE|cv::FileStorage::FORMAT_JSON);if(!fs.isOpened())throw std::runtime_error("output file write failed");
    fs<<"basic_damage_index"<<result.basic_damage_index<<"conservative_risk_index"<<result.conservative_risk_index<<"overall_level"<<result.overall_level<<"recommendation"<<result.recommendation<<"highest_risk_disc_id"<<result.highest_risk_disc_id<<"disc_count"<<result.disc_count<<"defect_instance_count"<<result.defect_instance_count<<"discs"<<"[";
    for(const insulator::DiscResult& d:result.discs){fs<<"{"<<"id"<<d.id<<"instance_id"<<d.instance_id<<"mask_quality"<<d.mask_quality<<"geometry_validity"<<d.geometry_validity<<"local_failure"<<d.local_failure<<"reference_eligible"<<static_cast<int>(d.reference_eligible)<<"ideal_area"<<d.ideal_area<<"predicted_major_span"<<d.predicted_major_span<<"coefficient"<<d.coefficient<<"crack_area"<<d.crack_area<<"pollution_area"<<d.pollution_area<<"flashover_area"<<d.flashover_area<<"broken_area"<<d.broken_area<<"crack_ratio"<<d.crack_ratio<<"pollution_ratio"<<d.pollution_ratio<<"flashover_ratio"<<d.flashover_ratio<<"broken_ratio"<<d.broken_ratio<<"basic_damage_index"<<d.basic_damage_index<<"conservative_risk_index"<<d.conservative_risk_index<<"}";}fs<<"]";fs.release();
    cv::FileStorage check(path,cv::FileStorage::READ);if(!check.isOpened()||check["discs"].empty())throw std::runtime_error("written result JSON validation failed");
}
}

namespace insulator {
QuantificationResult InsulatorQuantifier::processMasks(const std::vector<cv::Mat>& disc_masks,const std::vector<DiscResult>& metadata,const std::vector<DefectMask>& defects,const CalibrationModel& calibration,bool reuse) const{
    if(disc_masks.size()!=metadata.size())throw std::invalid_argument("disc masks/metadata mismatch");
    if(!validCalibration(calibration))throw std::invalid_argument("invalid Pitch calibration model");
    const cv::Size size=disc_masks.empty()?cv::Size():disc_masks[0].size();std::set<int> ids;
    for(std::size_t i=0;i<disc_masks.size();++i){if(disc_masks[i].type()!=CV_8UC1||disc_masks[i].size()!=size)throw std::invalid_argument("disc mask size/type mismatch");if(metadata[i].class_id!=0||!validClassName(metadata[i].class_id,metadata[i].class_name))throw std::invalid_argument("disc class id/name conflict");if(!std::isfinite(metadata[i].confidence)||!ids.insert(metadata[i].instance_id).second)throw std::invalid_argument("invalid disc metadata");}
    std::vector<cv::Mat> masks;for(const cv::Mat& m:disc_masks){cv::Mat binary;cv::compare(m,0,binary,cv::CMP_GT);masks.push_back(binary);}
    const detail::BrokenAssociation association=detail::assignBrokenToDisc(masks,metadata,defects);
    if(masks.empty()&&association.assignments.empty())throw std::invalid_argument("disc masks/metadata mismatch");
    std::vector<DiscResult> discs=metadata;
    for(std::size_t i=0;i<discs.size();++i){discs[i].centroid_x=association.disc_centroids[i].x;discs[i].centroid_y=association.disc_centroids[i].y;}
    if(!reuse&&!masks.empty())computeGating(discs,masks);
    detail::excludeBrokenReferences(discs,association);
    bool theil_sen_available=false;
    if(!masks.empty()){
        try{computeIdealAreas(discs,masks,calibration,reuse);theil_sen_available=true;}
        catch(const std::invalid_argument& error){const std::string message=error.what();if(association.assignments.empty()||(message!="insufficient reliable references"&&message!="references require distinct axis positions"))throw;}
    }
    if(association.assignments.empty()){detail::logFinalDiscList(discs,association);return detail::evaluateDamageWithBroken(masks,discs,defects,association,false);}
    const bool has_reference=detail::applyBrokenIdealAreas(discs,masks,defects,calibration,association,theil_sen_available);
    detail::logFinalDiscList(discs,association);
    return detail::evaluateDamageWithBroken(masks,discs,defects,association,!has_reference);
}

QuantificationResult InsulatorQuantifier::processFromFiles(const std::string& image_path,const std::string& metadata_path,const std::string& calibration_path,const std::string& result_path,bool verbose,bool reuse) const{
    (void)verbose;
    cv::Mat image=cv::imread(image_path,cv::IMREAD_COLOR);if(image.empty())throw std::runtime_error("image read failed: "+image_path);
    const std::string metadata_json=readOpenCvJson(metadata_path);cv::FileStorage metadata_fs(metadata_json,cv::FileStorage::READ|cv::FileStorage::MEMORY|cv::FileStorage::FORMAT_JSON);if(!metadata_fs.isOpened())throw std::runtime_error("metadata JSON parse failed");
    const int width=static_cast<int>(metadata_fs["image_width"]),height=static_cast<int>(metadata_fs["image_height"]);if(width!=image.cols||height!=image.rows)throw std::invalid_argument("image metadata dimensions mismatch");
    cv::FileNode instances=metadata_fs["instances"];if(instances.type()!=cv::FileNode::SEQ)throw std::invalid_argument("metadata instances must be an array");
    std::vector<cv::Mat> discs;std::vector<DiscResult> disc_metadata;std::vector<DefectMask> defects;
    for(cv::FileNodeIterator it=instances.begin();it!=instances.end();++it){const cv::FileNode node=*it;int class_id=static_cast<int>(node["class_id"]);std::string class_name=static_cast<std::string>(node["class_name"]);if(!validClassName(class_id,class_name))throw std::invalid_argument("class id/name conflict");
        std::string relative=static_cast<std::string>(node["mask_file"]);cv::Mat source=cv::imread(joinPath(directoryOf(metadata_path),relative),cv::IMREAD_GRAYSCALE);if(source.empty())throw std::runtime_error("PNG mask read failed: "+relative);cv::Mat full;
        cv::FileNode bbox=node["bbox_xyxy"];if(!bbox.empty()){if(bbox.type()!=cv::FileNode::SEQ||bbox.size()!=4)throw std::invalid_argument("invalid bbox_xyxy");int values[4],k=0;for(cv::FileNodeIterator b=bbox.begin();b!=bbox.end();++b)values[k++]=static_cast<int>(*b);if(values[0]<0||values[1]<0||values[2]<=values[0]||values[3]<=values[1]||values[2]>width||values[3]>height||source.cols!=values[2]-values[0]||source.rows!=values[3]-values[1])throw std::invalid_argument("ROI mask/bbox mismatch");full=cv::Mat::zeros(height,width,CV_8UC1);source.copyTo(full(cv::Rect(values[0],values[1],source.cols,source.rows)));}
        else{if(source.cols!=width||source.rows!=height)throw std::invalid_argument("full mask dimensions mismatch");full=source;}cv::compare(full,0,full,cv::CMP_GT);
        int instance_id=static_cast<int>(node["instance_id"]);double confidence=static_cast<double>(node["confidence"]);
        if(class_id==0){DiscResult d;d.instance_id=instance_id;d.confidence=confidence;if(reuse&&node["reference_eligible"].empty())throw std::invalid_argument("disc reference_eligible missing");if(!node["reference_eligible"].empty())d.reference_eligible=static_cast<int>(node["reference_eligible"])!=0;d.frozen_geometry_present=!node["axis_position"].empty()&&!node["major_span"].empty()&&!node["multi_axis_measurement_valid"].empty()&&!node["pose_ratio_q75"].empty();d.frozen_axis_position=optionalDouble(node["axis_position"]);d.frozen_major_span=optionalDouble(node["major_span"]);d.frozen_multi_axis_measurement_valid=!node["multi_axis_measurement_valid"].empty()&&static_cast<int>(node["multi_axis_measurement_valid"])!=0;d.frozen_pose_ratio_q75=optionalDouble(node["pose_ratio_q75"]);discs.push_back(full);disc_metadata.push_back(d);}
        else{DefectMask d;d.mask=full;d.instance_id=instance_id;d.class_id=class_id;d.class_name=class_name;d.confidence=confidence;defects.push_back(d);}
    }
    cv::FileStorage calibration_fs(calibration_path,cv::FileStorage::READ);if(!calibration_fs.isOpened())throw std::runtime_error("calibration JSON parse failed");cv::FileNode model=calibration_fs["model"];CalibrationModel calibration;calibration.version=static_cast<std::string>(calibration_fs["version"]);calibration.feature_name=static_cast<std::string>(model["feature_name"]);calibration.method=static_cast<std::string>(model["method"]);calibration.valid=static_cast<int>(model["valid"])!=0;calibration.slope=static_cast<double>(model["slope"]);calibration.intercept=static_cast<double>(model["intercept"]);
    QuantificationResult result=processMasks(discs,disc_metadata,defects,calibration,reuse);writeResult(result_path,result);return result;
}
}

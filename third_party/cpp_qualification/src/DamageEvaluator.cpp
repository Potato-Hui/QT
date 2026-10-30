#include "InsulatorQuantifier.hpp"
#include "QuantificationInternal.hpp"

#include <opencv2/core.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace {
double clip(double value){return std::max(0.0,std::min(1.0,value));}
double saturation(double ratio){if(!std::isfinite(ratio))throw std::invalid_argument("non-finite defect ratio");double r=clip(ratio);if(r==0.0)return 0.0;return(1.0-std::exp(-3.0*r))/(1.0-std::exp(-3.0));}
std::string level(double score){if(score<0.15)return "正常";if(score<0.35)return "轻度";if(score<0.60)return "中度";return "严重";}
std::string recommendation(const std::string& value){if(value=="正常")return "继续运行";if(value=="轻度")return "增加巡检频率";if(value=="中度")return "计划检修";return "重点检查";}
}

namespace insulator {
namespace detail {

QuantificationResult evaluateDamageWithBroken(const std::vector<cv::Mat>& masks,
                                               const std::vector<DiscResult>& source,
                                               const std::vector<DefectMask>& defects,
                                               const BrokenAssociation& association,
                                               bool no_reference_degraded) {
    if (masks.empty() || masks.size() != source.size()) throw std::invalid_argument("damage input mismatch");
    const char* names[]={"","crack","pollution","flashover","broken"};
    std::vector<cv::Mat> unions(4);for(int i=1;i<=3;++i)unions[i]=cv::Mat::zeros(masks[0].size(),CV_8UC1);
    for(const DefectMask& defect:defects){
        if(defect.class_id<1||defect.class_id>4||defect.class_name!=names[defect.class_id])throw std::invalid_argument("defect class id/name conflict");
        if(defect.mask.type()!=CV_8UC1||defect.mask.size()!=masks[0].size())throw std::invalid_argument("defect mask size/type mismatch");
        if(defect.class_id<=3)cv::bitwise_or(unions[defect.class_id],defect.mask,unions[defect.class_id]);
    }
    std::vector<cv::Mat> broken_unions(source.size());
    for(std::size_t i=0;i<source.size();++i)broken_unions[i]=cv::Mat::zeros(masks[0].size(),CV_8UC1);
    for(const BrokenAssignment& assignment:association.assignments){
        std::size_t target=source.size();
        for(std::size_t i=0;i<source.size();++i)if(source[i].instance_id==assignment.target_instance_id){target=i;break;}
        if(target==source.size())throw std::invalid_argument("broken assignment target missing");
        cv::Mat binary;cv::compare(defects[assignment.defect_index].mask,0,binary,cv::CMP_GT);
        cv::bitwise_or(broken_unions[target],binary,broken_unions[target]);
    }
    QuantificationResult result;result.discs=source;result.disc_count=static_cast<int>(source.size());result.defect_instance_count=static_cast<int>(defects.size());
    double basic_sum=0.0,conservative_sum=0.0;result.basic_max=-1.0;result.conservative_max=-1.0;
    for(std::size_t i=0;i<result.discs.size();++i){DiscResult& d=result.discs[i];cv::Mat intersection;
        int* surface_areas[]={&d.crack_area,&d.pollution_area,&d.flashover_area};double* surface_ratios[]={&d.crack_ratio,&d.pollution_ratio,&d.flashover_ratio};
        for(int c=1;c<=3;++c){cv::bitwise_and(unions[c],masks[i],intersection);*surface_areas[c-1]=cv::countNonZero(intersection);}
        d.broken_area=cv::countNonZero(broken_unions[i]);
        const bool ideal_valid=std::isfinite(d.ideal_area)&&d.ideal_area>0.0;
        if(ideal_valid){
            for(int c=0;c<3;++c)*surface_ratios[c]=*surface_areas[c]/d.ideal_area;
            d.broken_ratio=d.broken_area/d.ideal_area;
            const double ratios[]={d.crack_ratio,d.pollution_ratio,d.flashover_ratio,d.broken_ratio};
            for(double ratio:ratios)if(!std::isfinite(ratio)||ratio<0.0)throw std::invalid_argument("invalid defect ratio");
            d.basic_damage_index=clip(0.8*d.crack_ratio+0.6*d.pollution_ratio+0.9*d.flashover_ratio+1.0*d.broken_ratio);
            d.conservative_risk_index=clip(0.8*saturation(d.crack_ratio)+0.6*saturation(d.pollution_ratio)+0.9*saturation(d.flashover_ratio)+saturation(d.broken_ratio));
        }else{
            d.crack_ratio=d.pollution_ratio=d.flashover_ratio=d.broken_ratio=0.0;
            d.basic_damage_index=d.conservative_risk_index=0.0;
            if(no_reference_degraded&&d.broken_area>0){d.basic_damage_index=1.0;d.conservative_risk_index=1.0;}
        }
        basic_sum+=d.basic_damage_index;conservative_sum+=d.conservative_risk_index;result.basic_max=std::max(result.basic_max,d.basic_damage_index);
        if(d.conservative_risk_index>result.conservative_max){result.conservative_max=d.conservative_risk_index;result.highest_risk_disc_id=d.id;}
    }
    result.basic_mean=basic_sum/result.discs.size();result.conservative_mean=conservative_sum/result.discs.size();
    result.basic_damage_index=clip(0.5*result.basic_mean+0.5*result.basic_max);result.conservative_risk_index=clip(0.5*result.conservative_mean+0.5*result.conservative_max);
    double maximum_broken_ratio=0.0;for(const DiscResult& d:result.discs)maximum_broken_ratio=std::max(maximum_broken_ratio,d.broken_ratio);
    double broken_risk_floor=0.0;bool replacement=false;
    if(no_reference_degraded){broken_risk_floor=1.0;replacement=true;}
    else if(maximum_broken_ratio>0.50){broken_risk_floor=0.60;replacement=true;}
    else if(maximum_broken_ratio>=0.30)broken_risk_floor=0.60;
    else if(maximum_broken_ratio>=0.10)broken_risk_floor=0.35;
    result.conservative_risk_index=std::max(result.conservative_risk_index,broken_risk_floor);
    result.overall_level=level(result.conservative_risk_index);result.recommendation=replacement?"建议更换":recommendation(result.overall_level);return result;
}

}  // namespace detail

QuantificationResult InsulatorQuantifier::evaluateDamage(const std::vector<cv::Mat>& masks,const std::vector<DiscResult>& source,const std::vector<DefectMask>& defects) const{
    const detail::BrokenAssociation association=detail::assignBrokenToDisc(masks,source,defects);
    return detail::evaluateDamageWithBroken(masks,source,defects,association,false);
}
}

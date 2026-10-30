#include "InsulatorQuantifier.hpp"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <stdexcept>
#include <vector>

namespace {

const double EPS=1e-6;
const double CROSS_SECTION_U[]={-0.75,-0.40,0.00,0.40,0.75};
const double CROSS_SECTION_HALF_BAND=0.015;
const int MIN_SECTION_PIXELS=10;
const int SECTION_GAP_MERGE_PIXELS=2;
const double MASK_SCORE_SUSPECT=1.5;
const double MASK_SCORE_ANOMALOUS=2.5;
const double ELLIPSE_RECT_MINOR_INVALID_HIGH=2.0;
const double ELLIPSE_RECT_MINOR_INVALID_LOW=0.45;
const double ELLIPSE_RECT_MAJOR_INVALID_HIGH=1.8;
const double ELLIPSE_RECT_MAJOR_INVALID_LOW=0.55;
const double FILL_RATIO_EXTREME_LOW=0.10;
const double POSE_RATIO_EXTREME_HIGH=0.90;
const double CENTROID_OFFSET_ABSOLUTE_HIGH=0.15;
const int PROFILE_BREAK_ABSOLUTE_HIGH=2;
const double FILL_RATIO_CONSISTENCY_LOW=0.30;
const double ELLIPSE_RECT_MAJOR_CONSISTENT_LOW=0.80;
const double ELLIPSE_RECT_MAJOR_CONSISTENT_HIGH=1.25;
const double ELLIPSE_RECT_MINOR_CONSISTENT_LOW=0.75;
const double ELLIPSE_RECT_MINOR_CONSISTENT_HIGH=1.50;
const double RELATIVE_REASON_Z=2.50;
const double GV_RELATIVE_COMPONENT_CLIP=2.0;
const int SMOOTH_WINDOW=11;
const int LOCAL_HALF_WINDOW=25;
const int LONG_HALF_WINDOW=250;
const int GLOBAL_SMOOTH_WINDOW=401;
const int PERSISTENCE_WINDOW=61;
const double PROTRUSION_POINT_THRESHOLD=2.0;
const int PROTRUSION_REGION_MIN_POINTS=30;
const int PROTRUSION_REGION_MERGE_GAP=60;
const double PROTRUSION_BRIDGE_THRESHOLD=1.0;
const double PROTRUSION_REGION_MAX_CONTOUR_RATIO=0.50;
const double PROTRUSION_POLYGON_MIN_AREA_PX=4.0;
const double LOCAL_PROTRUSION_AREA_RATIO=0.05;
const double MAJOR_PROTRUSION_AREA_RATIO=0.15;
const double LOCAL_PROTRUSION_CONTOUR_RATIO=0.10;
const double MAJOR_PROTRUSION_CONTOUR_RATIO=0.25;

struct Feature {
    std::size_t source=0;
    cv::Mat mask;
    std::vector<cv::Point> contour;
    cv::Point2d centroid;
    cv::Point2d rect_center;
    cv::Point2d axis_u;
    cv::Point2d axis_v;
    double rect_major=0.0,rect_minor=0.0,rect_ratio=0.0,k_rect=0.0;
    double ellipse_major=0.0,ellipse_minor=0.0,pose_ratio=0.0,fill_ratio=0.0;
    double ellipse_rect_major_ratio=0.0,ellipse_rect_minor_ratio=0.0;
    double centroid_rect_offset_norm=0.0;
    std::vector<double> width_norm;
    int profile_break_count=0;
    std::string fit_method;
    double original_mask_anomaly_score=0.0;
    std::string mask_quality;
    double protrusion_score=0.0,max_protrusion_span_ratio=0.0;
    int protrusion_region_count=0;
    double protrusion_area_ratio=0.0,protrusion_contour_ratio=0.0;
    std::string geometry_validity,local_failure;
    bool reference_eligible=false;
};

bool finite(double value){return std::isfinite(value);}
bool finitePositive(double value){return finite(value)&&value>EPS;}

double median(std::vector<double> values){
    if(values.empty())return std::numeric_limits<double>::quiet_NaN();
    std::sort(values.begin(),values.end());const std::size_t n=values.size();
    return n%2?values[n/2]:0.5*(values[n/2-1]+values[n/2]);
}

double percentile(std::vector<double> values,double q){
    if(values.empty())return std::numeric_limits<double>::quiet_NaN();
    std::sort(values.begin(),values.end());const double position=q*static_cast<double>(values.size()-1);
    const std::size_t lo=static_cast<std::size_t>(std::floor(position)),hi=static_cast<std::size_t>(std::ceil(position));
    return values[lo]+(position-lo)*(values[hi]-values[lo]);
}

std::vector<double> robustAbsZ(const std::vector<double>& values,double clip=-1.0){
    std::vector<double> valid;for(double v:values)if(finite(v))valid.push_back(v);
    std::vector<double> result(values.size(),std::numeric_limits<double>::quiet_NaN());if(valid.empty())return result;
    const double center=median(valid);std::vector<double> deviations;for(double v:valid)deviations.push_back(std::abs(v-center));
    double mad=median(deviations);if(mad<=EPS){std::vector<double> nonzero;for(double v:deviations)if(v>EPS)nonzero.push_back(v);if(nonzero.empty()){for(std::size_t i=0;i<values.size();++i)if(finite(values[i]))result[i]=0.0;return result;}mad=median(nonzero);}
    for(std::size_t i=0;i<values.size();++i)if(finite(values[i])){result[i]=std::abs(values[i]-center)/(1.4826*mad+EPS);if(clip>=0.0)result[i]=std::min(clip,std::max(0.0,result[i]));}
    return result;
}

std::vector<double> robustHighZ(const std::vector<double>& values,double clip){
    std::vector<double> valid;for(double v:values)if(finite(v))valid.push_back(v);
    std::vector<double> result(values.size(),std::numeric_limits<double>::quiet_NaN());if(valid.empty())return result;
    const double center=median(valid);std::vector<double> deviations;for(double v:valid)deviations.push_back(std::abs(v-center));
    double mad=median(deviations);if(mad<=EPS){std::vector<double> nonzero;for(double v:deviations)if(v>EPS)nonzero.push_back(v);mad=nonzero.empty()?1.0:median(nonzero);}
    for(std::size_t i=0;i<values.size();++i)if(finite(values[i]))result[i]=std::min(clip,std::max(0.0,values[i]-center)/(1.4826*mad+EPS));
    return result;
}

std::vector<int> anomalyRanks(const std::vector<double>& values){
    std::vector<std::size_t> order(values.size());std::iota(order.begin(),order.end(),0);
    std::stable_sort(order.begin(),order.end(),[&](std::size_t a,std::size_t b){const double av=finite(values[a])?values[a]:-std::numeric_limits<double>::infinity();const double bv=finite(values[b])?values[b]:-std::numeric_limits<double>::infinity();return av>bv;});
    std::vector<int> ranks(values.size());for(std::size_t i=0;i<order.size();++i)ranks[order[i]]=static_cast<int>(i+1);return ranks;
}

std::vector<cv::Point> largestContour(const cv::Mat& mask){
    std::vector<std::vector<cv::Point> > contours;cv::findContours(mask.clone(),contours,cv::RETR_EXTERNAL,cv::CHAIN_APPROX_NONE);
    double best=0.0;std::vector<cv::Point> result;for(const std::vector<cv::Point>& contour:contours){const double area=cv::contourArea(contour);if(area>best){best=area;result=contour;}}
    if(result.empty())throw std::invalid_argument("disc gating requires a valid contour");return result;
}

cv::Point2d centroid(const cv::Mat& mask){const cv::Moments m=cv::moments(mask,true);if(m.m00<=EPS)throw std::invalid_argument("disc gating requires a non-empty mask");return cv::Point2d(m.m10/m.m00,m.m01/m.m00);}

int countSegments(const std::vector<double>& values){
    if(values.empty())return 0;const double minimum=*std::min_element(values.begin(),values.end()),maximum=*std::max_element(values.begin(),values.end());
    const int low=static_cast<int>(std::floor(minimum)),high=static_cast<int>(std::ceil(maximum));std::vector<unsigned char> occupancy(static_cast<std::size_t>(high-low+1),0);
    for(double value:values){int index=static_cast<int>(std::nearbyint(value))-low;index=std::max(0,std::min(index,high-low));occupancy[static_cast<std::size_t>(index)]=1;}
    std::vector<int> occupied;for(std::size_t i=0;i<occupancy.size();++i)if(occupancy[i])occupied.push_back(static_cast<int>(i));
    for(std::size_t i=1;i<occupied.size();++i){const int gap=occupied[i]-occupied[i-1]-1;if(gap>0&&gap<=SECTION_GAP_MERGE_PIXELS)for(int j=occupied[i-1]+1;j<occupied[i];++j)occupancy[static_cast<std::size_t>(j)]=1;}
    int segments=0;bool previous=false;for(unsigned char value:occupancy){const bool current=value!=0;if(current&&!previous)++segments;previous=current;}return segments;
}

Feature prepareFeature(const cv::Mat& mask,std::size_t source){
    Feature f;f.source=source;f.mask=mask;f.contour=largestContour(mask);f.centroid=centroid(mask);
    const cv::RotatedRect rect=cv::minAreaRect(f.contour);f.rect_center=rect.center;cv::Point2f box[4];rect.points(box);
    double lengths[4];cv::Point2d edges[4];for(int i=0;i<4;++i){edges[i]=cv::Point2d(box[(i+1)%4])-cv::Point2d(box[i]);lengths[i]=cv::norm(edges[i]);}
    int major_index=0;for(int i=1;i<4;++i)if(lengths[i]>lengths[major_index])major_index=i;
    f.rect_major=lengths[major_index];f.rect_minor=*std::min_element(lengths,lengths+4);f.axis_u=edges[major_index]*(1.0/(f.rect_major+EPS));
    if(f.axis_u.x<0.0||(std::abs(f.axis_u.x)<=EPS&&f.axis_u.y<0.0))f.axis_u*=-1.0;f.axis_v=cv::Point2d(-f.axis_u.y,f.axis_u.x);
    cv::RotatedRect ellipse;if(f.contour.size()>=5){try{ellipse=cv::fitEllipse(f.contour);f.fit_method="FIT_ELLIPSE";}catch(const cv::Exception&){ellipse=rect;f.fit_method="MIN_AREA_RECT_FALLBACK";}}else{ellipse=rect;f.fit_method="MIN_AREA_RECT_FALLBACK";}
    const double axis_a=ellipse.size.width,axis_b=ellipse.size.height;f.ellipse_major=std::max(axis_a,axis_b);f.ellipse_minor=std::min(axis_a,axis_b);
    std::vector<cv::Point> pixels;cv::findNonZero(mask,pixels);std::vector<double> us,vs;us.reserve(pixels.size());vs.reserve(pixels.size());
    double u_min=std::numeric_limits<double>::infinity(),u_max=-u_min;for(const cv::Point& point:pixels){const cv::Point2d relative=cv::Point2d(point)-f.rect_center;const double u=relative.dot(f.axis_u),v=relative.dot(f.axis_v);us.push_back(u);vs.push_back(v);u_min=std::min(u_min,u);u_max=std::max(u_max,u);}
    std::vector<double> widths;std::vector<int> segments;for(double target:CROSS_SECTION_U){std::vector<double> sample;for(std::size_t i=0;i<us.size();++i){const double normalized=2.0*(us[i]-0.5*(u_min+u_max))/(u_max-u_min+EPS);if(std::abs(normalized-target)<=CROSS_SECTION_HALF_BAND)sample.push_back(vs[i]);}if(sample.size()<static_cast<std::size_t>(MIN_SECTION_PIXELS)){widths.push_back(std::numeric_limits<double>::quiet_NaN());segments.push_back(0);}else{widths.push_back(percentile(sample,0.98)-percentile(sample,0.02));segments.push_back(countSegments(sample));}}
    std::vector<double> valid_widths;for(double width:widths)if(finite(width))valid_widths.push_back(width);const double median_width=median(valid_widths);for(double width:widths)f.width_norm.push_back(width/(median_width+EPS));
    for(int value:segments)f.profile_break_count+=std::max(value-1,0);
    const double visible=cv::countNonZero(mask),ellipse_area=CV_PI*f.ellipse_major*f.ellipse_minor/4.0;
    f.rect_ratio=f.rect_major/(f.rect_minor+EPS);f.k_rect=visible/(f.rect_major*f.rect_minor+EPS);f.pose_ratio=f.ellipse_minor/(f.ellipse_major+EPS);f.fill_ratio=visible/(ellipse_area+EPS);
    f.ellipse_rect_major_ratio=f.ellipse_major/(f.rect_major+EPS);f.ellipse_rect_minor_ratio=f.ellipse_minor/(f.rect_minor+EPS);f.centroid_rect_offset_norm=cv::norm(f.centroid-f.rect_center)/(f.rect_major+EPS);return f;
}

void orderByPca(std::vector<Feature>& features){
    cv::Mat points(static_cast<int>(features.size()),2,CV_64F);cv::Point2d center;
    for(std::size_t i=0;i<features.size();++i){points.at<double>(static_cast<int>(i),0)=features[i].centroid.x;points.at<double>(static_cast<int>(i),1)=features[i].centroid.y;center+=features[i].centroid;}center*=1.0/features.size();
    cv::Point2d axis(1.0,0.0);if(features.size()>1){cv::PCA pca(points,cv::Mat(),cv::PCA::DATA_AS_ROW);axis=cv::Point2d(pca.eigenvectors.at<double>(0,0),pca.eigenvectors.at<double>(0,1));}
    if(std::abs(axis.y)>=std::abs(axis.x)){if(axis.y<0.0)axis*=-1.0;}else if(axis.x<0.0)axis*=-1.0;
    std::stable_sort(features.begin(),features.end(),[&](const Feature& a,const Feature& b){return (a.centroid-center).dot(axis)<(b.centroid-center).dot(axis);});
}

void scoreOriginalMasks(std::vector<Feature>& features){
    std::vector<double> k,ratio;for(const Feature& f:features){k.push_back(f.k_rect);ratio.push_back(f.rect_ratio);}const std::vector<double> kz=robustAbsZ(k),rz=robustAbsZ(ratio);
    std::vector<std::vector<double> > profile_z(features.size(),std::vector<double>(5));for(int column=0;column<5;++column){std::vector<double> values;for(const Feature& f:features)values.push_back(f.width_norm[static_cast<std::size_t>(column)]);const std::vector<double> z=robustAbsZ(values,8.0);for(std::size_t i=0;i<features.size();++i)profile_z[i][static_cast<std::size_t>(column)]=z[i];}
    std::vector<double> errors;for(const std::vector<double>& row:profile_z){std::vector<double> valid;for(double value:row)if(finite(value))valid.push_back(value);errors.push_back(valid.empty()?0.0:median(valid));}
    const std::vector<int> kr=anomalyRanks(kz),rr=anomalyRanks(rz),pr=anomalyRanks(errors);
    for(std::size_t i=0;i<features.size();++i){const int top2=(kr[i]<=2)+(rr[i]<=2)+(pr[i]<=2);const double score=0.40*kz[i]+0.30*rz[i]+0.30*errors[i];features[i].original_mask_anomaly_score=score;if(score>=MASK_SCORE_ANOMALOUS||(top2>=2&&score>=1.8))features[i].mask_quality="ANOMALOUS";else if(score<MASK_SCORE_SUSPECT&&top2<2)features[i].mask_quality="NORMAL";else features[i].mask_quality="SUSPECT";}
}

int oddWindow(int window,int count){if(count<=1)return 1;int result=std::min(window,count%2?count:count-1);return std::max(1,result%2?result:result-1);}

std::vector<cv::Point2d> circularAverage(const std::vector<cv::Point2d>& values,int window){const int actual=oddWindow(window,static_cast<int>(values.size())),half=actual/2,count=static_cast<int>(values.size());std::vector<cv::Point2d> result(values.size());for(int i=0;i<count;++i)for(int offset=-half;offset<=half;++offset)result[static_cast<std::size_t>(i)]+=values[static_cast<std::size_t>((i+offset+count)%count)]*(1.0/actual);return result;}
std::vector<double> circularAverage(const std::vector<double>& values,int window){const int actual=oddWindow(window,static_cast<int>(values.size())),half=actual/2,count=static_cast<int>(values.size());std::vector<double> result(values.size(),0.0);for(int i=0;i<count;++i)for(int offset=-half;offset<=half;++offset)result[static_cast<std::size_t>(i)]+=values[static_cast<std::size_t>((i+offset+count)%count)]/actual;return result;}

bool maskSample(const cv::Mat& mask,const cv::Point2d& point){const int x=std::max(0,std::min(static_cast<int>(std::nearbyint(point.x)),mask.cols-1)),y=std::max(0,std::min(static_cast<int>(std::nearbyint(point.y)),mask.rows-1));return mask.at<unsigned char>(y,x)!=0;}

void outwardNormals(const cv::Mat& mask,const std::vector<cv::Point2d>& points,std::vector<cv::Point2d>& normals,std::vector<unsigned char>& valid){const int count=static_cast<int>(points.size());normals.assign(points.size(),cv::Point2d());valid.assign(points.size(),0);for(int i=0;i<count;++i){cv::Point2d tangent=points[static_cast<std::size_t>((i+3)%count)]-points[static_cast<std::size_t>((i-3+count)%count)];const double length=cv::norm(tangent);if(length<=EPS)continue;tangent*=1.0/length;cv::Point2d normal(-tangent.y,tangent.x);int plus=0,minus=0;for(int d=2;d<=4;++d){plus+=maskSample(mask,points[static_cast<std::size_t>(i)]+d*normal);minus+=maskSample(mask,points[static_cast<std::size_t>(i)]-d*normal);}if(plus>minus)normal*=-1.0;normals[static_cast<std::size_t>(i)]=normal;valid[static_cast<std::size_t>(i)]=1;}}

std::vector<double> chordScores(const std::vector<cv::Point2d>& points,const std::vector<cv::Point2d>& normals,const std::vector<unsigned char>& valid,int half){const int count=static_cast<int>(points.size());std::vector<double> scores(points.size(),0.0);for(int i=0;i<count;++i){const cv::Point2d previous=points[static_cast<std::size_t>((i-half+count)%count)],following=points[static_cast<std::size_t>((i+half)%count)];const cv::Point2d chord=following-previous;const double length=cv::norm(chord);if(!valid[static_cast<std::size_t>(i)]||length<=EPS)continue;cv::Point2d normal(-chord.y/length,chord.x/length);if(normal.dot(normals[static_cast<std::size_t>(i)])<0.0)normal*=-1.0;scores[static_cast<std::size_t>(i)]=std::max(0.0,(points[static_cast<std::size_t>(i)]-previous).dot(normal));}return scores;}

std::vector<double> robustPositive(const std::vector<double>& values){const double center=median(values);std::vector<double> deviation;for(double value:values)deviation.push_back(std::abs(value-center));const double mad=median(deviation);std::vector<double> result;for(double value:values)result.push_back(std::min(8.0,std::max(0.0,(value-center)/(1.4826*mad+EPS))));return result;}

std::vector<std::vector<int> > circularRuns(const std::vector<unsigned char>& binary){const int count=static_cast<int>(binary.size());if(count==0||std::find(binary.begin(),binary.end(),1)==binary.end())return std::vector<std::vector<int> >();if(std::find(binary.begin(),binary.end(),0)==binary.end()){std::vector<int> all(static_cast<std::size_t>(count));std::iota(all.begin(),all.end(),0);return std::vector<std::vector<int> >(1,all);}int first_zero=0;while(binary[static_cast<std::size_t>(first_zero)])++first_zero;const int start=(first_zero+1)%count;std::vector<std::vector<int> > runs;std::vector<int> current;for(int offset=0;offset<count;++offset){const int index=(start+offset)%count;if(binary[static_cast<std::size_t>(index)])current.push_back(index);else if(!current.empty()){runs.push_back(current);current.clear();}}if(!current.empty())runs.push_back(current);return runs;}

std::vector<int> circularGap(const std::vector<int>& first,const std::vector<int>& second,int count){const int length=(second.front()-first.back()-1+count)%count;std::vector<int> result;for(int step=1;step<=length;++step)result.push_back((first.back()+step)%count);return result;}

std::vector<std::vector<int> > detectRegions(const std::vector<double>& scores){std::vector<unsigned char> binary(scores.size());for(std::size_t i=0;i<scores.size();++i)binary[i]=scores[i]>=PROTRUSION_POINT_THRESHOLD;std::vector<std::vector<int> > regions;for(const std::vector<int>& run:circularRuns(binary))if(run.size()>=static_cast<std::size_t>(PROTRUSION_REGION_MIN_POINTS))regions.push_back(run);bool changed=true;while(changed&&regions.size()>1){changed=false;std::vector<std::vector<int> > merged;std::size_t index=0;while(index<regions.size()){std::vector<int> current=regions[index];if(index+1<regions.size()){const std::vector<int> bridge=circularGap(current,regions[index+1],static_cast<int>(scores.size()));double mean=std::numeric_limits<double>::infinity();if(!bridge.empty()){mean=0.0;for(int i:bridge)mean+=scores[static_cast<std::size_t>(i)];mean/=bridge.size();}if(bridge.size()<=static_cast<std::size_t>(PROTRUSION_REGION_MERGE_GAP)&&mean>=PROTRUSION_BRIDGE_THRESHOLD){current.insert(current.end(),bridge.begin(),bridge.end());current.insert(current.end(),regions[index+1].begin(),regions[index+1].end());++index;changed=true;}}merged.push_back(current);++index;}regions.swap(merged);}return regions;}

std::vector<std::vector<int> > mergeRegions(const std::vector<std::vector<int> >& regions,int count){if(count<=0||regions.empty())return std::vector<std::vector<int> >();std::vector<unsigned char> covered(static_cast<std::size_t>(count),0);for(const std::vector<int>& region:regions)for(int i:region)if(i>=0&&i<count)covered[static_cast<std::size_t>(i)]=1;return circularRuns(covered);}

void protrusionDiagnostic(Feature& f){
    std::vector<cv::Point2d> raw;for(const cv::Point& point:f.contour)raw.push_back(point);const std::vector<cv::Point2d> points=circularAverage(raw,SMOOTH_WINDOW);std::vector<cv::Point2d> normals;std::vector<unsigned char> valid;outwardNormals(f.mask,points,normals,valid);
    std::vector<double> local=chordScores(points,normals,valid,std::min(LOCAL_HALF_WINDOW,std::max(1,static_cast<int>(points.size())/4)));const int long_window=std::min(std::max(40,std::min(LONG_HALF_WINDOW,static_cast<int>(points.size())/8)),std::max(1,static_cast<int>(points.size())/2-1));std::vector<double> longer=chordScores(points,normals,valid,long_window);const std::vector<cv::Point2d> trend=circularAverage(points,GLOBAL_SMOOTH_WINDOW);
    std::vector<double> global(points.size(),0.0);for(std::size_t i=0;i<points.size();++i)if(valid[i])global[i]=std::max(0.0,(points[i]-trend[i]).dot(normals[i]));local=robustPositive(local);longer=robustPositive(longer);global=robustPositive(global);std::vector<double> multi(points.size());for(std::size_t i=0;i<points.size();++i)multi[i]=0.25*local[i]+0.40*longer[i]+0.35*global[i];const std::vector<double> persistent=circularAverage(multi,PERSISTENCE_WINDOW);std::vector<double> final(points.size());for(std::size_t i=0;i<points.size();++i)final[i]=0.6*persistent[i]+0.4*multi[i];
    std::vector<std::vector<int> > regions=mergeRegions(detectRegions(final),static_cast<int>(raw.size())),major;for(const std::vector<int>& run:regions){double mean=0.0,maximum=0.0;for(int i:run){mean+=final[static_cast<std::size_t>(i)];maximum=std::max(maximum,final[static_cast<std::size_t>(i)]);}mean/=run.size();if(run.size()>=80&&mean>=2.0&&maximum>=3.0)major.push_back(run);}const std::vector<std::vector<int> >& selected=major.empty()?regions:major;f.protrusion_region_count=static_cast<int>(selected.size());int high=0;for(double value:final)high+=value>=PROTRUSION_POINT_THRESHOLD;f.protrusion_score=static_cast<double>(high)/final.size();for(const std::vector<int>& run:selected)f.max_protrusion_span_ratio=std::max(f.max_protrusion_span_ratio,run.size()/(points.size()+EPS));
    double total_length=0.0,abnormal_length=0.0;for(std::size_t i=0;i<raw.size();++i)total_length+=cv::norm(raw[(i+1)%raw.size()]-raw[i]);const std::vector<std::vector<int> > merged=mergeRegions(selected,static_cast<int>(raw.size()));for(const std::vector<int>& run:merged)for(std::size_t i=1;i<run.size();++i)abnormal_length+=cv::norm(raw[static_cast<std::size_t>(run[i])]-raw[static_cast<std::size_t>(run[i-1])]);f.protrusion_contour_ratio=abnormal_length/(total_length+EPS);
    cv::Mat union_mask=cv::Mat::zeros(f.mask.size(),CV_8UC1);for(const std::vector<int>& run:merged){if(run.size()<3||run.size()/(raw.size()+EPS)>PROTRUSION_REGION_MAX_CONTOUR_RATIO)continue;std::vector<cv::Point> polygon;cv::Point2d polygon_sum;for(int i:run)polygon.push_back(cv::Point(cvRound(raw[static_cast<std::size_t>(i)].x),cvRound(raw[static_cast<std::size_t>(i)].y)));if(std::abs(cv::contourArea(polygon))<PROTRUSION_POLYGON_MIN_AREA_PX)continue;const cv::Moments moments=cv::moments(polygon);if(std::abs(moments.m00)<=EPS)continue;const cv::Point2d polygon_center(moments.m10/moments.m00,moments.m01/moments.m00),chord_center=0.5*(raw[static_cast<std::size_t>(run.front())]+raw[static_cast<std::size_t>(run.back())]);cv::Point2d mean_normal;for(int i:run)mean_normal+=normals[static_cast<std::size_t>(i)];mean_normal*=1.0/run.size();const double normal_length=cv::norm(mean_normal);if(normal_length<=EPS||(polygon_center-chord_center).dot(mean_normal*(1.0/normal_length))<=0.0)continue;cv::Mat region=cv::Mat::zeros(f.mask.size(),CV_8UC1);std::vector<std::vector<cv::Point> > polygons(1,polygon);cv::fillPoly(region,polygons,cv::Scalar(1));cv::bitwise_and(region,f.mask,region);if(cv::countNonZero(region)>0)cv::bitwise_or(union_mask,region,union_mask);}
    f.protrusion_area_ratio=cv::countNonZero(union_mask)/(static_cast<double>(cv::countNonZero(f.mask))+EPS);
}

bool geometryConsistent(const Feature& f){return ELLIPSE_RECT_MAJOR_CONSISTENT_LOW<=f.ellipse_rect_major_ratio&&f.ellipse_rect_major_ratio<=ELLIPSE_RECT_MAJOR_CONSISTENT_HIGH&&ELLIPSE_RECT_MINOR_CONSISTENT_LOW<=f.ellipse_rect_minor_ratio&&f.ellipse_rect_minor_ratio<=ELLIPSE_RECT_MINOR_CONSISTENT_HIGH&&f.fill_ratio>=FILL_RATIO_CONSISTENCY_LOW;}

void scoreGeometry(std::vector<Feature>& features){
    std::vector<double> major,minor,fill,offset,breaks,protrusion;for(const Feature& f:features){major.push_back(finitePositive(f.ellipse_rect_major_ratio)?std::abs(std::log(f.ellipse_rect_major_ratio)):std::numeric_limits<double>::quiet_NaN());minor.push_back(finitePositive(f.ellipse_rect_minor_ratio)?std::abs(std::log(f.ellipse_rect_minor_ratio)):std::numeric_limits<double>::quiet_NaN());fill.push_back(f.fill_ratio);offset.push_back(f.centroid_rect_offset_norm);breaks.push_back(f.profile_break_count);protrusion.push_back(f.protrusion_score);}
    const std::vector<double> major_z=robustAbsZ(major,8.0),minor_z=robustAbsZ(minor,8.0),fill_z=robustAbsZ(fill,8.0),offset_z=robustAbsZ(offset,8.0),break_z=robustAbsZ(breaks,8.0),protrusion_z=robustHighZ(protrusion,8.0);
    for(std::size_t i=0;i<features.size();++i){Feature& f=features[i];std::vector<std::string> reasons;const bool mismatch=!finitePositive(f.ellipse_rect_minor_ratio)||f.ellipse_rect_minor_ratio>ELLIPSE_RECT_MINOR_INVALID_HIGH||f.ellipse_rect_minor_ratio<ELLIPSE_RECT_MINOR_INVALID_LOW||!finitePositive(f.ellipse_rect_major_ratio)||f.ellipse_rect_major_ratio>ELLIPSE_RECT_MAJOR_INVALID_HIGH||f.ellipse_rect_major_ratio<ELLIPSE_RECT_MAJOR_INVALID_LOW;if(!finitePositive(f.ellipse_rect_minor_ratio)||f.ellipse_rect_minor_ratio>ELLIPSE_RECT_MINOR_INVALID_HIGH||f.ellipse_rect_minor_ratio<ELLIPSE_RECT_MINOR_INVALID_LOW)reasons.push_back("ELLIPSE_RECT_MINOR_MISMATCH");if(!finitePositive(f.ellipse_rect_major_ratio)||f.ellipse_rect_major_ratio>ELLIPSE_RECT_MAJOR_INVALID_HIGH||f.ellipse_rect_major_ratio<ELLIPSE_RECT_MAJOR_INVALID_LOW)reasons.push_back("ELLIPSE_RECT_MAJOR_MISMATCH");if(!finite(f.fill_ratio)||f.fill_ratio<FILL_RATIO_EXTREME_LOW)reasons.push_back("EXTREME_LOW_FILL");if(finite(f.pose_ratio)&&f.pose_ratio>POSE_RATIO_EXTREME_HIGH)reasons.push_back("ELLIPSE_NEAR_CIRCULAR");const bool fit_unstable=f.fit_method!="FIT_ELLIPSE";if(fit_unstable)reasons.push_back("FIT_UNSTABLE");const bool consistent=geometryConsistent(f),low_fill=f.fill_ratio<FILL_RATIO_EXTREME_LOW,centroid_bad=offset_z[i]>=RELATIVE_REASON_Z||f.centroid_rect_offset_norm>=CENTROID_OFFSET_ABSOLUTE_HIGH,supporting=!consistent||protrusion_z[i]>=RELATIVE_REASON_Z||f.original_mask_anomaly_score>=MASK_SCORE_SUSPECT,detected=f.protrusion_region_count>0&&((finite(f.protrusion_area_ratio)&&f.protrusion_area_ratio>EPS)||(finite(f.protrusion_contour_ratio)&&f.protrusion_contour_ratio>EPS));
        const bool major_extent=(finite(f.protrusion_area_ratio)&&f.protrusion_area_ratio>=MAJOR_PROTRUSION_AREA_RATIO)||(finite(f.protrusion_contour_ratio)&&f.protrusion_contour_ratio>=MAJOR_PROTRUSION_CONTOUR_RATIO);std::string severity;if(!detected)severity="NONE";else if(major_extent&&supporting)severity="MAJOR";else if(consistent&&!supporting)severity="LOCAL";else if(finite(f.protrusion_area_ratio)&&f.protrusion_area_ratio<=LOCAL_PROTRUSION_AREA_RATIO&&finite(f.protrusion_contour_ratio)&&f.protrusion_contour_ratio<=LOCAL_PROTRUSION_CONTOUR_RATIO&&f.max_protrusion_span_ratio<=LOCAL_PROTRUSION_CONTOUR_RATIO&&consistent)severity="LOCAL";else severity="MODERATE";
        if(centroid_bad)reasons.push_back("CENTROID_OFFSET");const bool fragmented=break_z[i]>=RELATIVE_REASON_Z||f.profile_break_count>=PROFILE_BREAK_ABSOLUTE_HIGH;if(fragmented)reasons.push_back("PROFILE_FRAGMENTED");if(fill_z[i]>=RELATIVE_REASON_Z&&std::find(reasons.begin(),reasons.end(),"EXTREME_LOW_FILL")==reasons.end())reasons.push_back("FILL_RATIO_OUTLIER");const bool profile_supported=mismatch||low_fill||centroid_bad;const double score=0.35*std::min(minor_z[i],GV_RELATIVE_COMPONENT_CLIP)+0.20*std::min(major_z[i],GV_RELATIVE_COMPONENT_CLIP)+0.20*std::min(fill_z[i],GV_RELATIVE_COMPONENT_CLIP)+0.15*std::min(offset_z[i],GV_RELATIVE_COMPONENT_CLIP)+0.10*std::min(break_z[i],GV_RELATIVE_COMPONENT_CLIP)*profile_supported;
        const int moderate=!(ELLIPSE_RECT_MAJOR_CONSISTENT_LOW<=f.ellipse_rect_major_ratio&&f.ellipse_rect_major_ratio<=ELLIPSE_RECT_MAJOR_CONSISTENT_HIGH)+!(ELLIPSE_RECT_MINOR_CONSISTENT_LOW<=f.ellipse_rect_minor_ratio&&f.ellipse_rect_minor_ratio<=ELLIPSE_RECT_MINOR_CONSISTENT_HIGH)+(f.fill_ratio<FILL_RATIO_CONSISTENCY_LOW);const int secondary=centroid_bad+(moderate>=2);bool hard=false;for(const std::string& reason:reasons)if(reason=="ELLIPSE_RECT_MINOR_MISMATCH"||reason=="ELLIPSE_RECT_MAJOR_MISMATCH"||reason=="EXTREME_LOW_FILL"||reason=="ELLIPSE_NEAR_CIRCULAR"||reason=="FIT_UNSTABLE")hard=true;int auxiliary=0;for(const std::string& reason:reasons)if(reason=="PROFILE_FRAGMENTED"||reason=="FILL_RATIO_OUTLIER"||reason=="CENTROID_OFFSET")++auxiliary;if(hard||secondary>=2)f.geometry_validity="INVALID";else if(auxiliary>=2)f.geometry_validity="UNCERTAIN";else if(score<1.50)f.geometry_validity="VALID";else f.geometry_validity="UNCERTAIN";
        const bool fill_outlier=std::find(reasons.begin(),reasons.end(),"FILL_RATIO_OUTLIER")!=reasons.end();const bool auxiliary_weak=false;if(severity=="MAJOR")f.local_failure="MAJOR_MASK_PROTRUSION";else if(mismatch&&low_fill&&severity!="MAJOR")f.local_failure="OCCLUSION_OR_TRUNCATION";else if(fit_unstable||(mismatch&&auxiliary_weak))f.local_failure="FIT_UNSTABLE";else if(severity=="LOCAL"&&f.geometry_validity=="VALID")f.local_failure="LOCAL_CONTOUR_DEVIATION";else if(severity=="NONE"&&consistent)f.local_failure="NONE";else f.local_failure="UNCERTAIN";(void)fill_outlier;
        if(f.geometry_validity=="INVALID"||severity=="MAJOR"||f.local_failure=="MAJOR_MASK_PROTRUSION"||f.mask_quality=="ANOMALOUS"||f.geometry_validity!="VALID")f.reference_eligible=false;else if(f.mask_quality=="NORMAL")f.reference_eligible=true;else if(f.mask_quality=="SUSPECT")f.reference_eligible=f.local_failure=="NONE"||f.local_failure=="LOCAL_CONTOUR_DEVIATION"||(severity=="MODERATE"&&f.local_failure=="UNCERTAIN");else f.reference_eligible=false;
    }
}

}  // namespace

namespace insulator {

void InsulatorQuantifier::computeGating(std::vector<DiscResult>& discs,const std::vector<cv::Mat>& masks) const{
    if(discs.size()!=masks.size()||discs.empty())throw std::invalid_argument("disc gating input mismatch");std::vector<Feature> features;features.reserve(masks.size());for(std::size_t i=0;i<masks.size();++i)features.push_back(prepareFeature(masks[i],i));orderByPca(features);scoreOriginalMasks(features);for(Feature& feature:features)protrusionDiagnostic(feature);scoreGeometry(features);for(const Feature& feature:features){DiscResult& disc=discs[feature.source];disc.mask_quality=feature.mask_quality;disc.geometry_validity=feature.geometry_validity;disc.local_failure=feature.local_failure;disc.reference_eligible=feature.reference_eligible;}
}

}  // namespace insulator

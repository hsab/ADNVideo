#pragma once

#include <opencv2/opencv.hpp>
#include <algorithm>

namespace amu {

    /** Create multiple scaled down versions of an image */
    void MakeMultiResolution(const cv::Mat& image, cv::vector<cv::Mat>& scaled, int steps = 3) {
        scaled.clear();
        for(int i = 0; i < steps; i++) {
            scaled.push_back(cv::Mat());
            int size = 1 << (i + 1);
            cv::resize(image, scaled.back(), cv::Size(image.cols / size, image.rows / size), 0, 0, cv::INTER_AREA);
        }
    }

    /** Generate a mask with pixels likely to be skin */
    void GetSkinPixels(const cv::Mat &image, cv::Mat& output) {
        cv::Mat hsv;
        cv::cvtColor(image, hsv, CV_BGR2HSV);
        cv::Mat hue(image.size(), CV_8UC1), saturation(image.size(), CV_8UC1), value(image.size(), CV_8UC1);
        std::vector<cv::Mat> channels;
        channels.push_back(hue);
        channels.push_back(saturation);
        channels.push_back(value);
        cv::split(hsv, channels);
        cv::threshold(hue, hue, 18, 255, CV_THRESH_BINARY_INV);
        cv::threshold(saturation, saturation, 50, 255, CV_THRESH_BINARY);
        cv::threshold(value, value, 80, 255, CV_THRESH_BINARY);
        if(output.empty() || output.rows != image.rows || output.cols != image.cols)
            output.create(image.size(), CV_8UC1);
        output = hue & saturation & value;
    }

    /** Reduce rectangle size to fit non-null pixels of a mask */
    cv::Rect AdjustToMask(const cv::Rect rect, const cv::Mat& mask) {
        cv::Mat reduced(mask, rect);
        cv::vector<cv::vector<cv::Point> > contours;
        cv::vector<cv::Vec4i> hierarchy;
        cv::findContours(reduced, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
        return cv::boundingRect(contours[0]) + cv::Point(rect.x, rect.y);
    }

    double SelectedRatio(const cv::Mat& mask, cv::Rect rect = cv::Rect()) {
        if(rect.width == 0) rect = cv::Rect(0, 0, mask.cols, mask.rows);
        cv::Mat selected = mask(rect);
        cv::Scalar sums = cv::sum(selected);
        double result = sums[0] / (selected.rows * selected.cols * 255);
        return result;
    }

    /** Draw the pixels from image which are non-null in mask */
    void DrawMasked(const cv::Mat& image, const cv::Mat& mask, cv::Mat& output) {
        image.copyTo(output, mask & 1);
    }

    /** Draw the contour of a single masked area */
    void DrawContour(const cv::Mat& mask, cv::Mat& output, cv::Scalar color = cv::Scalar(0, 255, 0)) {
        cv::vector<cv::vector<cv::Point> > contours;
        cv::vector<cv::Vec4i> hierarchy;
        cv::findContours(mask, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
        cv::drawContours(output, contours, 0, color);
    }

    cv::vector<cv::Point> ConvexHull(const cv::vector<cv::Point> &points) {
        if(points.size() == 0) return points;
        cv::vector<cv::Point> output;
        cv::convexHull(cv::Mat_<cv::Point>(cv::Mat(points)), output);
        return output;
    }

    /** Find and draw the convex hull of a set of points */
    void DrawConvexHull(const cv::vector<cv::Point2f> points, cv::Mat& output, cv::Scalar color) {
        cv::vector<cv::Point> hull;
        cv::convexHull(cv::Mat_<cv::Point>(cv::Mat(points)), hull);
        cv::fillConvexPoly(output, &hull[0], (int) hull.size(), color, 8, 0);
    }

    cv::vector<cv::Point> AsPoint(const cv::vector<cv::Point2f> &points) {
        cv::vector<cv::Point> output;
        for(size_t i = 0; i < points.size(); i++) {
            output.push_back(points[i]);
        }
        return output;
    }

    /** Rescale a rectangle */
    void Scale(cv::Rect& rect, double factor) {
        rect.x *= factor;
        rect.y *= factor;
        rect.width *= factor;
        rect.height *= factor;
    }

    void Shrink(std::vector<cv::Point>& polygon, const cv::Point& center, double factor = 0.5) {
        for(size_t i = 0; i < polygon.size(); i++) {
            cv::Point vector = polygon[i] - center;
            polygon[i] = center + vector * factor;
        }
    }

    std::vector<cv::Point> Shrinked(const std::vector<cv::Point>& polygon, const cv::Point& center, double factor = 0.5) {
        std::vector<cv::Point> output = polygon;
        Shrink(output, center, factor);
        return output;
    }

    void Shrink(std::vector<cv::Point>& polygon, double factor = 0.5) {
        cv::Point center(0, 0);
        for(size_t i = 0; i < polygon.size(); i++) {
            center += polygon[i];
        }
        center *= 1.0 / polygon.size();
        for(size_t i = 0; i < polygon.size(); i++) {
            cv::Point vector = polygon[i] - center;
            polygon[i] = center + vector * factor;
        }
    }

    std::vector<cv::Point> Shrinked(const std::vector<cv::Point>& polygon, double factor = 0.5) {
        std::vector<cv::Point> output = polygon;
        Shrink(output, factor);
        return output;
    }

    cv::Rect Scaled(const cv::Rect& rect, double factor) {
        return cv::Rect(rect.x * factor, rect.y * factor, rect.width * factor, rect.height * factor);
    }

    /** Rescale a vector of rectangles */
    void Scale(cv::vector<cv::Rect>& rects, double factor) {
        for(size_t i = 0; i < rects.size(); i++) {
            Scale(rects[i], factor);
        }
    }

    cv::vector<cv::Rect> Scaled(const cv::vector<cv::Rect>& rects, double factor) {
        cv::vector<cv::Rect> output;
        for(size_t i = 0; i < rects.size(); i++) {
            output.push_back(Scaled(rects[i], factor));
        }
        return output;
    }

    void Scale(cv::vector<cv::Point> &polygon, double factor) {
        for(size_t i = 0; i < polygon.size(); i++) {
            polygon[i] *= factor;
        }
    }

    cv::vector<cv::Point> Scaled(const cv::vector<cv::Point> &polygon, double factor) {
        cv::vector<cv::Point> output = polygon;
        Scale(output, factor);
        return output;
    }

    /** Perform multiple erode iterations */
    void Erode(cv::Mat& mask, int iterations = 1) {
        cv::Mat element = cv::getStructuringElement( cv::MORPH_ELLIPSE, cv::Size(5, 5), cv::Point(2, 2));
        for(int i = 0; i < iterations; i++) {
            cv::erode(mask, mask, element);
        }
    }

    cv::Rect BoundingBox(const std::vector<cv::Point2f>& points) {
        return cv::boundingRect(points);
    }


    cv::Point Center(const cv::vector<cv::Point>& points) {
        cv::Point output(0, 0);
        for(size_t i = 0; i < points.size(); i++) {
            output += points[i];
        }
        output *= 1.0 / points.size();
        return output;
    }

    /** Fast approximation along axes */
    cv::Point GeometricMedian(const cv::vector<cv::Point>& points) {
        cv::Point median;
        std::vector<double> values;
        for(size_t i = 0; i < points.size(); i++) values.push_back(points[i].x);
        std::sort(values.begin(), values.end());
        median.x = values[values.size() / 2];
        values.clear();
        for(size_t i = 0; i < points.size(); i++) values.push_back(points[i].y);
        std::sort(values.begin(), values.end());
        median.y = values[values.size() / 2];
        return median;
    }

    cv::Rect MedianBoundingBox(const cv::vector<cv::Point>& points, const cv::Point center) {
        cv::Rect rect;
        std::vector<double> values; 
        for(size_t i = 0; i < points.size(); i++) values.push_back(fabs(points[i].x - center.x));
        std::sort(values.begin(), values.end());
        rect.width = 8 * values[values.size() / 2];
        for(size_t i = 0; i < points.size(); i++) values.push_back(fabs(points[i].y - center.y));
        std::sort(values.begin(), values.end());
        rect.height = 8 * values[values.size() / 2];
        rect.x = center.x - rect.width / 2;
        rect.y = center.y - rect.height / 2;
        return rect;
    }

    double MedianDistance(const cv::vector<cv::Point>& points, const cv::Point& center) {
        if(points.size() == 0) return 0;
        std::vector<double> distances; 
        for(size_t i = 0; i < points.size(); i++) {
            distances.push_back(cv::norm(points[i] - center));
        }
        std::sort(distances.begin(), distances.end());
        return distances[distances.size() / 2];
    }

    cv::vector<cv::Point> MedianFilter(const cv::vector<cv::Point>& points, const cv::Point& center, double ratio = 1) {
        cv::vector<cv::Point> output;
        double radius = MedianDistance(points, center);
        for(size_t i = 0; i < points.size(); i++) {
            if(cv::norm(points[i] - center) < radius * ratio) {
                output.push_back(points[i]);
            }
        }
        return output;
    }

    /** Find the boundaries of an object using grab-cut */
    void GrabCut(const cv::Mat& image, const cv::vector<cv::Point>& polygon, cv::Mat &output, bool probableBackground = false, int fraction = 8) {
        cv::Size zoom(image.size().width / fraction, image.size().height / fraction);

        cv::Mat resized_image, mask(zoom, CV_8UC1);
        cv::resize(image, resized_image, zoom, cv::INTER_AREA);

        // build mask
        if(probableBackground) {
            mask.setTo(cv::GC_PR_BGD);
        } else {
            mask.setTo(cv::GC_BGD);
            cv::fillPoly(mask, std::vector<std::vector<cv::Point> >(1, Shrinked(Scaled(polygon, 1.0 / fraction), 1.2)), cv::GC_PR_BGD);
        }
        cv::fillPoly(mask, std::vector<std::vector<cv::Point> >(1, Shrinked(Scaled(polygon, 1.0 / fraction), 0.8)), cv::GC_PR_FGD);
        cv::fillPoly(mask, std::vector<std::vector<cv::Point> >(1, Shrinked(Scaled(polygon, 1.0 / fraction), 0.5)), cv::GC_FGD);

        // apply one iteration of grab cut
        cv::Mat bgModel, fgModel;
        cv::grabCut(resized_image, mask, cv::Rect(), bgModel, fgModel, 1, cv::GC_INIT_WITH_MASK);
        mask &= 1;

        cv::resize(mask, output, image.size());
        output &= 1;

        // find main connected component
        /*cv::floodFill(output, cv::Point(rect.x + rect.width / 2, rect.y + rect.height / 2), cv::Scalar(2));
        output = output & 2;*/
    }

    /** Find the boundaries of an object using grab-cut */
    void GrabCut(const cv::Mat& image, const cv::Rect& rect, cv::Mat &output) {
        // build mask
        if(output.empty() || output.rows != image.rows || output.cols != image.cols)
            output.create(image.size(), CV_8UC1);
        cv::Point center(rect.x + rect.width / 2, rect.y + rect.height / 2);

        output.setTo(cv::GC_BGD);
        cv::ellipse(output, center, cv::Size(rect.width, rect.height), 0, 0, 360, cv::GC_PR_BGD, CV_FILLED);
        cv::ellipse(output, center, cv::Size(rect.width / 2, rect.height / 2), 0, 0, 360, cv::GC_PR_FGD, CV_FILLED);
        cv::ellipse(output, center, cv::Size(rect.width / 4, rect.height / 4), 0, 0, 360, cv::GC_FGD, CV_FILLED);

        // apply one iteration of grab cut
        cv::Mat bgModel, fgModel;
        cv::grabCut(image, output, rect, bgModel, fgModel, 1, cv::GC_INIT_WITH_MASK);
        cv::imshow("grab", output * 50);
        output = output & 1;

        // find main connected component
        /*cv::floodFill(output, cv::Point(rect.x + rect.width / 2, rect.y + rect.height / 2), cv::Scalar(2));
        output = output & 2;*/
    }

    /** Simple object tracker */
    struct Tracker {
        cv::vector<cv::Point2f> points, matchedPoints;
        cv::Mat previous;

        void AddModel(const cv::Mat& gray, const cv::vector<cv::Point>& polygon) {
            points.clear();
            for(size_t i = 0; i < polygon.size(); i++) {
                points.push_back((polygon[i]));
            }
            gray.copyTo(previous);
        }

        void AddModel(const cv::Mat& gray, const cv::Mat& mask) {
            cv::goodFeaturesToTrack(gray, points, 100, 0.01, 3, mask);
            gray.copyTo(previous);
        }

        bool Find(const cv::Mat& gray, bool keepOriginal = false) {
            if(points.size() == 0) return false;
            std::vector<cv::Point2f> newPoints;
            std::vector<uchar> status;
            std::vector<float> error;
            cv::calcOpticalFlowPyrLK(previous, gray, points, newPoints, status, error);
            matchedPoints.clear();
            for(size_t i = 0; i < newPoints.size(); i++) {
                if(status[i] == 1) {
                    matchedPoints.push_back(newPoints[i]);
                }
            }
            if(keepOriginal == false) {
                gray.copyTo(previous);
                points = matchedPoints;
            }
            return matchedPoints.size() > points.size() / 2;
        }

        void Draw(cv::Mat& display, int size = 3, cv::Scalar color = cv::Scalar(255, 0, 0)) {
            for(size_t i = 0; i < matchedPoints.size(); i++) {
                cv::circle(display, matchedPoints[i], size, color, CV_FILLED);
            }
        }
    };

}

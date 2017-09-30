#include <opencv/cv.h>
#include <vector>
#include "features/featuresSIMULATED.h"
#include <iostream>
#include <time.h>
#include <opencv2/imgproc.hpp>
#pragma warning (disable : 4244) //disable conversion double to float

using namespace std;
using namespace cv;

///Constructor
featuresSIMULATED::featuresSIMULATED(void){}

void featuresSIMULATED::Apply(std::vector<Feature> &Features1, std::vector<Feature> &Features2,
cv::Mat kleft, cv::Mat kright, int image_w, int image_h, cv::Mat T_1to2, int numberFeatures)
{
    int min_distance = 250; //750;
    int error = 10000;
    int max_distance = 500;
    //srand(time(NULL));

    Mat images = Mat::zeros(image_h, image_w, CV_8UC3);
    Mat left = Mat::zeros(image_h, image_w, CV_64F);
    Mat right = Mat::zeros(image_h, image_w, CV_64F);

    for(int n=0; n<numberFeatures; n++)
    {
        Mat left_point = Mat::ones(4,1,CV_64F);
        double x, y, z;
        bool perfect_points = false;

        /*int points_type = (rand() % 100);
        if(points_type < 50)
            perfect_points = false;//*/

        if(perfect_points)
        {
            int type = (rand() % 100);

            //points for rx
            if(type < 33)
            {
                z = (rand() % max_distance)+min_distance/2;
                y = z;// + (rand() % error)-error/2;
                int sign = (rand() % 100);
                if(sign < 50)
                    y = -y;
                x = (rand() % max_distance)-max_distance/2;
            }
            //points for ry
            else if(type >= 33 && type < 67)
            {
                z = (rand() % max_distance)+min_distance/2;
                x = z;// + (rand() % error)-error/2;
                int sign = (rand() % 100);
                if(sign < 50)
                    x = -x;
                y = (rand() % max_distance)-max_distance/2;
            }
            //points for rz
            else
            {
                z = (rand() % max_distance)+min_distance/2;
                double d = z;// + (rand() % error)-error/2;
                y = (rand() % max_distance)-max_distance/2;
                x = sqrt(d*d - y*y);
                int sign = (rand() % 100);
                if(sign < 50)
                    x = -x;

            }
        }
        else
        {
            int max_x_y = 2*(min_distance+max_distance);
            x = (rand() % max_x_y)-double(max_x_y)/2;
            y = (rand() % max_x_y)-double(max_x_y)/2;
            z = (rand() % max_distance)+min_distance; //min_distance;
        }

        left_point.at<double>(0,0) = x;
        left_point.at<double>(1,0) = y;
        left_point.at<double>(2,0) = z;//*/

        Mat left_norm_point = Mat::ones(3,1,CV_64F);
        left_norm_point.at<double>(0,0) = left_point.at<double>(0,0)/left_point.at<double>(2,0);
        left_norm_point.at<double>(1,0) = left_point.at<double>(1,0)/left_point.at<double>(2,0);

        Mat right_point = T_1to2*left_point;
        Mat right_norm_point = Mat::ones(3,1,CV_64F);
        right_norm_point.at<double>(0,0) = right_point.at<double>(0,0)/right_point.at<double>(2,0);
        right_norm_point.at<double>(1,0) = right_point.at<double>(1,0)/right_point.at<double>(2,0);

        Mat left_image_point = kleft*left_norm_point;
        Mat right_image_point = kright*right_norm_point;

        Feature feat_left, feat_right;
        feat_left.Point.x = round(left_image_point.at<double>(0,0));
        feat_left.Point.y = round(left_image_point.at<double>(1,0));

        int max_feature_size = 10;
        feat_right.Point.x = round(right_image_point.at<double>(0,0) + (rand() %max_feature_size)-double(max_feature_size)/2);
        feat_right.Point.y = round(right_image_point.at<double>(1,0) + (rand() %max_feature_size)-double(max_feature_size)/2);

        if(feat_left.Point.x > 0 && feat_left.Point.x < image_w &&
        feat_left.Point.y > 0 && feat_left.Point.y < image_h &&
        feat_right.Point.x > 0 && feat_right.Point.x < image_w &&
        feat_right.Point.y > 0 && feat_right.Point.y < image_h)
        {
            double lval = left.at<double>(feat_left.Point.y, feat_left.Point.x);
            double rval = right.at<double>(feat_right.Point.y, feat_right.Point.x);

            if(lval == 0 && rval == 0)
            {
                left.at<double>(feat_left.Point.y, feat_left.Point.x) = 1;
                right.at<double>(feat_right.Point.y, feat_right.Point.x) = 1;

                Features1.push_back(feat_left);
                Features2.push_back(feat_right);

                cv::circle(images, feat_left.Point, 2, Scalar(0,255,0));
                circle(images, feat_right.Point, 2, Scalar(0,0,255));
            }

        }

    }

    /*imshow("images", images);
    waitKey(1);//*/

}

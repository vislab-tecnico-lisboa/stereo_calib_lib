#include "spherical_multiple_filter_stereo_calib_lib.h"

using namespace std;
using namespace cv;

spherical_multiple_filter_stereo_calib::spherical_multiple_filter_stereo_calib(spherical_multiple_filter_stereo_calib_params sscp_general_)
{
    //the standard image is 320x240
    sscp_general = sscp_general_;
    resize_factor = 320./sscp_general_.left_cam_resx;

    baseline = sscp_general_.baseline;

    translation_state_noise = 0.33;
    rotation_state_noise = 0.5;
    translation_transition_noise = 0.075; //0.075;
    rotation_transition_noise = 0.015; //0.015;
    translation_measurements_noise = 0.5; //0.5;
    rotation_measurements_noise = 0.1; //0.1;
    features_measurements_noise = 3/(resize_factor*resize_factor); //5;
    matching_threshold = 0.2; //0.3;
    max_number_of_features = 100; //200;
    min_number_of_features = 1;

    Kleft = Mat::eye(3,3,CV_64F);
    Kleft.at<double>(0,0) = sscp_general_.left_cam_fx;
    Kleft.at<double>(1,1) = sscp_general_.left_cam_fy;
    Kleft.at<double>(0,2) = sscp_general_.left_cam_cx;
    Kleft.at<double>(1,2) = sscp_general_.left_cam_cy;

    Kright = Mat::eye(3,3,CV_64F);
    Kright.at<double>(0,0) = sscp_general_.right_cam_fx;
    Kright.at<double>(1,1) = sscp_general_.right_cam_fy;
    Kright.at<double>(0,2) = sscp_general_.right_cam_cx;
    Kright.at<double>(1,2) = sscp_general_.right_cam_cy;

    number_fixed_state_params = 1;

    csc_ty.id_variable_to_estimate = 0;
    csc_tz.id_variable_to_estimate = 1;
    csc_rx.id_variable_to_estimate = 2;
    csc_ry.id_variable_to_estimate = 3;
    csc_rz.id_variable_to_estimate = 4;

    csc_ty.Num_Fix_State_Params = number_fixed_state_params;
    csc_tz.Num_Fix_State_Params = number_fixed_state_params;
    csc_rx.Num_Fix_State_Params = number_fixed_state_params;
    csc_ry.Num_Fix_State_Params = number_fixed_state_params;
    csc_rz.Num_Fix_State_Params = number_fixed_state_params;

    //set the filter parameters
    X_ty = Mat::zeros(number_fixed_state_params,1,CV_64F);
    X_tz = Mat::zeros(number_fixed_state_params,1,CV_64F);
    X_rx = Mat::zeros(number_fixed_state_params,1,CV_64F);
    X_ry = Mat::zeros(number_fixed_state_params,1,CV_64F);
    X_rz = Mat::zeros(number_fixed_state_params,1,CV_64F);

    P_ty = Mat::eye(number_fixed_state_params,number_fixed_state_params,CV_64F)*translation_state_noise*translation_state_noise;
    P_tz = Mat::eye(number_fixed_state_params,number_fixed_state_params,CV_64F)*translation_state_noise*translation_state_noise;
    P_rx = Mat::eye(number_fixed_state_params,number_fixed_state_params,CV_64F)*rotation_state_noise*rotation_state_noise;
    P_ry = Mat::eye(number_fixed_state_params,number_fixed_state_params,CV_64F)*rotation_state_noise*rotation_state_noise;
    P_rz = Mat::eye(number_fixed_state_params,number_fixed_state_params,CV_64F)*rotation_state_noise*rotation_state_noise;

    csc_ty.X_k = X_ty.clone();
    csc_tz.X_k = X_tz.clone();
    csc_rx.X_k = X_rx.clone();
    csc_ry.X_k = X_ry.clone();
    csc_rz.X_k = X_rz.clone();

    csc_ty.P_k = P_ty;
    csc_tz.P_k = P_tz;
    csc_rx.P_k = P_rx;
    csc_ry.P_k = P_ry;
    csc_rz.P_k = P_rz;

    csc_ty.U_k = Mat::zeros(1,1,CV_64F);
    csc_tz.U_k = Mat::zeros(1,1,CV_64F);
    csc_rx.U_k = Mat::zeros(1,1,CV_64F);
    csc_ry.U_k = Mat::zeros(1,1,CV_64F);
    csc_rz.U_k = Mat::zeros(1,1,CV_64F);

    csc_ty.Pn = Mat::zeros(1,1,CV_64F);
    csc_tz.Pn = Mat::zeros(1,1,CV_64F);
    csc_rx.Pn = Mat::zeros(1,1,CV_64F);
    csc_ry.Pn = Mat::zeros(1,1,CV_64F);
    csc_rz.Pn = Mat::zeros(1,1,CV_64F);

    double translation_transition_noise_ty = translation_transition_noise/5;
    double rotationtransition_noise_rx = rotation_transition_noise/2;
    double rotationtransition_noise_rz = rotation_transition_noise/2;
    csc_ty.Q = Mat::eye(number_fixed_state_params,number_fixed_state_params,CV_64F)*translation_transition_noise_ty*translation_transition_noise_ty;
    csc_tz.Q = Mat::eye(number_fixed_state_params,number_fixed_state_params,CV_64F)*translation_transition_noise*translation_transition_noise;
    csc_rx.Q = Mat::eye(number_fixed_state_params,number_fixed_state_params,CV_64F)*rotationtransition_noise_rx*rotationtransition_noise_rx;
    csc_ry.Q = Mat::eye(number_fixed_state_params,number_fixed_state_params,CV_64F)*rotation_transition_noise*rotation_transition_noise;
    csc_rz.Q = Mat::eye(number_fixed_state_params,number_fixed_state_params,CV_64F)*rotationtransition_noise_rz*rotationtransition_noise_rz;

    //set the camera intrinsic parameters
    csc_ty.LeftCalibMat = Kleft;
    csc_ty.RightCalibMat = Kright;

    csc_tz.LeftCalibMat = Kleft;
    csc_tz.RightCalibMat = Kright;

    csc_rx.LeftCalibMat = Kleft;
    csc_rx.RightCalibMat = Kright;

    csc_ry.LeftCalibMat = Kleft;
    csc_ry.RightCalibMat = Kright;

    csc_rz.LeftCalibMat = Kleft;
    csc_rz.RightCalibMat = Kright;

    //type of points used
    use_good_points = true;
    use_close_points = false;

    use_good_points_ty = false;
    use_good_points_tz = false;
    use_good_points_rx = false;
    use_good_points_ry = false;
    use_good_points_rz = false;

    first_images = true;
    min_image_diff = 5; //in pixel
}

void spherical_multiple_filter_stereo_calib::calibrate(const cv::Mat image_left, const cv::Mat image_right)
{
    bool use_measurements = true;
    if(!first_images)
    {
        Mat img_diff_left = abs(image_left-previous_left_img);
        Mat img_diff_right = abs(image_right-previous_right_img);

        double w = image_left.cols;
        double h = image_left.rows;

        double diff_left = cv::sum( img_diff_left )[0]/(w*h);
        double diff_right = cv::sum( img_diff_right )[0]/(w*h);

        if(diff_left > min_image_diff || diff_right > min_image_diff)
        {
            use_measurements = true;
            filters_converged = false;
        }
        else
            use_measurements = false;//*/
    }

    if(first_images)
    {
        previous_left_img = image_left.clone();
        previous_right_img = image_right.clone();
        first_images = false;
    }

    if(use_measurements || !filters_converged)
    {
        featuresSIFT get_features;
        std::vector<Feature> features_left;
        std::vector<Feature> features_right;

        get_features.Apply(image_left, image_right, features_left, features_right, max_number_of_features, matching_threshold);

        calibrate(features_left, features_right);
    }

    previous_left_img = image_left.clone();
    previous_right_img = image_right.clone();
}

void spherical_multiple_filter_stereo_calib::calibrate(std::vector<Feature> features_left, std::vector<Feature> features_right)
{
    int number_of_features = features_left.size();
    if(number_of_features >= min_number_of_features)
    {
        csc_ty.Flag_Cameras_Measurements = true;
        csc_tz.Flag_Cameras_Measurements = true;
        csc_rx.Flag_Cameras_Measurements = true;
        csc_ry.Flag_Cameras_Measurements = true;
        csc_rz.Flag_Cameras_Measurements = true;
    }
    else
    {
        csc_ty.Flag_Cameras_Measurements = false;
        csc_tz.Flag_Cameras_Measurements = false;
        csc_rx.Flag_Cameras_Measurements = false;
        csc_ry.Flag_Cameras_Measurements = false;
        csc_rz.Flag_Cameras_Measurements = false;
    }

    csc_ty.Filter_Prediction();
    csc_tz.Filter_Prediction();
    csc_rx.Filter_Prediction();
    csc_ry.Filter_Prediction();
    csc_rz.Filter_Prediction();

    double ty_pred = csc_ty.X_k.at<double>(0,0);
    double tz_pred = csc_tz.X_k.at<double>(0,0);
    double rx_pred = csc_rx.X_k.at<double>(0,0);
    double ry_pred = csc_ry.X_k.at<double>(0,0);
    double rz_pred = csc_rz.X_k.at<double>(0,0);

    //use estimation variance as measurement noises
    double ty_var = sqrt(csc_ty.P_k.at<double>(0,0));
    double tz_var = sqrt(csc_tz.P_k.at<double>(0,0));
    double t_var = max(ty_var, tz_var);

    double rx_var = sqrt(csc_rx.P_k.at<double>(0,0));
    double ry_var = sqrt(csc_ry.P_k.at<double>(0,0));
    double r_var_tmp = max(rx_var, ry_var);

    double rz_var = sqrt(csc_rz.P_k.at<double>(0,0));
    double r_var = max(r_var_tmp, rz_var);

    if(!filters_converged)
    {
        filters_converged = (csc_ty.filter_converged && csc_tz.filter_converged && csc_rx.filter_converged && csc_ry.filter_converged && csc_rz.filter_converged);
    }

    /*int w = 150;
    double var = 2.5;
    Mat calib_state = Mat::ones(2*w, 3*w, CV_8UC3);
    calib_state = Scalar(0,0,125);

    //for ty
    double n_ty = ty_var/(var*translation_transition_noise);
    double w_ty = 1-1/(pow(2,n_ty));

    Mat using_good_points_ty = calib_state(Range(0,w),Range(w,2*w));
    if(csc_ty.filter_converged)
        using_good_points_ty = Scalar(0,125,0);
    //using_good_points_ty = Scalar(0,125*(1-w_ty),125*w_ty);
    putText(calib_state, "Ty: "+doubleToString(round((1-w_ty)*100))+"%", Point(1.25*w,w/2), FONT_HERSHEY_TRIPLEX, .5, Scalar(255,255,255));
    //putText(calib_state, "Ty", Point(1.5*w,w/2), FONT_HERSHEY_TRIPLEX, .5, Scalar(255,255,255));

    //for tz
    double n_tz = tz_var/(var*translation_transition_noise);
    double w_tz = 1-1/(pow(2,n_tz));

    Mat using_good_points_tz = calib_state(Range(0,w),Range(2*w,3*w));
    if(csc_tz.filter_converged)
        using_good_points_tz = Scalar(0,125,0);
    //using_good_points_tz = Scalar(0,125*(1-w_tz),125*w_tz);
    putText(calib_state, "Tz: "+doubleToString(round((1-w_tz)*100))+"%", Point(2.25*w+25,w/2), FONT_HERSHEY_TRIPLEX, .5, Scalar(255,255,255));
    //putText(calib_state, "Tz", Point(2.5*w,w/2), FONT_HERSHEY_TRIPLEX, .5, Scalar(255,255,255));

    //for tx
    double w_tx = min(w_ty, w_tz);

    Mat using_good_points_tx = calib_state(Range(0,w),Range(0,w));
    if(csc_ty.filter_converged && csc_tz.filter_converged)
        using_good_points_tx = Scalar(0,125,0);
    //using_good_points_tx = Scalar(0,125*(1-w_tx),125*w_tx);
    putText(calib_state, "Tx: "+doubleToString(round((1-w_tx)*100))+"%", Point(0.25*w,w/2), FONT_HERSHEY_TRIPLEX, .5, Scalar(255,255,255));
    //putText(calib_state, "Tx", Point(w/2,w/2), FONT_HERSHEY_TRIPLEX, .5, Scalar(255,255,255));

    //for rx
    double n_rx = rx_var/(var*rotation_transition_noise);
    double w_rx = 1-1/(pow(2,n_rx));

    Mat using_good_points_rx = calib_state(Range(w,2*w),Range(0,w));
    if(csc_rx.filter_converged)
        using_good_points_rx = Scalar(0,125,0);
    //using_good_points_rx = Scalar(0,125*(1-w_rx),125*w_rx);
    putText(calib_state, "Rx: "+doubleToString(round((1-w_rx)*100))+"%", Point(0.25*w,1.5*w), FONT_HERSHEY_TRIPLEX, .5, Scalar(255,255,255));
    //putText(calib_state, "Rx", Point(w/2,1.5*w), FONT_HERSHEY_TRIPLEX, .5, Scalar(255,255,255));

     //for ry
    double n_ry = ry_var/(var*rotation_transition_noise);
    double w_ry = 1-1/(pow(2,n_ry));

    Mat using_good_points_ry = calib_state(Range(w,2*w),Range(w,2*w));
    if(csc_ry.filter_converged)
        using_good_points_ry = Scalar(0,125,0);
    //uusing_good_points_ry = Scalar(0,125*(1-w_ry),125*w_ry);
    putText(calib_state, "Ry: "+doubleToString(round((1-w_ry)*100))+"%", Point(1.25*w,1.5*w), FONT_HERSHEY_TRIPLEX, .5, Scalar(255,255,255));
    //putText(calib_state, "Ry", Point(1.5*w,1.5*w), FONT_HERSHEY_TRIPLEX, .5, Scalar(255,255,255));

    //for rz
    double n_rz = rz_var/(var*rotation_transition_noise);
    double w_rz = 1-1/(pow(2,n_rz));

    Mat using_good_points_rz = calib_state(Range(w,2*w),Range(2*w,3*w));
    if(csc_rz.filter_converged)
        using_good_points_rz = Scalar(0,125,0);
    //uusing_good_points_rz = Scalar(0,125*(1-w_rz),125*w_rz);
    putText(calib_state, "Rz: "+doubleToString(round((1-w_rz)*100))+"%", Point(2.25*w,1.5*w), FONT_HERSHEY_TRIPLEX, .5, Scalar(255,255,255));
    //putText(calib_state, "Rz", Point(2.5*w,1.5*w), FONT_HERSHEY_TRIPLEX, .5, Scalar(255,255,255));

    imshow("calib_state", calib_state);//*/


    if(csc_ty.Flag_Cameras_Measurements)
    {
        filterMeasurementsStruct filter_measurements_struct = defineFiltersMeasurementsVector(ty_pred, tz_pred, rx_pred, ry_pred, rz_pred, features_left, features_right, number_of_features,
                                                                                          t_var, r_var, features_measurements_noise);//*/

        /*filterMeasurementsStruct filter_measurements_struct = defineFiltersMeasurementsVector(ty_pred, tz_pred, rx_pred, ry_pred, rz_pred, features_left, features_right, number_of_features,
                                                                                          ty_var, tz_var, rx_var, ry_var, rz_var, features_measurements_noise);//*/

        csc_ty.NumPoints = (filter_measurements_struct.Z_ty.rows-4)/4;
        csc_tz.NumPoints = (filter_measurements_struct.Z_tz.rows-4)/4;
        csc_rx.NumPoints = (filter_measurements_struct.Z_rx.rows-4)/4;
        csc_ry.NumPoints = (filter_measurements_struct.Z_ry.rows-4)/4;
        csc_rz.NumPoints = (filter_measurements_struct.Z_rz.rows-4)/4;

        //cout << double(csc_ty.NumPoints)/double(number_of_features)*100 << "% - " << double(csc_tz.NumPoints)/double(number_of_features)*100  << "% - " << double(csc_rx.NumPoints)/double(number_of_features)*100 << "% - " << double(csc_ry.NumPoints)/double(number_of_features)*100 << "% - " << double(csc_rz.NumPoints)/double(number_of_features)*100 << "%" << endl;

        if(csc_ty.NumPoints < min_number_of_features)
        {
            csc_ty.Flag_Cameras_Measurements = false;
        }

        if(csc_tz.NumPoints < min_number_of_features)
        {
            csc_tz.Flag_Cameras_Measurements = false;
        }

        if(csc_rx.NumPoints < min_number_of_features)
        {
            csc_rx.Flag_Cameras_Measurements = false;
        }

        if(csc_ry.NumPoints < min_number_of_features)
        {
            csc_ry.Flag_Cameras_Measurements = false;
        }

        if(csc_rz.NumPoints < min_number_of_features)
        {
            csc_rz.Flag_Cameras_Measurements = false;
        }

        csc_ty.Rn_explicit = Mat();
        csc_ty.Rn_implicit = filter_measurements_struct.R_ty.clone();

        csc_tz.Rn_explicit = Mat();
        csc_tz.Rn_implicit = filter_measurements_struct.R_tz.clone();

        csc_rx.Rn_explicit = Mat();
        csc_rx.Rn_implicit = filter_measurements_struct.R_rx.clone();

        csc_ry.Rn_explicit = Mat();
        csc_ry.Rn_implicit = filter_measurements_struct.R_ry.clone();

        csc_rz.Rn_explicit = Mat();
        csc_rz.Rn_implicit = filter_measurements_struct.R_rz.clone();

        if(csc_ty.Flag_Cameras_Measurements)
            csc_ty.Filter_Update(Mat(), filter_measurements_struct.Z_ty.clone());

        if(csc_tz.Flag_Cameras_Measurements)
            csc_tz.Filter_Update(Mat(), filter_measurements_struct.Z_tz.clone());

        if(csc_rx.Flag_Cameras_Measurements)
            csc_rx.Filter_Update(Mat(), filter_measurements_struct.Z_rx.clone());

        if(csc_ry.Flag_Cameras_Measurements)
            csc_ry.Filter_Update(Mat(), filter_measurements_struct.Z_ry.clone());

        if(csc_rz.Flag_Cameras_Measurements)
            csc_rz.Filter_Update(Mat(), filter_measurements_struct.Z_rz.clone());

    }
}

spherical_multiple_filter_stereo_calib_data spherical_multiple_filter_stereo_calib::get_calibrated_transformations()
{
    spherical_multiple_filter_stereo_calib_data scd;

    scd.ty =  csc_ty.X_k.clone().at<double>(0,0);
    scd.tz =  csc_tz.X_k.clone().at<double>(0,0);
    scd.rx = csc_rx.X_k.clone().at<double>(0,0);
    scd.ry =  csc_ry.X_k.clone().at<double>(0,0);
    scd.rz = csc_rz.X_k.clone().at<double>(0,0);

    Mat rot_LeftToRightKplus1 = Mat::zeros(3,1, CV_64F);
	rot_LeftToRightKplus1.at<double>(0,0) = scd.rx;
	rot_LeftToRightKplus1.at<double>(1,0) = scd.ry;
	rot_LeftToRightKplus1.at<double>(2,0) = scd.rz;

	Mat R_LeftToRightKplus1;
	Rodrigues(rot_LeftToRightKplus1, R_LeftToRightKplus1);

	Mat t_LeftToRightKplus1 = Mat::zeros(3,1,CV_64F);

	t_LeftToRightKplus1.at<double>(0,0) = -sqrt(1 - scd.ty*scd.ty - scd.tz*scd.tz)*baseline;
	t_LeftToRightKplus1.at<double>(1,0) = scd.ty*baseline;
	t_LeftToRightKplus1.at<double>(2,0) = scd.tz*baseline;

	//t_LeftToRightKplus1 = t_LeftToRightKplus1.clone()/norm(t_LeftToRightKplus1.clone())*baseline;

	Mat T_LeftToRightKplus1 = Mat::eye(4,4,CV_64F);
	for (int r=0; r<3; r++)
	{
        T_LeftToRightKplus1.at<double>(r,3) = t_LeftToRightKplus1.at<double>(r,0);

        for (int c=0; c<3; c++)
        {
            T_LeftToRightKplus1.at<double>(r,c) = R_LeftToRightKplus1.at<double>(r,c);
        }
	}

	scd.R_left_cam_to_right_cam = R_LeftToRightKplus1;
	scd.rot_left_cam_to_right_cam = rot_LeftToRightKplus1;
	scd.t_left_cam_to_right_cam = t_LeftToRightKplus1;
	scd.transformation_left_cam_to_right_cam = T_LeftToRightKplus1;

    return scd;
}

spherical_multiple_filter_stereo_disparity_data spherical_multiple_filter_stereo_calib::get_disparity_map(cv::Mat left_image, cv::Mat right_image)
{
    spherical_multiple_filter_stereo_disparity_data sdd;
    spherical_multiple_filter_stereo_calib_data scd = get_calibrated_transformations();

    cv::Mat R1(3,3,CV_64F), R2(3,3,CV_64F);
    Mat stereo_left_calib_mat, stereo_right_calib_mat;
    Mat Q;

    Mat stereo_rectification_map1_left, stereo_rectification_map2_left, stereo_rectification_map1_right, stereo_rectification_map2_right;
    Mat rectified_left_image, rectified_right_image;

	cv::stereoRectify(Kleft, Mat::zeros(5,1,CV_64F), Kright, Mat::zeros(5,1,CV_64F), Size(left_image.cols, left_image.rows), scd.R_left_cam_to_right_cam, scd.t_left_cam_to_right_cam, R1, R2,
                   stereo_left_calib_mat, stereo_right_calib_mat, Q, CV_CALIB_ZERO_DISPARITY,0, Size(left_image.cols, left_image.rows));

	cv::initUndistortRectifyMap(Kleft, Mat::zeros(5,1,CV_64F), R1, stereo_left_calib_mat, Size(left_image.cols, left_image.rows), CV_16SC2, stereo_rectification_map1_left, stereo_rectification_map2_left);

	cv::initUndistortRectifyMap(Kright, Mat::zeros(5,1,CV_64F), R2, stereo_right_calib_mat, Size(right_image.cols, right_image.rows), CV_16SC2, stereo_rectification_map1_right, stereo_rectification_map2_right);

	cv::remap(left_image, rectified_left_image, stereo_rectification_map1_left, stereo_rectification_map2_left, cv::INTER_LINEAR);
	cv::remap(right_image, rectified_right_image, stereo_rectification_map1_right, stereo_rectification_map2_right, cv::INTER_LINEAR);

	//Disparity Method
    Ptr<StereoSGBM> SBM;

    SBM->create(0,
               80,
               3,
                768,
                1536,
                0,
                63,
                15,
                50,
                16
                );


	Mat Disparity;
    SBM->compute(rectified_left_image, rectified_right_image, Disparity);
    Disparity.convertTo(sdd.disparity_values, CV_64F, -1/16.);

    //sdd.point_cloud_xyz = Mat::zeros(left_image.rows, left_image.cols, CV_64FC3);
    sdd.disparity_values = Mat::zeros(Disparity.rows,Disparity.cols, CV_64F);

	/*for (int r=0; r<Disparity.rows; r++){
		for (int c=0; c<Disparity.cols; c++){

            sdd.disparity_values.at<double>(r,c) = -double(Disparity.at<short>(r,c))/16;
            //disparity_values.at<double>(r,c) = -double(Disparity.at<short>(r,c))/16;

            Mat imagePoint = Mat::ones(4,1,CV_64F);
            imagePoint.at<double>(0,0) = c;
            imagePoint.at<double>(1,0) = r;
            imagePoint.at<double>(2,0) = -sdd.disparity_values.at<double>(r,c);

            Mat triPoint = Q*imagePoint;
            triPoint = triPoint/triPoint.at<double>(3,0);

	    //triPoint = scd.transformation_left_cam_to_baseline_center*triPoint.clone();

            sdd.point_cloud_xyz.at<Vec3d>(r,c)[0] = triPoint.at<double>(0,0);
            sdd.point_cloud_xyz.at<Vec3d>(r,c)[1] = triPoint.at<double>(1,0);
            sdd.point_cloud_xyz.at<Vec3d>(r,c)[2] = triPoint.at<double>(2,0);

		}
	}//*/
    sdd.point_cloud_rgb = rectified_left_image;
    Disparity.convertTo(sdd.disparity_image, CV_8U, 255/(SBM->getNumDisparities()*16.));

    return sdd;
}


//functions
filterMeasurementsStruct spherical_multiple_filter_stereo_calib::defineFiltersMeasurementsVector(double ty_pred, double tz_pred, double rx_pred, double ry_pred, double rz_pred,
                                                                 std::vector<Feature> features_left, std::vector<Feature> features_right, int number_of_features, double ty_uncertainty, double tz_uncertainty,
                                                                 double rx_uncertainty, double ry_uncertainty, double rz_uncertainty, double features_noise)
{
    filterMeasurementsStruct filter_measurements_struct;

    double translation_noise = max(ty_uncertainty, tz_uncertainty);
    double rotation_noise_tmp = max(rx_uncertainty, ry_uncertainty);
    double rotation_noise = max(rotation_noise_tmp, rz_uncertainty);

    Mat R_ty_vec, R_tz_vec, R_rx_vec, R_ry_vec, R_rz_vec;

    spherical_multiple_filter_stereo_calib_data cmfscd = get_calibrated_transformations();

    //for the translations and rotations
    filter_measurements_struct.Z_ty.push_back(tz_pred);
    filter_measurements_struct.Z_ty.push_back(rx_pred);
    filter_measurements_struct.Z_ty.push_back(ry_pred);
    filter_measurements_struct.Z_ty.push_back(rz_pred);

    R_ty_vec.push_back(translation_noise);
    R_ty_vec.push_back(rotation_noise);
    R_ty_vec.push_back(rotation_noise);
    R_ty_vec.push_back(rotation_noise);

    filter_measurements_struct.Z_tz.push_back(ty_pred);
    filter_measurements_struct.Z_tz.push_back(rx_pred);
    filter_measurements_struct.Z_tz.push_back(ry_pred);
    filter_measurements_struct.Z_tz.push_back(rz_pred);

    R_tz_vec.push_back(translation_noise);
    R_tz_vec.push_back(rotation_noise);
    R_tz_vec.push_back(rotation_noise);
    R_tz_vec.push_back(rotation_noise);

    filter_measurements_struct.Z_rx.push_back(ty_pred);
    filter_measurements_struct.Z_rx.push_back(tz_pred);
    filter_measurements_struct.Z_rx.push_back(ry_pred);
    filter_measurements_struct.Z_rx.push_back(rz_pred);

    R_rx_vec.push_back(translation_noise);
    R_rx_vec.push_back(translation_noise);
    R_rx_vec.push_back(rotation_noise);
    R_rx_vec.push_back(rotation_noise);

    filter_measurements_struct.Z_ry.push_back(ty_pred);
    filter_measurements_struct.Z_ry.push_back(tz_pred);
    filter_measurements_struct.Z_ry.push_back(rx_pred);
    filter_measurements_struct.Z_ry.push_back(rz_pred);

    R_ry_vec.push_back(translation_noise);
    R_ry_vec.push_back(translation_noise);
    R_ry_vec.push_back(rotation_noise);
    R_ry_vec.push_back(rotation_noise);

    filter_measurements_struct.Z_rz.push_back(ty_pred);
    filter_measurements_struct.Z_rz.push_back(tz_pred);
    filter_measurements_struct.Z_rz.push_back(rx_pred);
    filter_measurements_struct.Z_rz.push_back(ry_pred);

    R_rz_vec.push_back(translation_noise);
    R_rz_vec.push_back(translation_noise);
    R_rz_vec.push_back(rotation_noise);
    R_rz_vec.push_back(rotation_noise);

    Mat ir = Mat::zeros(2*sscp_general.left_cam_resy, 3*sscp_general.left_cam_resx, CV_8UC3);

    bool show_points = true;
    //bool use_good_points = true;

    double min_displacement = 2./resize_factor;
    double max_t = 0.25;
    double max_r = 0.05;
    int w = 100;
    Mat using_good_points = Mat::zeros(2*w,3*w,CV_8UC3);
    using_good_points = Scalar(0,0,125);

    double var = 2.;
    //if(ty_uncertainty < var*translation_transition_noise)
        use_good_points_ty = true;

    //if(tz_uncertainty < var*translation_transition_noise)
        use_good_points_tz = true;

    //if(rx_uncertainty < var*rotation_transition_noise)
        use_good_points_rx = true;

    //if(ry_uncertainty < var*rotation_transition_noise)
        use_good_points_ry = true;

    //if(rz_uncertainty < var*rotation_transition_noise)
        use_good_points_rz = true;

    if(ty_uncertainty < var*translation_transition_noise && tz_uncertainty < var*translation_transition_noise)
    {
        Mat using_good_points_tx = using_good_points(Range(0,w),Range(0,w));
        using_good_points_tx = Scalar(0,125,0);
    }

    if(ty_uncertainty < var*translation_transition_noise)
    {
        Mat using_good_points_ty = using_good_points(Range(0,w),Range(w,2*w));
        using_good_points_ty = Scalar(0,125,0);
    }

    if(tz_uncertainty < var*translation_transition_noise)
    {
        Mat using_good_points_tz = using_good_points(Range(0,w),Range(2*w,3*w));
        using_good_points_tz = Scalar(0,125,0);
    }

    if(rx_uncertainty < var*translation_transition_noise)
    {
        Mat using_good_points_rx = using_good_points(Range(w,2*w),Range(0,w));
        using_good_points_rx = Scalar(0,125,0);
    }

    if(ry_uncertainty < var*translation_transition_noise)
    {
        Mat using_good_points_ry = using_good_points(Range(w,2*w),Range(w,2*w));
        using_good_points_ry = Scalar(0,125,0);
    }

    if(rz_uncertainty < var*translation_transition_noise)
    {
        Mat using_good_points_rz = using_good_points(Range(w,2*w),Range(2*w,3*w));
        using_good_points_rz = Scalar(0,125,0);
    }

    imshow("using_good_points", using_good_points);

    for(int j=0; j<number_of_features; j++){

        bool ty_condition = true;
        bool tz_condition = true;
        bool rx_condition = true;
        bool ry_condition = true;
        bool rz_condition = true;

        if(use_good_points_ty || use_good_points_tz || use_good_points_rx || use_good_points_ry || use_good_points_rz)
        {
            cv::Point3d wp = ImageToWorld(features_left[j].Point, features_right[j].Point, Mat::eye(4,4,CV_64F), cmfscd.transformation_left_cam_to_right_cam, Kleft, Kright);
            double fx = Kright.at<double>(0,0);
            double fy = Kright.at<double>(1,1);

            if(use_good_points_ty)
            {
                double weight_ty_l = PointWeight_ty(wp.x, wp.y, wp.z, fx, fy, baseline, -max_t, min_displacement);
                double weight_ty_u = PointWeight_ty(wp.x, wp.y, wp.z, fx, fy, baseline, max_t, min_displacement);
                ty_condition = (weight_ty_l == 1 || weight_ty_u == 1);
            }

            if(use_good_points_tz)
            {
                double weight_tz_l = PointWeight_tz(wp.x, wp.y, wp.z, fx, fy, baseline, -max_t, min_displacement);
                double weight_tz_u = PointWeight_tz(wp.x, wp.y, wp.z, fx, fy, baseline, max_t, min_displacement);
                tz_condition = (weight_tz_l == 1 || weight_tz_u == 1);
            }

            if(use_good_points_rx)
            {
                double weight_rx_l = PointWeight_rx(wp.x, wp.y, wp.z, fx, fy, baseline, -max_r, min_displacement);
                double weight_rx_u = PointWeight_rx(wp.x, wp.y, wp.z, fx, fy, baseline, max_r, min_displacement);
                rx_condition = (weight_rx_l == 1 || weight_rx_u == 1);
            }

            if(use_good_points_ry)
            {
                double weight_ry_l = PointWeight_ry(wp.x, wp.y, wp.z, fx, fy, baseline, -max_r, min_displacement);
                double weight_ry_u = PointWeight_ry(wp.x, wp.y, wp.z, fx, fy, baseline, max_r, min_displacement);
                ry_condition = (weight_ry_l == 1 || weight_ry_u == 1);
            }

            if(use_good_points_rz)
            {
                double weight_rz_l = PointWeight_rz(wp.x, wp.y, wp.z, fx, fy, baseline, -max_r, min_displacement);
                double weight_rz_u = PointWeight_rz(wp.x, wp.y, wp.z, fx, fy, baseline, max_r, min_displacement);
                rz_condition = (weight_rz_l == 1 || weight_rz_u == 1);
            }

        }

        if(show_points)
        {
            if(ty_condition)
            {
                ir.at<Vec3b>(features_right[j].Point.y, sscp_general.left_cam_resx + features_right[j].Point.x)[1] = 255;
            }
            if(tz_condition)
            {
                ir.at<Vec3b>(features_right[j].Point.y, 2*sscp_general.left_cam_resx + features_right[j].Point.x)[2] = 255;
            }
            if(rx_condition)
            {
                ir.at<Vec3b>(sscp_general.left_cam_resy + features_right[j].Point.y, features_right[j].Point.x)[0] = 255;
            }

            if(ry_condition)
            {
                ir.at<Vec3b>(sscp_general.left_cam_resy + features_right[j].Point.y, sscp_general.left_cam_resx + features_right[j].Point.x)[1] = 255;
            }

            if(rz_condition)
            {
                ir.at<Vec3b>(sscp_general.left_cam_resy + features_right[j].Point.y, 2*sscp_general.left_cam_resx + features_right[j].Point.x)[2] = 255;
            }
        }//*/

        if(ty_condition)
        {
            filter_measurements_struct.Z_ty.push_back(double(features_left[j].Point.x));
            filter_measurements_struct.Z_ty.push_back(double(features_left[j].Point.y));
            filter_measurements_struct.Z_ty.push_back(double(features_right[j].Point.x));
            filter_measurements_struct.Z_ty.push_back(double(features_right[j].Point.y));

            R_ty_vec.push_back(features_noise);
            R_ty_vec.push_back(features_noise);
            R_ty_vec.push_back(features_noise);
            R_ty_vec.push_back(features_noise);
        }

        if(tz_condition)
        {
            filter_measurements_struct.Z_tz.push_back(double(features_left[j].Point.x));
            filter_measurements_struct.Z_tz.push_back(double(features_left[j].Point.y));
            filter_measurements_struct.Z_tz.push_back(double(features_right[j].Point.x));
            filter_measurements_struct.Z_tz.push_back(double(features_right[j].Point.y));

            R_tz_vec.push_back(features_noise);
            R_tz_vec.push_back(features_noise);
            R_tz_vec.push_back(features_noise);
            R_tz_vec.push_back(features_noise);
        }

        if(rx_condition)
        {
            filter_measurements_struct.Z_rx.push_back(double(features_left[j].Point.x));
            filter_measurements_struct.Z_rx.push_back(double(features_left[j].Point.y));
            filter_measurements_struct.Z_rx.push_back(double(features_right[j].Point.x));
            filter_measurements_struct.Z_rx.push_back(double(features_right[j].Point.y));

            R_rx_vec.push_back(features_noise);
            R_rx_vec.push_back(features_noise);
            R_rx_vec.push_back(features_noise);
            R_rx_vec.push_back(features_noise);
        }

        if(ry_condition)
        {
            filter_measurements_struct.Z_ry.push_back(double(features_left[j].Point.x));
            filter_measurements_struct.Z_ry.push_back(double(features_left[j].Point.y));
            filter_measurements_struct.Z_ry.push_back(double(features_right[j].Point.x));
            filter_measurements_struct.Z_ry.push_back(double(features_right[j].Point.y));

            R_ry_vec.push_back(features_noise);
            R_ry_vec.push_back(features_noise);
            R_ry_vec.push_back(features_noise);
            R_ry_vec.push_back(features_noise);

        }

        if(rz_condition)
        {

            filter_measurements_struct.Z_rz.push_back(double(features_left[j].Point.x));
            filter_measurements_struct.Z_rz.push_back(double(features_left[j].Point.y));
            filter_measurements_struct.Z_rz.push_back(double(features_right[j].Point.x));
            filter_measurements_struct.Z_rz.push_back(double(features_right[j].Point.y));

            R_rz_vec.push_back(features_noise);
            R_rz_vec.push_back(features_noise);
            R_rz_vec.push_back(features_noise);
            R_rz_vec.push_back(features_noise);
        }
    }

    filter_measurements_struct.R_ty = Mat::eye(R_ty_vec.rows,R_ty_vec.rows,CV_64F);
    for(int j=0; j<R_ty_vec.rows; j++)
        filter_measurements_struct.R_ty.at<double>(j,j) = R_ty_vec.at<double>(j,0)*R_ty_vec.at<double>(j,0);

    filter_measurements_struct.R_tz = Mat::eye(R_tz_vec.rows,R_tz_vec.rows,CV_64F);
    for(int j=0; j<R_tz_vec.rows; j++)
        filter_measurements_struct.R_tz.at<double>(j,j) = R_tz_vec.at<double>(j,0)*R_tz_vec.at<double>(j,0);

    filter_measurements_struct.R_rx = Mat::eye(R_rx_vec.rows,R_rx_vec.rows,CV_64F);
    for(int j=0; j<R_rx_vec.rows; j++)
        filter_measurements_struct.R_rx.at<double>(j,j) = R_rx_vec.at<double>(j,0)*R_rx_vec.at<double>(j,0);

    filter_measurements_struct.R_ry = Mat::eye(R_ry_vec.rows,R_ry_vec.rows,CV_64F);
    for(int j=0; j<R_ry_vec.rows; j++)
        filter_measurements_struct.R_ry.at<double>(j,j) = R_ry_vec.at<double>(j,0)*R_ry_vec.at<double>(j,0);

    filter_measurements_struct.R_rz = Mat::eye(R_rz_vec.rows,R_rz_vec.rows,CV_64F);
    for(int j=0; j<R_rz_vec.rows; j++)
        filter_measurements_struct.R_rz.at<double>(j,j) = R_rz_vec.at<double>(j,0)*R_rz_vec.at<double>(j,0);

    //cout << endl;

    if(show_points)
    {
        imshow("ir", ir);
        waitKey(10);
    }

    return filter_measurements_struct;
}

//functions
filterMeasurementsStruct spherical_multiple_filter_stereo_calib::defineFiltersMeasurementsVector(double ty_pred, double tz_pred, double rx_pred, double ry_pred, double rz_pred,
                                                                 std::vector<Feature> features_left, std::vector<Feature> features_right, int number_of_features, double translation_noise,
                                                                 double rotation_noise, double features_noise)
{
    filterMeasurementsStruct filter_measurements_struct;

    Mat R_ty_vec, R_tz_vec, R_rx_vec, R_ry_vec, R_rz_vec;

    spherical_multiple_filter_stereo_calib_data cmfscd = get_calibrated_transformations();

    //for the translations and rotations
    filter_measurements_struct.Z_ty.push_back(tz_pred);
    filter_measurements_struct.Z_ty.push_back(rx_pred);
    filter_measurements_struct.Z_ty.push_back(ry_pred);
    filter_measurements_struct.Z_ty.push_back(rz_pred);

    R_ty_vec.push_back(translation_noise);
    R_ty_vec.push_back(rotation_noise);
    R_ty_vec.push_back(rotation_noise);
    R_ty_vec.push_back(rotation_noise);

    filter_measurements_struct.Z_tz.push_back(ty_pred);
    filter_measurements_struct.Z_tz.push_back(rx_pred);
    filter_measurements_struct.Z_tz.push_back(ry_pred);
    filter_measurements_struct.Z_tz.push_back(rz_pred);

    R_tz_vec.push_back(translation_noise);
    R_tz_vec.push_back(rotation_noise);
    R_tz_vec.push_back(rotation_noise);
    R_tz_vec.push_back(rotation_noise);

    filter_measurements_struct.Z_rx.push_back(ty_pred);
    filter_measurements_struct.Z_rx.push_back(tz_pred);
    filter_measurements_struct.Z_rx.push_back(ry_pred);
    filter_measurements_struct.Z_rx.push_back(rz_pred);

    R_rx_vec.push_back(translation_noise);
    R_rx_vec.push_back(translation_noise);
    R_rx_vec.push_back(rotation_noise);
    R_rx_vec.push_back(rotation_noise);

    filter_measurements_struct.Z_ry.push_back(ty_pred);
    filter_measurements_struct.Z_ry.push_back(tz_pred);
    filter_measurements_struct.Z_ry.push_back(rx_pred);
    filter_measurements_struct.Z_ry.push_back(rz_pred);

    R_ry_vec.push_back(translation_noise);
    R_ry_vec.push_back(translation_noise);
    R_ry_vec.push_back(rotation_noise);
    R_ry_vec.push_back(rotation_noise);

    filter_measurements_struct.Z_rz.push_back(ty_pred);
    filter_measurements_struct.Z_rz.push_back(tz_pred);
    filter_measurements_struct.Z_rz.push_back(rx_pred);
    filter_measurements_struct.Z_rz.push_back(ry_pred);

    R_rz_vec.push_back(translation_noise);
    R_rz_vec.push_back(translation_noise);
    R_rz_vec.push_back(rotation_noise);
    R_rz_vec.push_back(rotation_noise);

    Mat ir = Mat::zeros(2*sscp_general.left_cam_resy, 3*sscp_general.left_cam_resx, CV_8UC3);

    bool show_points = false;
    //bool use_good_points = true;

    double threshold_all = 0.0;
    double threshold_good = 0.7; //0.8;
    double threshold_bad = 0.6; //0.6;

    for(int j=0; j<number_of_features; j++){

        bool ty_condition;
        bool tz_condition;

        double weight_ty = PointWeight_ty(features_left[j].Point, features_right[j].Point, Kleft, Kright, cmfscd.transformation_left_cam_to_right_cam);
        double weight_tz = weight_ty;
        double weight_rx = PointWeight_rx(features_right[j].Point, Kright, sscp_general.left_cam_resy);
        double weight_ry = PointWeight_ry(features_right[j].Point, Kright, sscp_general.left_cam_resx);
        double weight_rz = PointWeight_rz(features_right[j].Point, Kright, sscp_general.left_cam_resx, sscp_general.left_cam_resy);

        bool rx_condition;
        bool ry_condition;
        bool rz_condition;

        if(use_good_points)
        {
            ty_condition = (weight_ty==1);
            tz_condition = (weight_tz==1);
            rx_condition = (weight_rx > threshold_good);
            ry_condition = (weight_ry > threshold_good);
            rz_condition = (weight_rz > threshold_good);
        }
        else
        {
            ty_condition = true;
            tz_condition = true;
            rx_condition = true; //(weight_rx < threshold_bad);
            ry_condition = true; //(weight_ry < threshold_bad);
            rz_condition = true; //(weight_rz < threshold_bad);
        }

        if(show_points)
        {
            if(ty_condition)
            {
                ir.at<Vec3b>(features_right[j].Point.y, sscp_general.left_cam_resx + features_right[j].Point.x)[1] = 255;
            }
            if(tz_condition)
            {
                ir.at<Vec3b>(features_right[j].Point.y, 2*sscp_general.left_cam_resx + features_right[j].Point.x)[2] = 255;
            }
            if(rx_condition)
            {
                ir.at<Vec3b>(sscp_general.left_cam_resy + features_right[j].Point.y, features_right[j].Point.x)[0] = 255;
            }

            if(ry_condition)
            {
                ir.at<Vec3b>(sscp_general.left_cam_resy + features_right[j].Point.y, sscp_general.left_cam_resx + features_right[j].Point.x)[1] = 255;
            }

            if(rz_condition)
            {
                ir.at<Vec3b>(sscp_general.left_cam_resy + features_right[j].Point.y, 2*sscp_general.left_cam_resx + features_right[j].Point.x)[2] = 255;
            }
        }

        if(ty_condition)
        {
            filter_measurements_struct.Z_ty.push_back(double(features_left[j].Point.x));
            filter_measurements_struct.Z_ty.push_back(double(features_left[j].Point.y));
            filter_measurements_struct.Z_ty.push_back(double(features_right[j].Point.x));
            filter_measurements_struct.Z_ty.push_back(double(features_right[j].Point.y));

            R_ty_vec.push_back(features_noise);
            R_ty_vec.push_back(features_noise);
            R_ty_vec.push_back(features_noise);
            R_ty_vec.push_back(features_noise);
        }

        if(tz_condition)
        {
            filter_measurements_struct.Z_tz.push_back(double(features_left[j].Point.x));
            filter_measurements_struct.Z_tz.push_back(double(features_left[j].Point.y));
            filter_measurements_struct.Z_tz.push_back(double(features_right[j].Point.x));
            filter_measurements_struct.Z_tz.push_back(double(features_right[j].Point.y));

            R_tz_vec.push_back(features_noise);
            R_tz_vec.push_back(features_noise);
            R_tz_vec.push_back(features_noise);
            R_tz_vec.push_back(features_noise);
        }

        if(rx_condition)
        {
            filter_measurements_struct.Z_rx.push_back(double(features_left[j].Point.x));
            filter_measurements_struct.Z_rx.push_back(double(features_left[j].Point.y));
            filter_measurements_struct.Z_rx.push_back(double(features_right[j].Point.x));
            filter_measurements_struct.Z_rx.push_back(double(features_right[j].Point.y));

            R_rx_vec.push_back(features_noise);
            R_rx_vec.push_back(features_noise);
            R_rx_vec.push_back(features_noise);
            R_rx_vec.push_back(features_noise);
        }

        if(ry_condition)
        {
            filter_measurements_struct.Z_ry.push_back(double(features_left[j].Point.x));
            filter_measurements_struct.Z_ry.push_back(double(features_left[j].Point.y));
            filter_measurements_struct.Z_ry.push_back(double(features_right[j].Point.x));
            filter_measurements_struct.Z_ry.push_back(double(features_right[j].Point.y));

            R_ry_vec.push_back(features_noise);
            R_ry_vec.push_back(features_noise);
            R_ry_vec.push_back(features_noise);
            R_ry_vec.push_back(features_noise);

        }

        if(rz_condition)
        {

            filter_measurements_struct.Z_rz.push_back(double(features_left[j].Point.x));
            filter_measurements_struct.Z_rz.push_back(double(features_left[j].Point.y));
            filter_measurements_struct.Z_rz.push_back(double(features_right[j].Point.x));
            filter_measurements_struct.Z_rz.push_back(double(features_right[j].Point.y));

            R_rz_vec.push_back(features_noise);
            R_rz_vec.push_back(features_noise);
            R_rz_vec.push_back(features_noise);
            R_rz_vec.push_back(features_noise);
        }
    }

    filter_measurements_struct.R_ty = Mat::eye(R_ty_vec.rows,R_ty_vec.rows,CV_64F);
    for(int j=0; j<R_ty_vec.rows; j++)
        filter_measurements_struct.R_ty.at<double>(j,j) = R_ty_vec.at<double>(j,0)*R_ty_vec.at<double>(j,0);

    filter_measurements_struct.R_tz = Mat::eye(R_tz_vec.rows,R_tz_vec.rows,CV_64F);
    for(int j=0; j<R_tz_vec.rows; j++)
        filter_measurements_struct.R_tz.at<double>(j,j) = R_tz_vec.at<double>(j,0)*R_tz_vec.at<double>(j,0);

    filter_measurements_struct.R_rx = Mat::eye(R_rx_vec.rows,R_rx_vec.rows,CV_64F);
    for(int j=0; j<R_rx_vec.rows; j++)
        filter_measurements_struct.R_rx.at<double>(j,j) = R_rx_vec.at<double>(j,0)*R_rx_vec.at<double>(j,0);

    filter_measurements_struct.R_ry = Mat::eye(R_ry_vec.rows,R_ry_vec.rows,CV_64F);
    for(int j=0; j<R_ry_vec.rows; j++)
        filter_measurements_struct.R_ry.at<double>(j,j) = R_ry_vec.at<double>(j,0)*R_ry_vec.at<double>(j,0);

    filter_measurements_struct.R_rz = Mat::eye(R_rz_vec.rows,R_rz_vec.rows,CV_64F);
    for(int j=0; j<R_rz_vec.rows; j++)
        filter_measurements_struct.R_rz.at<double>(j,j) = R_rz_vec.at<double>(j,0)*R_rz_vec.at<double>(j,0);

    //cout << endl;

    if(show_points)
    {
        cv::imshow("ir", ir);
        cv::waitKey(10);
    }

    return filter_measurements_struct;
}//*/


#include <chrono>
#include <iostream>
#include <functional>
#include <memory>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "sensor_msgs/msg/compressed_image.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "tf2_ros/transform_broadcaster.h"
#include <opencv2/opencv.hpp>

using std::placeholders::_1;
using namespace std::chrono_literals;

const char* MODEL_PATH      = "/home/nova/nova/src/ros/rover/object_detection/resources/model.onnx";
const char* RGB_IMAGE_TOPIC = "/oak/rgb/image_raw/compressed";
const char* ODOMETRY_TOPIC  = "";

const float DEPTH_CAMERA_POS_OFFSET_X = 0.49f;
const float DEPTH_CAMERA_POS_OFFSET_Y = 0.0f;
const float DEPTH_CAMERA_POS_OFFSET_Z = 0.48f;

struct Detection
{
    cv::Rect box;
    int class_id;
    float confidence;
};

//See https://github.com/mallumoSK/yolov8/blob/master/yolo/YoloDet.cpp

static std::vector<Detection> run_model_inference(cv::dnn::Net& network, cv::Size model_shape, int class_count, const cv::Mat& frame, float min_confidence = 0.5f)
{
    bool crop = false;
    bool swap_rb = true;

    //Convert the frame to a opencv blob that can be an input to the network 
    cv::Mat blob = cv::dnn::blobFromImage(frame, 1.0f / 255.0f, model_shape, cv::Scalar(), swap_rb, crop);

    //Run model
    network.setInput(blob);

    std::vector<cv::Mat> outputs;
    network.forward(outputs, network.getUnconnectedOutLayersNames());

    //Reshape output to make it easy to parse
    int rows = outputs[0].size[2];
    int dims = outputs[0].size[1];
    outputs[0] = outputs[0].reshape(1, dims);
    cv::transpose(outputs[0], outputs[0]);

    //Calculate conversions from model coordinates to input image coordinates
    float x_scale = (float)frame.cols / model_shape.width;
    float y_scale = (float)frame.rows / model_shape.height;

    //Parse output
    float* data = (float*)outputs[0].data;
    std::vector<int> class_ids;
    std::vector<cv::Rect> boxes;
    std::vector<float> confidences;

    for (int row = 0; row < rows; row++)
    {
        float* scores_data = data + 4;

        cv::Mat scores(1, class_count, CV_32FC1, scores_data);
        cv::Point class_id;
        double max_class_confidence;

        cv::minMaxLoc(scores, nullptr, &max_class_confidence, nullptr, &class_id);
        
        if (max_class_confidence >= min_confidence)
        {
            confidences.push_back((float)max_class_confidence);
            class_ids.push_back(class_id.x);

            float x = data[0] * x_scale;
            float y = data[1] * y_scale;
            float w = data[2] * x_scale;
            float h = data[3] * y_scale;

            boxes.emplace_back((int)(x - 0.5f * w), (int)(y - 0.5f * h), (int)w, (int)h);
        }

        data += dims;
    }

    //Run non-maximum suppression to eliminate duplicate detections
    float nms_threshold = 0.5f;
    std::vector<int> nms_result;
    cv::dnn::NMSBoxes(boxes, confidences, min_confidence, nms_threshold, nms_result);

    //nms_result is a list of indices into boxes and confidences
    std::vector<Detection> result;
    for (int index : nms_result)
    {
        Detection detection = {};
        detection.box = boxes[index];
        detection.class_id = class_ids[index];
        detection.confidence = confidences[index];
        result.push_back(detection);
    }

    return result;
}

class ObjectDetectionNode : public rclcpp::Node
{
    cv::dnn::Net onnx_network;
    cv::Size model_shape;
    std::vector<std::string> model_classes;

    rclcpp::Subscription<sensor_msgs::msg::CompressedImage>::SharedPtr rgb_image_subscription;

    std::unique_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_blue;

public:
    ObjectDetectionNode() 
        : Node("object_detection")
    {
        //Load ONNX model
        model_shape = {640, 640};
        model_classes = {"blue", "green", "red", "white"};

        onnx_network = cv::dnn::readNetFromONNX(MODEL_PATH);
        RCLCPP_INFO(this->get_logger(), "Opened ONNX model file '%s'", MODEL_PATH);

        onnx_network.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
        onnx_network.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);

        //Subscribe to compressed image
        rgb_image_subscription = this->create_subscription<sensor_msgs::msg::CompressedImage>(RGB_IMAGE_TOPIC, 10, std::bind(&ObjectDetectionNode::image_callback, this, _1));

        tf_broadcaster_blue = std::make_unique<tf2_ros::TransformBroadcaster>(*this);
    }

private:
    void image_callback(const sensor_msgs::msg::CompressedImage::SharedPtr compressed_image)
    {
        cv::Mat image = cv::imdecode(compressed_image->data, cv::IMREAD_COLOR);

        std::vector<Detection> detections = run_model_inference(onnx_network, model_shape, model_classes.size(), image, 0.5f);
        for (Detection detection : detections)
        {
            std::string& colour = model_classes[detection.class_id];
            
            std::cout << "Found " << colour << " box at {" << (detection.box.x + detection.box.width / 2) << ", " << (detection.box.y + detection.box.height / 2) << "}\n";
        }

        cv::Scalar colours[] = {CV_RGB(0, 0, 255), CV_RGB(0, 255, 0), CV_RGB(255, 0, 0), CV_RGB(255, 255, 255)};

        for (Detection detection : detections)
        {
            cv::rectangle(image, detection.box, colours[detection.class_id], 2);
        }

        cv::imwrite("/home/nova/output.jpeg", image);
    }
};

//Test code
static void run_test()
{    cv::Size model_shape = {640, 640};
    std::vector<std::string> model_classes = {"blue", "green", "red", "white"};

    cv::dnn::Net onnx_network = cv::dnn::readNetFromONNX(MODEL_PATH);

    cv::Mat image = cv::imread("/home/nova/input.png");

    std::vector<Detection> detections = run_model_inference(onnx_network, model_shape, model_classes.size(), image, 0.5f);

    cv::Scalar colours[] = {CV_RGB(0, 0, 255), CV_RGB(0, 255, 0), CV_RGB(255, 0, 0), CV_RGB(255, 255, 255)};

    for (Detection detection : detections)
    {
        cv::rectangle(image, detection.box, colours[detection.class_id], 2);
    }

    cv::imwrite("/home/nova/output.jpeg", image);
}

int main(int argc, char * argv[])
{
    //run_test();

    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<ObjectDetectionNode>());
    rclcpp::shutdown();
    return 0;
}
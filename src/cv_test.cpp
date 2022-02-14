#include <stdio.h>
#include <opencv2/opencv.hpp>
//#include <pybind11/pybind11.h>
//#include <pybind11/stl.h>

//namespace py = pybind11;
using namespace cv;

int main(int argc, char** argv)
{
	if ( argc != 2 ) {
		printf("usage: DisplayImage.out <image_path>\n");
		return -1;
	}

	Mat image;
	image = imread(argv[1], 1);

	if (!image.data) {
		printf("No image data \n");
		return -1;
	}

	namedWindow("Display Image", WINDOW_AUTOSIZE );
	cv::resizeWindow("Display Image", 600, 600);
	imshow("Display Image", image);

	waitKey(0);

	return 0;
}

/*PYBIND11_MODULE(cv_test, m) {
	m.def("cv_test", &main);
}*/

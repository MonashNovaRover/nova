import cv2
import cv2.aruco as ar
import numpy as np
import os



def findArTag(img,markerSize=6,totalMarkers=250,draw=True):

	imgGray = cv2.cvtColor(img,cv2.COLOR_BGR2GRAY)
	arDict = ar.Dictionary_get(ar.DICT_6X6_250)
	arParam = ar.DetectorParameters_create()
	bboxs,ids,rejected = ar.detectMarkers(imgGray,arDict,parameters =arParam)
	print("AR TAG DETECTED WITH ID")
	print(ids)

	camera_calibration_parameters_filename = 'calibration_chessboard.yaml'
	aruco_marker_side_length = 0.0785
	cv_file = cv2.FileStorage(camera_calibration_parameters_filename, cv2.FILE_STORAGE_READ)
	mtx = cv_file.getNode('K').mat()
	dst = cv_file.getNode('D').mat()
	cv_file.release()



	if ids is not None:
		ar.drawDetectedMarkers(img,bboxs)

		rot_mat,trans_mat = ar.estimatePoseSingleMarkers(bboxs,aruco_marker_side_length,mtx,dst)


		for i, id in enumerate(ids):
			trans_x = trans_mat[i][0][0]
			trans_y = trans_mat[i][0][1]
			trans_z = trans_mat[i][0][2]

			print("translation in x : " + str(trans_x))
			print("translation in y : " + str(trans_y))
			print("translation in z : " + str(trans_z))

			ar.drawAxis(img,mtx,dst,rot_mat[i],trans_mat[i],0.05)




def main():
	cap = cv2.VideoCapture(0)

	while True:
		success,img = cap.read()
		findArTag(img)
		cv2.imshow("Image",img)
		cv2.waitKey(1)

if __name__ == "__main__":
	main()

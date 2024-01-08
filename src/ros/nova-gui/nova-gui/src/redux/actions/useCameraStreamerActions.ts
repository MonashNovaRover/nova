import { useDispatch } from "react-redux";
import { Camera, CameraStreamerStatus } from "../models/CameraStreamState";
import { CameraStreamerAction } from "../slices/CameraStreamSlice";

export const useCameraStreamerActions = () => {
  const dispatch = useDispatch();

  const updateStatus = (status: CameraStreamerStatus) => {
    dispatch({ type: CameraStreamerAction.UPDATE_STATUS, payload: status });
  };

  const updateCameras = (cameras: Camera[]) => {
    dispatch({ type: CameraStreamerAction.UPDATE_STATUS, payload: cameras });
  };

  return {
    updateCameras,
    updateStatus,
  };
};

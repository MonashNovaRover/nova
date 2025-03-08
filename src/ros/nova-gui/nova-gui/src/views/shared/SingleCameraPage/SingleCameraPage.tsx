import { useParams } from "react-router-dom";
import { useCameraStreamer } from "../../../components/CameraComponent/hooks/useCameraStreamer";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState";
import { Spinner } from "@nextui-org/react";
import SerialMappedCameraComponent from "../CamerasPage/SerialMappedCameraComponent.tsx";

export const SingleCameraPage = () => {
  const { serial } = useParams<{ serial: string }>();

  useCameraStreamer();

  const camerasFromRos = useSelector(
    (state: RootState) => state.camerasStore.cameras
  );

  const isOnline = camerasFromRos
    .map((cam) => cam.serial)
    .includes(serial ?? "");

  if (!serial || !isOnline)
    return (
      <div className="w-full h-full min-h-screen flex flex-col items-center justify-center">
        {!serial ? (
          <div className="font-bold text-medium">No Serial Specified</div>
        ) : (
          <Spinner />
        )}
      </div>
    );

  return <SerialMappedCameraComponent cameraSerial={serial} />;
};

import {CameraComponent} from "../../../components/CameraComponent/CameraComponent.tsx";
import {CameraSerials} from "../../shared/CamerasPage/CameraPageConstants";

export default function TestScimbalCamView () {

  return (
    <div className="p-3">
      <CameraComponent cameraSerial={CameraSerials.SCIENCE_GIMBAL}/>
    </div>
  );
}
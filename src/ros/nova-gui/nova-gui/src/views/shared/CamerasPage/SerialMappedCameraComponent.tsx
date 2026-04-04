import {FC, memo, useMemo} from "react";
import {BaseCameraComponentProps, CameraComponent} from "../../../components/cameras/CameraComponent/CameraComponent.tsx";
import {CameraSerials} from "./CameraPageConstants.tsx";
import BarOverlayedCameraComponent from "../../../components/cameras/CameraComponent/special/BarOverlayedCameraComponent.tsx";
//import KeyboardOverlayedCameraComponent from "../../../components/CameraComponent/special/KeyboardOverlayedCameraComponent.tsx";
import {
  GimbalOverlayedCameraComponent
} from "../../../components/cameras/CameraComponent/special/GimbalOverlayedCameraComponent.tsx";
import DriveCameraComponent from "../../../components/cameras/CameraComponent/special/DriveCameraComponent.tsx";
import WheelTelemetryCameraComponent
  from "../../../components/cameras/CameraComponent/special/WheelTelemetryCameraComponent.tsx";
import SiteSelectCameraComponent from "../../../components/cameras/CameraComponent/special/SiteSelectCameraComponent.tsx";
import ActivatedNodesCameraComponent
  from "../../../components/cameras/CameraComponent/special/ActivatedNodesCameraComponent.tsx";
import DepthSensor
  from "../../../components/cameras/CameraComponent/special/DepthSensorCameraComponent.tsx";
import MicroscopeScaleOverlayedCameraComponent
  from "../../../components/cameras/CameraComponent/special/MicroscopeScaleOverlayedCameraComponent.tsx";
import PowerCycleCameraComponent
  from "../../../components/cameras/CameraComponent/special/PowerCycleCameraComponent.tsx";
import DriveControlCameraComponent
  from "../../../components/cameras/CameraComponent/special/DriveControlCameraComponent.tsx";
import ScienceCombinedCameraComponent
  from "../../../components/cameras/CameraComponent/special/ScienceCombinedCameraComponent.tsx";
import YoloCameraComponent from "../../../components/auto/ObjectDetection/YoloCameraComponent.tsx";

/// Defines special components to use for certain cameras
export const cameraSerialToComponentMap: { [k: string]: FC<BaseCameraComponentProps> } = {
  [CameraSerials.ARM_END_PERISCOPE]: BarOverlayedCameraComponent,
  [CameraSerials.SCIENCE_GIMBAL]: GimbalOverlayedCameraComponent,
  [CameraSerials.WHEEL_TELEMETRY]: WheelTelemetryCameraComponent,
  [CameraSerials.DRIVE_TELEMETRY]: DriveCameraComponent,
  [CameraSerials.DRIVE_CONTROL]: DriveControlCameraComponent,
  [CameraSerials.SITE_SELECT]: SiteSelectCameraComponent,
  [CameraSerials.URC_ACTIVATED_NODES]: ActivatedNodesCameraComponent,
  [CameraSerials.URC_SCIENCE_AUGER_DEPTH_SENSORS]: DepthSensor,
  [CameraSerials.SCIENCE_MICROSCOPE]: MicroscopeScaleOverlayedCameraComponent,
  [CameraSerials.SCIENCE_POWER_CYCLE]: PowerCycleCameraComponent,
  [CameraSerials.SCIENCE_COMBINED]: ScienceCombinedCameraComponent,
  [CameraSerials.AUTO_FORWARD]: YoloCameraComponent,
  [CameraSerials.AUTO_RIGHT]: YoloCameraComponent,
  [CameraSerials.AUTO_LEFT]: YoloCameraComponent,
  [CameraSerials.AUTO_BEHIND]: YoloCameraComponent,
}

/// Function that used the above map to get the component for a specified camera serial
const cameraSerialToComponent: (serial: string) => FC<BaseCameraComponentProps> = (serial) => {
  if (serial in cameraSerialToComponentMap)
    return cameraSerialToComponentMap[serial];
  return CameraComponent;
}

const SerialMappedCameraComponentUnmemoed: FC<BaseCameraComponentProps> = (props) => {
  const SerialMappedComponent = useMemo(() => cameraSerialToComponent(props.cameraSerial), [props.cameraSerial]);
  return <SerialMappedComponent {...props}></SerialMappedComponent>
}

const SerialMappedCameraComponent = memo(SerialMappedCameraComponentUnmemoed);
export default SerialMappedCameraComponent;

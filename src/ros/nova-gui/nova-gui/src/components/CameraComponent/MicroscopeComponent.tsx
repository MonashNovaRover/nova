import { Card, Slider } from "@nextui-org/react";
import { CameraComponent } from "./CameraComponent";
import { useCameraStreamer } from "./hooks/useCameraStreamer";
import { RootState } from "../../redux/RootState";
import { useSelector } from "react-redux";
import { RosTopic } from "../../ros/topics/rosTopic";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { useEffect } from "react";
import { RosService } from "../../ros/services/rosService";
import { CameraSerials } from "../../views/shared/CamerasPage/CameraPageConstants";

const MicroscopeComponent: React.FC = () => {
  // Invoking Bifrost and pointing it towards MICROSCOPE_SERVO
  const topicBifrost = useBifrost({ topic: RosTopic.MICROSCOPE_SERVO });

  const microscopeServoState = useSelector(
    (state: RootState) => state.microscopeServoStore
  );

  // Wrap with useEffect hook to only run it once
  useEffect(() => {
    // call bifrost.syncWithTopic() to initiate Realtime Updates
    topicBifrost.syncWithTopic();
  }, [topicBifrost]);

  // Accessing the Store using useSelector hook
  const microscopeServoService = useSelector((state: RootState) => state.microscopeServiceStore);

  // Invoking Bifrost and pointing it towards SET_SERVO
  const serviceBifrost = useBifrost({ service: RosService.MOVE_MICROSCOPE_SERVO });

  const setZoomFocus = (angle: number) => serviceBifrost.callServiceToRedux(angle);

  useCameraStreamer();

  return (
    <Card className={`m-4`}>
      <CameraComponent cameraSerial={CameraSerials.SCIENCE_MICROSCOPE} />
      <Slider
        className="max-w-full pt-2 pb-6 pl-8 pr-8"
        size="lg"
        label="Zoom/Focus"
        startContent="1x"
        endContent="100x"
        value={microscopeServoState.angle}
        onChange={(value) => setZoomFocus(value as number)}
      />
      { microscopeServoService.success ? <div>ERROR executing microscope servo request on rover</div> : null}
    </Card>
  );
};
export default MicroscopeComponent;

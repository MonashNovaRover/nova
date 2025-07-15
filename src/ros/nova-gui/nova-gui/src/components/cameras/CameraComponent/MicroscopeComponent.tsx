import { Card, Slider } from "@nextui-org/react";
import { CameraComponent } from "./CameraComponent.tsx";
import { useCameraStreamer } from "./hooks/useCameraStreamer.ts";
import { RootState } from "../../../redux/RootState.ts";
import { useSelector } from "react-redux";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { useEffect, useState } from "react";
import { RosService } from "../../../ros/services/rosService.ts";
import { CameraSerials } from "../../../views/shared/CamerasPage/CameraPageConstants.tsx";

const MicroscopeComponent: React.FC = () => {
  // Invoking Bifrost and pointing it towards MICROSCOPE_SERVO
  const topicBifrost = useBifrost({ topic: RosTopic.MICROSCOPE_SERVO });

  const microscopeServoState = useSelector(
    (state: RootState) => state.microscopeServoStore
  );

  // Accessing the Store using useSelector hook
  const microscopeServoService = useSelector((state: RootState) => 
    state.microscopeServiceStore
  );

  // Invoking Bifrost and pointing it towards SET_SERVO
  const serviceBifrost = useBifrost({ service: RosService.MOVE_MICROSCOPE_SERVO });

  const setZoomFocus = (angle: number) => serviceBifrost.callService({ angle: angle });

  useCameraStreamer();

  const [zoom, setZoom] = useState(45);


  // Wrap with useEffect hook to only run it once
  useEffect(() => {
    // call bifrost.syncWithTopic() to initiate Realtime Updates
    topicBifrost.syncWithTopic();
  }, [topicBifrost]);

  useEffect(() => {
    if (microscopeServoState.angle !== zoom) {
      setZoomFocus(zoom);
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [zoom]);

  return (
    <Card className={`m-4`}>
      <CameraComponent cameraSerial={CameraSerials.SCIENCE_MICROSCOPE} />
      <Slider
        className="max-w-full pt-2 pb-6 pl-8 pr-8"
        size="lg"
        label="Zoom/Focus"
        startContent="0%"
        endContent="100%"
        minValue={0}
        maxValue={90}
        step={1}
        value={zoom}
        onChange={(value) => setZoom(value as number)}
      />
      { !microscopeServoService.success && <div>ERROR executing microscope servo request on rover</div>}
    </Card>
  );
};
export default MicroscopeComponent;

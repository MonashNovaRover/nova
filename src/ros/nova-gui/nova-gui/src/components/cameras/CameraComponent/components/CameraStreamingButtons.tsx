import {Button,} from "@nextui-org/react";
import {useBifrost} from "../../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../../../ros/topics/rosTopic.ts";
import {useEffect} from "react";
import {RosService} from "../../../../ros/services/rosService.ts";
import {Pause, Play} from "react-feather";

export interface CameraStreamingButtonsProps{
  isStartButton: boolean
  refreshAvailabilies: () => void;
  size: "sm" | "md" | "lg"
}

export const CameraStreamingButton = ({
    isStartButton,
    refreshAvailabilies,
    ...buttonProps
  }: CameraStreamingButtonsProps) => {
  const bifrost = useBifrost({ topic: RosTopic.CAMERAS });

  const bifrostStarter = useBifrost({ service: RosService.START_CAMS });

  const bifrostStopper = useBifrost({ service: RosService.PAUSE_CAMS });

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const startStreaming = () =>
    bifrostStarter.callService(
      { serials: [] },
      {
        responseToast: true,
        successToastMessage: "All Cameras Started Up!",
        handleResponse: () => refreshAvailabilies(),
      }
    );

  const pauseStreaming = () =>
    bifrostStopper.callService(
      { serials: [] },
      {
        responseToast: true,
        successToastMessage: "All Cameras Paused!",
        handleResponse: () => refreshAvailabilies(),
      }
    );

  return isStartButton ? (
    <Button size={buttonProps.size} color="primary" onPress={startStreaming}>
      <Play size={15}/> Start Streaming
    </Button>
  ) : (
    <Button size={buttonProps.size} color="warning" onPress={pauseStreaming}>
      <Pause size={15}/> Pause Streaming
    </Button>
  );
};

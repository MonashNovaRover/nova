import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosService} from "../../../ros/services/rosService.ts";
import {Button} from "@nextui-org/react";
import {RefreshCcw, Square} from "react-feather";

export const UVVisSpecStartStopButtons = () => {
  const bifrostStart = useBifrost({ service: RosService.UV_VIS_SPEC_START });
  const startCameraFeed = () => bifrostStart.callService({});

  const bifrostStop = useBifrost({ service: RosService.UV_VIS_SPEC_STOP });
  const stopCameraFeed = () => bifrostStop.callService({});

  return (
    <div className="flex flex-row gap-3">
      <Button
        isIconOnly
        color="warning"
        size="sm"
        variant="light"
        onPressStart={startCameraFeed}
      >
        <RefreshCcw size={18}/>
      </Button>
      <Button
        isIconOnly
        color="danger"
        size="sm"
        variant="light"
        onPressStart={stopCameraFeed}
      >
        <Square size={18}/>
      </Button>
    </div>
  )

}


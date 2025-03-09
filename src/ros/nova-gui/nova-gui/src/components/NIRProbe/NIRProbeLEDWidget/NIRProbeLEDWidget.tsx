import {Card, CardBody, CardHeader, CardProps} from "@nextui-org/react";
import {useEffect} from "react";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../../redux/RootState.ts";
import {RosService} from "../../../ros/services/rosService.ts";
import SpinnerButton from "../../shared/buttons/SpinnerButton.tsx";
import {IRosNovaInterfacesNirProbeDataConst} from "../../../ros/rosTypes.ts";

interface INIRProbeLEDWidgetProps extends CardProps {
}

/**
 * Widget containing buttons to call the ros2 service "/science/take_nir_probe_reading"
 * @param cardProps
 * @constructor
 */
const NIRProbeLEDWidget: React.FC<INIRProbeLEDWidgetProps> = ({...cardProps}) => {
  const bifrost = useBifrost({ topic: RosTopic.NIR_DATA, service: RosService.TAKE_NIR_PROBE_READING });
  const status = useSelector((state: RootState) => state.nirStore.status);
  const takeReading = (led: number) => bifrost.callService({led: led});

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  return (
    <Card {...cardProps}>
      <CardHeader className="pb-0">
        NIR Probe LED
      </CardHeader>
      <CardBody className="grid grid-cols-2 gap-3">
        <SpinnerButton
          onClick={() => takeReading(IRosNovaInterfacesNirProbeDataConst.LED_WATER)}
          isLoading={status === IRosNovaInterfacesNirProbeDataConst.LED_WATER}
          isDisabled={status === IRosNovaInterfacesNirProbeDataConst.LED_ICE}
        >
          Take Water LED Reading
        </SpinnerButton>
        <SpinnerButton
          onClick={() => takeReading(IRosNovaInterfacesNirProbeDataConst.LED_ICE)}
          isLoading={status === IRosNovaInterfacesNirProbeDataConst.LED_ICE}
          isDisabled={status === IRosNovaInterfacesNirProbeDataConst.LED_WATER}
        >
          Take Ice LED Reading
        </SpinnerButton>
      </CardBody>
    </Card>
  );
}

export default NIRProbeLEDWidget;
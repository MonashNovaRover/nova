import React, {useEffect} from "react";
import {Button, Card, CardBody, CardHeader, CardProps} from "@nextui-org/react";
import {OverlayedProgress} from "../OverlayedProgress/OverlayedProgress.tsx";
import {useBifrost} from "../../redux/actions/bifrost/useBifrostAction";
import {RootState} from "../../redux/RootState";
import {useSelector} from "react-redux";
import {RosTopic} from "../../ros/topics/rosTopic";
import {RosService} from "../../ros/services/rosService.ts";
import {IRosNovaInterfacesMoveHydraprobeRequestConst} from "../../ros/rosTypes.ts";

export interface IHydroprobeProps extends CardProps {}

const HydroprobeWidget: React.FC<IHydroprobeProps> = (
  props: IHydroprobeProps
) => {
  const bifrost = useBifrost({ topic: RosTopic.HYDRAPROBE_DATA, service: RosService.HYDRAPROBE_COMMAND });
  const moveHydraprobe = (command: IRosNovaInterfacesMoveHydraprobeRequestConst) => {
    console.log("moving hydraprobe")
    bifrost.callService({command: command})
  };
  const serviceBifrost = useBifrost({ service: RosService.REQUEST_HYDRAPROBE_READING });
  const temperature = useSelector((state: RootState) => state.hydraprobeData.temperature);
  const moisture = useSelector((state: RootState) => state.hydraprobeData.moisture);
  const conductivity = useSelector((state: RootState) => state.hydraprobeData.conductivity);
  const dielectric = useSelector((state: RootState) => state.hydraprobeData.dielectric);

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const requestReading = () => {
    serviceBifrost.callService({});
  };

  const HydroprobeCardBody = (
    <CardBody className="gap-4">
      <div className="grid grid-cols-2 grid-rows-2 gap-4">
        <div className="text-center">
          <OverlayedProgress size="lg" label="Temperature" value={temperature}>
            {temperature.toFixed(2)} °C
          </OverlayedProgress>
        </div>
        <div className="text-center">
          <OverlayedProgress size="lg" label="Moisture" value={moisture}>
            {moisture.toFixed(2)} %
          </OverlayedProgress>
        </div>
        <div className="text-center">
          <OverlayedProgress size="lg" label="Conductivity" value={conductivity}>
            {conductivity.toFixed(2)} mS/cm
          </OverlayedProgress>
        </div>
        <div className="text-center">
          <OverlayedProgress size="lg" label="Dielectric" value={dielectric}>
            {dielectric.toFixed(2)}
          </OverlayedProgress>
        </div>
      </div>
      <div className="grid grid-cols-3 gap-4">
        <Button size="sm" onClick={() => moveHydraprobe(IRosNovaInterfacesMoveHydraprobeRequestConst.RESET)}>Reset</Button>
        <Button size="sm" onClick={() => moveHydraprobe(IRosNovaInterfacesMoveHydraprobeRequestConst.DEPLOY)}>Deploy</Button>
        <Button size="sm" onClick={() => moveHydraprobe(IRosNovaInterfacesMoveHydraprobeRequestConst.RETRACT)}>Retract</Button>
      </div>
      <div className="flex justify-center py-2">
        <Button onClick={requestReading} color="primary">
          Request New Reading
        </Button>
      </div>
    </CardBody>
  );

  return (
    <Card {...props}>
      <CardHeader className="text-h1 pb-0">Hydroprobe Data</CardHeader>
      {HydroprobeCardBody}
    </Card>
  );
};

export default HydroprobeWidget;

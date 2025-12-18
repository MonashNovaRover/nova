import React, {useEffect, useMemo} from "react";
import {Button, Card, CardBody, CardHeader, CardProps} from "@nextui-org/react";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RootState} from "../../../redux/RootState.ts";
import {useSelector} from "react-redux";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import {RosService} from "../../../ros/services/rosService.ts";
import {IRosScienceInterfacesMoveHydraprobeRequestConst} from "../../../ros/rosTypes.ts";
import SensorDataDisplay from "../SensorDataDisplay.tsx";

export interface IHydroprobeProps extends CardProps {}

const HydroprobeWidget: React.FC<IHydroprobeProps> = (
  props: IHydroprobeProps
) => {
  const bifrost = useBifrost({ topic: RosTopic.HYDRAPROBE_DATA, service: RosService.HYDRAPROBE_COMMAND });
  const moveHydraprobe = (command: IRosScienceInterfacesMoveHydraprobeRequestConst) => {
    console.log("moving hydraprobe")
    bifrost.callService({command: command})
  };
  const serviceBifrost = useBifrost({ service: RosService.REQUEST_HYDRAPROBE_READING });
  const temperature = useSelector((state: RootState) => state.hydraprobeData.temperature);
  const moisture = useSelector((state: RootState) => state.hydraprobeData.moisture);
  const conductivity = useSelector((state: RootState) => state.hydraprobeData.conductivity);
  const dielectric = useSelector((state: RootState) => state.hydraprobeData.dielectric);
  const sensorData = useMemo(() => [temperature, moisture, conductivity, dielectric], [temperature, moisture, conductivity, dielectric])

  useEffect(() => {
    bifrost.syncWithTopic();
    serviceBifrost.syncWithTopic()
  }, [bifrost]);

  const requestReading = () => {
    serviceBifrost.callService({});
  };

  // IN CASE THE ROS CONTEXT ISSUE HAPPENS AGAIN WITH THE ACTUAL HYDRAPROBE (been fine with dummy data, couldn't try again with the hydraprobe)
  // Cursed hack to make the ros context not undefined for this topic.
  // const [fixRosState, setFixRosState] = useState<boolean>(false);
  // useEffect(() => {
  //   const thing = setTimeout(() => {
  //     bifrost.syncWithTopic();
  //     if (!fixRosState)
  //       setFixRosState(() => true);
  //   }, 1000);
  //
  //   return () => {
  //     clearTimeout(thing);
  //   }
  // }, [fixRosState, setFixRosState]);

  const HydraprobeCardBody = (
    <CardBody className="gap-4">
      <SensorDataDisplay
        values={sensorData}
        labels={["Temperature", "Moisture", "Conductivity", "Dielectric"]}
        suffixes={["°C", "%", "mS/cm", ""]}
      />
      <div className="grid grid-cols-3 gap-4">
        <Button onClick={() => moveHydraprobe(IRosScienceInterfacesMoveHydraprobeRequestConst.RESET)}>Reset</Button>
        <Button onClick={() => moveHydraprobe(IRosScienceInterfacesMoveHydraprobeRequestConst.DEPLOY)}>Deploy</Button>
        <Button onClick={() => moveHydraprobe(IRosScienceInterfacesMoveHydraprobeRequestConst.RETRACT)}>Retract</Button>
      </div>
      <Button onPress={requestReading} color="primary">
        Request New Reading
      </Button>
    </CardBody>
  );

  return (
    <Card {...props}>
      <CardHeader className="text-h1 pb-0">Hydraprobe Data</CardHeader>
      {HydraprobeCardBody}
    </Card>
  );
};

export default HydroprobeWidget;

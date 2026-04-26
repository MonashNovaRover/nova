import React, {useEffect, useMemo} from "react";
import {Card, CardBody, CardHeader, CardProps} from "@nextui-org/react";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RootState} from "../../../redux/RootState.ts";
import {useSelector} from "react-redux";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import SensorDataDisplay from "../SensorDataDisplay.tsx";

export interface IHydroprobeProps extends CardProps {}

const HydroprobeWidget: React.FC<IHydroprobeProps> = (
  props: IHydroprobeProps
) => {
  const bifrost = useBifrost({ topic: RosTopic.HYDRAPROBE_DATA });
  const temperature = useSelector((state: RootState) => state.hydraprobeData.temperature);
  const moisture = useSelector((state: RootState) => state.hydraprobeData.moisture);
  const sensorData = useMemo(() => [temperature, moisture], [temperature, moisture])

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const HydraprobeCardBody = (
    <CardBody className="gap-4">
      <SensorDataDisplay
        values={sensorData}
        labels={["Temperature", "Moisture"]}
        suffixes={["°C", "%"]}
      />
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

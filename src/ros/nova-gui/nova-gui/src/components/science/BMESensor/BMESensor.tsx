import React, {useEffect, useMemo} from "react";
import {Card, CardHeader, CardBody, CardProps, Button} from "@nextui-org/react";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RootState } from "../../../redux/RootState.ts";
import { useSelector } from "react-redux";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";
import SensorDataDisplay from "../SensorDataDisplay.tsx";
import {RosService} from "../../../ros/services/rosService.ts";

export interface IBMESensorProps extends CardProps {}

const BMESensor: React.FC<IBMESensorProps> = (
    props: IBMESensorProps
) => {
  const bifrost = useBifrost({ topic: RosTopic.BME_SENSOR, service: RosService.BME_TRIGGER });
  const temperature = useSelector((state: RootState) => state.bmeSensorStore.temperature);
  const humidity = useSelector((state: RootState) => state.bmeSensorStore.humidity);
  const pressure = useSelector((state: RootState) => state.bmeSensorStore.pressure);
  const sensorData = useMemo(() => [temperature, humidity, pressure], [temperature, humidity, pressure])


  useEffect(() => {
      bifrost.syncWithTopic();
  }, [bifrost]);

  const BMESensorCardBody = (
    <CardBody className="flex flex-col gap-3">
      <SensorDataDisplay
        values={sensorData}
        labels={["Temperature", "Humidity", "Pressure"]}
        suffixes={["°C", "%", "hPa"]}
      />
      <Button onPressStart={() => bifrost.callService({})}>
        Take Reading
      </Button>
    </CardBody>
  );

  return (
    <Card {...props}>
      <CardHeader className="text-h1 pb-0">BME Sensor Data</CardHeader>
      {BMESensorCardBody}
    </Card>
  );
};

export default BMESensor;
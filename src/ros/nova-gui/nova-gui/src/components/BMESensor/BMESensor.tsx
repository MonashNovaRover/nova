import React, {useEffect, useMemo} from "react";
import { Card, CardHeader, CardBody, CardProps } from "@nextui-org/react";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { RootState } from "../../redux/RootState";
import { useSelector } from "react-redux";
import { RosTopic } from "../../ros/topics/rosTopic";
import SensorDataDisplay from "../science/SensorDataDisplay.tsx";

export interface IBMESensorProps extends CardProps {}

const BMESensor: React.FC<IBMESensorProps> = (
    props: IBMESensorProps
) => {
  const bifrost = useBifrost({ topic: RosTopic.BME_SENSOR });
  const temperature = useSelector((state: RootState) => state.bmeSensorStore.temperature);
  const humidity = useSelector((state: RootState) => state.bmeSensorStore.humidity);
  const pressure = useSelector((state: RootState) => state.bmeSensorStore.pressure);
  const altitude = useSelector((state: RootState) => state.bmeSensorStore.altitude);
  const sensorData = useMemo(() => [temperature, humidity, pressure, altitude], [temperature, humidity, pressure, altitude])


  useEffect(() => {
      bifrost.syncWithTopic();
  }, [bifrost]);

  const BMESensorCardBody = (
    <CardBody>
      <SensorDataDisplay
        values={sensorData}
        labels={["Temperature", "Humidity", "Pressure", "Altitude"]}
        suffixes={["°C", "%", "hPa", ""]}
      />
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
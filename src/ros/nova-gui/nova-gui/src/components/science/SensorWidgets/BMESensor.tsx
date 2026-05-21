import React, {useEffect, useMemo} from "react";
import {Card, CardHeader, CardBody, CardProps, Button} from "@nextui-org/react";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RootState } from "../../../redux/RootState.ts";
import { useSelector } from "react-redux";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";
import SensorDataDisplay from "./SensorDataDisplay.tsx";
import {RosService} from "../../../ros/services/rosService.ts";
import {Save} from "react-feather";
import {useSiteSensorData} from "./useSiteSensorData.ts";
import {SensorData} from "../../../redux/models/genericStores/SiteDataState.ts";
import toast from "react-hot-toast";

export interface IBMESensorProps extends CardProps {}

const BMESensor: React.FC<IBMESensorProps> = (
    props: IBMESensorProps
) => {
  const bifrost = useBifrost({ topic: RosTopic.BME_SENSOR, service: RosService.BME_TRIGGER });
  const temperature = useSelector((state: RootState) => state.bmeSensorStore.temperature);
  const humidity = useSelector((state: RootState) => state.bmeSensorStore.humidity);
  const pressure = useSelector((state: RootState) => state.bmeSensorStore.pressure);
  const sensorData = useMemo(() => [temperature, humidity, pressure], [temperature, humidity, pressure])

  // rover GPS data
  const roverLocation = useSelector((state: RootState) => state.roverLocationStore);

  const [_, __, addSensorData] = useSiteSensorData()

  useEffect(() => {
      bifrost.syncWithTopic();
  }, [bifrost]);

  const saveData = () => {
    const bmeData: SensorData[] = [
      {name: "BME Temperature", data: temperature},
      {name: "BME Humidity", data: humidity},
      {name: "BME Pressure", data: pressure},
    ];

    const gpsData: SensorData[] = [
      {name: "Latitude", data: roverLocation.latitude},
      {name: "Longitude", data: roverLocation.longitude},
      {name: "Altitude", data:  roverLocation.altitude},
    ];

    addSensorData([...bmeData, ...gpsData]);
    toast.success("BME sensor and GPS data saved")
  }

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
      <CardHeader className="text-h1 pb-0 flex flex-row justify-between">
        <span>BME Sensor Data</span>
        <Button
          isIconOnly
          variant="light"
          onPressStart={saveData}
        ><Save/></Button>
      </CardHeader>
      {BMESensorCardBody}
    </Card>
  );
};

export default BMESensor;
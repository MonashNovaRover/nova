import React, {useEffect, useMemo} from "react";
import {Button, Card, CardBody, CardHeader, CardProps} from "@nextui-org/react";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RootState} from "../../../redux/RootState.ts";
import {useSelector} from "react-redux";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import SensorDataDisplay from "./SensorDataDisplay.tsx";
import {Save, Trash2} from "react-feather";
import {useSiteSensorData} from "./useSiteSensorData.ts";
import {SensorData} from "../../../redux/models/genericStores/SiteDataState.ts";
import toast from "react-hot-toast";

export interface IHydroprobeProps extends CardProps {}

const HydroprobeWidget: React.FC<IHydroprobeProps> = (
  props: IHydroprobeProps
) => {
  const bifrost = useBifrost({ topic: RosTopic.HYDRAPROBE_DATA });
  const temperature = useSelector((state: RootState) => state.hydraprobeData.temperature);
  const moisture = useSelector((state: RootState) => state.hydraprobeData.moisture);
  const sensorData = useMemo(() => [temperature, moisture], [temperature, moisture])

  const [siteSensorData, setSiteSensorData, addSensorData] = useSiteSensorData()

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const saveData = () => {
    console.log("saving data")
    addSensorData([
      {name: "Temperature", data: temperature},
      {name: "Moisture", data: moisture},
    ] as SensorData[])
    toast.success("Hydraprobe data saved")
  }

  const deleteData = () => {
    const hydraprobeDataNames = ["Temperature", "Moisture"];
    const filteredData = siteSensorData.filter((entry) => !hydraprobeDataNames.includes(entry.name));
    setSiteSensorData(filteredData);
    toast.success("Hydraprobe data deleted");
  }

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
      <CardHeader className="text-h1 pb-0 flex flex-row justify-between">
        <span>Hydraprobe Data</span>
        <div className="flex gap-1">
          <Button
            isIconOnly
            variant="light"
            onPressStart={saveData}
          ><Save/></Button>
          <Button
            isIconOnly
            variant="light"
            color="danger"
            onPressStart={deleteData}
          ><Trash2/></Button>
        </div>
      </CardHeader>
      {HydraprobeCardBody}
    </Card>
  );
};

export default HydroprobeWidget;

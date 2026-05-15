import React from "react";
import {Card, CardHeader, CardBody} from "@nextui-org/react";
import {useSiteSensorData} from "../SensorWidgets/useSiteSensorData.ts";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";
import {Site} from "../../../redux/models/genericStores/CurrentSiteStore.ts";
import SensorDataDisplay from "../SensorWidgets/SensorDataDisplay.tsx";

const SensorDataWidget: React.FC = () => {
  const [currentSite, _] = useGenericStore<Site>("currentSite");
  const [sensorData, __, ___] = useSiteSensorData();

  return (
    <Card>
      <CardHeader className="text-h1 pb-0">
        Sensor Data - Site {currentSite + 1}
      </CardHeader>
      <CardBody>
        {sensorData.length === 0 ? (
          <p className="text-default-500 text-center py-4">No sensor data saved</p>
        ) : (
          <SensorDataDisplay
            values={sensorData.map(s => s.data)}
            labels={sensorData.map(s => s.name)}
            suffixes={sensorData.map(() => "")}
          />
        )}
      </CardBody>
    </Card>
  );
};

export default SensorDataWidget;

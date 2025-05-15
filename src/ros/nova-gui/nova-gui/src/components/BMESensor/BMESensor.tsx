import React, { useEffect } from "react";
import { Card, CardHeader, CardBody, CardProps } from "@nextui-org/react";
import { OverlayedProgress } from "../OverlayedProgress/OverlayedProgress.tsx";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { RootState } from "../../redux/RootState";
import { useSelector } from "react-redux";
import { RosTopic } from "../../ros/topics/rosTopic";

// This is just a repurposed HydroprobeWidget.tsx (Thanks Kabi)

export interface IBMESensorProps extends CardProps {}

const BMESensor: React.FC<IBMESensorProps> = (
    props: IBMESensorProps
) => {
    const bifrost = useBifrost({ topic: RosTopic.BME_SENSOR });
    const temperature = useSelector((state: RootState) => state.bmeSensorStore.temperature);
    const humidity = useSelector((state: RootState) => state.bmeSensorStore.humidity);
    const pressure = useSelector((state: RootState) => state.bmeSensorStore.pressure);
    const altitude = useSelector((state: RootState) => state.bmeSensorStore.altitude);

    useEffect(() => {
        bifrost.syncWithTopic();
    }, [bifrost]);

    const BMESensorCardBody = (
      <CardBody className="grid grid-cols-2 grid-rows-2 gap-4">
        <div className="text-center">
          <OverlayedProgress size="lg" label="Temperature" value={temperature}>
            {temperature.toFixed(2)} °C
          </OverlayedProgress>
        </div>
        <div className="text-center">
          <OverlayedProgress size="lg" label="Humidity" value={humidity}>
            {humidity.toFixed(2)} %
          </OverlayedProgress>
        </div>
        <div className="text-center">
          <OverlayedProgress size="lg" label="Pressure" value={pressure}>
            {pressure}
          </OverlayedProgress>
        </div>
        <div className="text-center">
          <OverlayedProgress size="lg" label="Altitude" value={altitude}>
            {altitude}
          </OverlayedProgress>
        </div>
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
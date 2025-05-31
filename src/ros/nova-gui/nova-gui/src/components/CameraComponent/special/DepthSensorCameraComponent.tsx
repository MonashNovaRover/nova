import React from "react";
import { Card, CardHeader, CardBody, Chip } from "@nextui-org/react";
import { BaseCameraComponentProps } from "../CameraComponent";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction";
import { RootState } from "../../../redux/RootState.ts";
import { useSelector } from "react-redux";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";
import { useEffect } from "react";
import SiteSelectWidget from "../../SiteSelectWidget/SiteSelectWidget.tsx";

// This is just a repurposed BMESensor.tsx (Thanks Someone)
// Which is just a repurposed HydroprobeWidget.tsx (Thanks Kabi)

const DepthSensor: React.FC<BaseCameraComponentProps> = (
    _: BaseCameraComponentProps
) => {
    // Accessing the Store using useSelector hook
    const auger1_depth_hit = useSelector(
        (state: RootState) => state.auger1DepthSensorStore.data
    );
    
    // Invoking Bifrost and pointing it towards TEMP_SENSOR
    const auger1_bifrost = useBifrost({ topic: RosTopic.AUGER1_DEPTH_SENSOR });
    
    // Wrap with useEffect hook to only run it once
    useEffect(() => {
        // call bifrost.syncWithTopic() to initiate Realtime Updates
        auger1_bifrost.syncWithTopic();
    }, [auger1_bifrost]);

    // Accessing the Store using useSelector hook
    const auger2_depth_hit = useSelector(
        (state: RootState) => state.auger2DepthSensorStore.data
    );
    
    // Invoking Bifrost and pointing it towards TEMP_SENSOR
    const auger2_bifrost = useBifrost({ topic: RosTopic.AUGER2_DEPTH_SENSOR });
    
    // Wrap with useEffect hook to only run it once
    useEffect(() => {
        // call bifrost.syncWithTopic() to initiate Realtime Updates
        auger2_bifrost.syncWithTopic();
    }, [auger2_bifrost]);

    // Colours differ from colours that any buttons use
    const DepthSensorCardBody = (
        <CardBody className="flex flex-row justify-around p-0">
            <text className="w-max">Auger 1:</text>
            <Chip className={auger1_depth_hit ? "w-min bg-green-600" : "w-min bg-rose-600"}>
                {auger1_depth_hit ? "Yes" : "No"}
            </Chip>
            <text className="w-max">Auger 2:</text>
            <Chip className={auger2_depth_hit ? "w-min bg-green-600" : "w-min bg-rose-600"}>
                {auger2_depth_hit ? "Yes" : "No"}
            </Chip>
        </CardBody>
    );

    return (
      <div>
        <SiteSelectWidget/>
        <Card className="flex flex-col justify-around gap-3 py-3 my-3">
          {DepthSensorCardBody}
          <div className="flex flex-row justify-center">
            <CardHeader className="w-auto text-h1 p-0 text-center">Auger Depth Sensors: Drilled more than 10cm?</CardHeader>
          </div>
        </Card>
      </div>
    );
};

export default DepthSensor;
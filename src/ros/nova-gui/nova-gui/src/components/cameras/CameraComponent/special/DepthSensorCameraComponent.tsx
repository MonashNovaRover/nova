import React from "react";
import { Card, CardHeader, CardBody, Chip } from "@nextui-org/react";
import { BaseCameraComponentProps } from "../CameraComponent.tsx";
import { useBifrost } from "../../../../redux/actions/bifrost/useBifrostAction.ts";
import { RootState } from "../../../../redux/RootState.ts";
import { useSelector } from "react-redux";
import { RosTopic } from "../../../../ros/topics/rosTopic.ts";
import { useEffect } from "react";
import SiteSelectWidget from "../../../science/SiteSelectWidget/SiteSelectWidget.tsx";

// This is just an updated version of the DepthSensorCameraComponent.tsx (Thanks Conner)
// Which is just a repurposed BMESensor.tsx (Thanks Someone)
// Which is just a repurposed HydroprobeWidget.tsx (Thanks Kabi)

const DepthSensor: React.FC<BaseCameraComponentProps> = (
    _: BaseCameraComponentProps
) => {
    // Accessing the Store using useSelector hook
    const auger_left_depth_hit = useSelector(
        (state: RootState) => state.augerLeftDepthStore.data
    );
    
    // Invoking Bifrost and pointing it towards TEMP_SENSOR
    const auger_left_bifrost = useBifrost({ topic: RosTopic.AUGER_LEFT_DEPTH });
    
    // Wrap with useEffect hook to only run it once
    useEffect(() => {
        // call bifrost.syncWithTopic() to initiate Realtime Updates
        auger_left_bifrost.syncWithTopic();
    }, [auger_left_bifrost]);

    // Accessing the Store using useSelector hook
    const auger_right_depth_hit = useSelector(
        (state: RootState) => state.augerRightDepthStore.data
    );
    
    // Invoking Bifrost and pointing it towards TEMP_SENSOR
    const auger_right_bifrost = useBifrost({ topic: RosTopic.AUGER_RIGHT_DEPTH });
    
    // Wrap with useEffect hook to only run it once
    useEffect(() => {
        // call bifrost.syncWithTopic() to initiate Realtime Updates
        auger_right_bifrost.syncWithTopic();
    }, [auger_right_bifrost]);

    const DepthSensorCardBody = (
        <CardBody className="flex flex-row justify-around p-0">
            <text className="w-max">Auger Left:</text>
            <Chip className={auger_left_depth_hit ? "w-min bg-success" : "w-min bg-danger"}>
                {auger_left_depth_hit ? "Yes" : "No"}
            </Chip>
            <text className="w-max">Auger Right:</text>
            <Chip className={auger_right_depth_hit ? "w-min bg-success" : "w-min bg-danger"}>
                {auger_right_depth_hit ? "Yes" : "No"}
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
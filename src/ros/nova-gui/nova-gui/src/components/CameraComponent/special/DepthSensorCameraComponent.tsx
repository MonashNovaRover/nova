import React from "react";
import { Card, CardHeader, CardBody, Chip } from "@nextui-org/react";
import { BaseCameraComponentProps } from "../CameraComponent";
// import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction.ts";
// import { RootState } from "../../redux/RootState.ts";
// import { useSelector } from "react-redux";
// import { RosTopic } from "../../ros/topics/rosTopic.ts";

// This is just a repurposed BMESensor.tsx (Thanks Someone)
// Which is just a repurposed HydroprobeWidget.tsx (Thanks Kabi)

const DepthSensor: React.FC<BaseCameraComponentProps> = (
    _: BaseCameraComponentProps
) => {
    /*
    const bifrost = useBifrost({ topic: RosTopic.DEPTH_SENSOR });
    const augerdepths = useSelector((state: RootState) => state.depthSensorStore.augerdepths);
    // Currently assuming these heights are sent as boolean values by hall effect sensor (true if drill depth > 10cm)? To Be Implemented
    

    useEffect(() => {
        bifrost.syncWithTopic();
    }, [bifrost]);
    */
    
    // For visualisation without connection to ROS
    const augerdepths = [true, false];

    // Colours differ from colours that any buttons use
    const DepthSensorCardBody = (
        <CardBody className="flex flex-row justify-around p-0">
            <text className="w-max">Auger 1:</text>
            <Chip className={augerdepths[0] ? "w-min bg-green-600" : "w-min bg-rose-600"}>
                {augerdepths[0] ? "Yes" : "No"}
            </Chip>
            <text className="w-max">Auger 2:</text>
            <Chip className={augerdepths[1] ? "w-min bg-green-600" : "w-min bg-rose-600"}>
                {augerdepths[1] ? "Yes" : "No"}
            </Chip>
        </CardBody>
    );

    return (
        <Card className="flex flex-col justify-around gap-3 py-3">
            {DepthSensorCardBody}
            <div className="flex flex-row justify-center">
                <CardHeader className="w-auto text-h1 p-0 text-center">Auger Depth Sensors: Drilled more than 10cm?</CardHeader>
            </div>
        </Card>
    );
};

export default DepthSensor;
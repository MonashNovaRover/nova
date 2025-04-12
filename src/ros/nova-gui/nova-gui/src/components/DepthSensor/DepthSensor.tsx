import React, { useEffect } from "react";
import { Card, CardHeader, CardBody, CardProps, Chip } from "@nextui-org/react";
import { OverlayedProgress } from "../OverlayedProgress/OverlayedProgress.tsx";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction.ts";
import { RootState } from "../../redux/RootState.ts";
import { useSelector } from "react-redux";
import { RosTopic } from "../../ros/topics/rosTopic.ts";

// This is just a repurposed BMESensor.tsx (Thanks Someone)
// Which is just a repurposed HydroprobeWidget.tsx (Thanks Kabi)

export interface IDepthSensorProps extends CardProps {}

const DepthSensor: React.FC<IDepthSensorProps> = (
    props: IDepthSensorProps
) => {
    /*
    const bifrost = useBifrost({ topic: RosTopic.DEPTH_SENSOR });
    const augerdepths = useSelector((state: RootState) => state.depthSensorStore.augerdepths);
    // Assume these heights are sent as measurements in millimetres like TOF sensor
    

    useEffect(() => {
        bifrost.syncWithTopic();
    }, [bifrost]);
    */
    
    // For visualisation without connection to ROS
    const augerdepths = [110, 90];

    // Colours differ from colours that any buttons use
    const DepthSensorCardBody = (
        <CardBody className="grid grid-cols-4 grid-rows-2 gap-4">
            <text className="row-start-1 w-full col-span-4 row-span-1 text-center">Drill depth {">"}10cm?</text>
            <text className="row-start-2 w-max col-span-1 row-span-1">Auger 1:</text>
            <Chip className={augerdepths[0] >= 100 ? "row-start-2 w-min col-span-1 row-span-1 bg-green-600" : "row-start-2 w-min col-span-1 row-span-1 bg-rose-500"}>
                {augerdepths[0] >= 100 ? "Yes" : "No"}
            </Chip>
            <text className="row-start-2 w-max col-span-1 row-span-1">Auger 2:</text>
            <Chip className={augerdepths[1] >= 100 ? "row-start-2 w-min col-span-1 row-span-1 bg-green-600" : "row-start-2 w-min col-span-1 row-span-1 bg-rose-500"}>
                {augerdepths[1] >= 100 ? "Yes" : "No"}
            </Chip>
        </CardBody>
    );

    return (
        <Card {...props}>
            <CardHeader className="text-h1 pb-0">Auger Depth Sensors</CardHeader>
            {DepthSensorCardBody}
        </Card>
    );
};

export default DepthSensor;
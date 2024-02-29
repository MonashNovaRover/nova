import {
    Card,
    CardHeader,
    CardBody,
    CardProps,
    Image,
} from "@nextui-org/react";
import React, { useEffect } from "react";
// import './ArmWidget.css';
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
// import { RootState } from "../../redux/RootState";
// import { useSelector } from "react-redux";
// import ArmWidgetCell, { IArmWidgetCellProps } from "./ArmWidgetCell.tsx";
import { OverlayedProgress } from "../OverlayedProgress/OverlayedProgress.tsx";
import ARMImage from "../../assets/arm-image.png";
import { RosTopic } from "../../ros/topics/rosTopic.ts";


export interface IArmWidgetProps extends CardProps { }

/** 
 * A component that displays arm telemetry.
 */

const ArmWidget: React.FC<IArmWidgetProps> = (
    props: IArmWidgetProps
) => {
    const bifrostArm = useBifrost({ topic: RosTopic.ARM });


    useEffect(() => {
        bifrostArm.syncWithTopic();
    }, [bifrostArm]);


    const armDataCardBody = (
        <CardBody className="flex flex-row gap-3">
            <div className="flex flex-col items-center justify-center">
                <div>
                    <OverlayedProgress
                        size="lg"
                        label="J1"
                        value={0.5}
                        maxValue={1}
                    ></OverlayedProgress>
                </div>
                <div>
                    <OverlayedProgress
                        size="lg"
                        label="J2"
                        value={0.5}
                        maxValue={1}
                    ></OverlayedProgress>
                </div>
                <div>
                    <OverlayedProgress
                        size="lg"
                        label="J3"
                        value={0.5}
                        maxValue={1}
                    ></OverlayedProgress>
                </div>

            </div>

            <div className="flex justify-center">
                <Image src={ARMImage} alt="Arm Image" width={200} height={200} />
            </div>

            <div className="flex flex-col items-center justify-center">
                <div>
                    <OverlayedProgress
                        size="lg"
                        label="J4"
                        value={0.5}
                        maxValue={1}
                    ></OverlayedProgress>
                </div>
                <div>
                    <OverlayedProgress
                        size="lg"
                        label="J5"
                        value={0.5}
                        maxValue={1}
                    ></OverlayedProgress>
                </div>
                <div>
                    <OverlayedProgress
                        size="lg"
                        label="J6"
                        value={0.5}
                        maxValue={1}
                    ></OverlayedProgress>
                </div>

            </div>
        </CardBody>
    );


    return (
        <Card {...props}>
            <CardHeader className="text-h1 pb-0">Arm Telemetry</CardHeader>
            {armDataCardBody}
        </Card>
    );
}

export default ArmWidget;










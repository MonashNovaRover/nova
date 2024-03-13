import {
    Card,
    CardHeader,
    CardBody,
    CardProps,
    Image,
} from "@nextui-org/react";
import React, { useEffect } from "react";

import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
// import { RootState } from "../../redux/RootState";
// import { useSelector } from "react-redux";
import ARMImage from "../../assets/arm-image.png";
import { RosTopic } from "../../ros/topics/rosTopic.ts";
import ArmWidgetCell, { IArmWidgetCellProps } from "./ArmWidgetCell.tsx";

export interface IArmWidgetProps extends CardProps { }

/** 
 * A component that displays arm telemetry.
 */

const ArmWidget: React.FC<IArmWidgetProps> = (
    props: IArmWidgetProps
) => {
    const bifrostArm = useBifrost({ topic: RosTopic.ARM_TELEMETRY });

    useEffect(() => {
        bifrostArm.syncWithTopic();
    }, [bifrostArm]);

    const armDataCardBody = (
        <CardBody className="grid auto-cols-fr grid-flow-col gap-3">
            <div className="flex flex-col justify-center gap-3">
                <ArmWidgetCell
                    jointValue={0.5}
                    label={<>J1</>}
                />
                <ArmWidgetCell
                    jointValue={0.5}
                    label={<>J2</>}
                />
                <ArmWidgetCell
                    jointValue={0.5}
                    label={<>J3</>}
                />
            </div>

            <div className="flex justify-center">
                <Image src={ARMImage} alt="Arm Image" width={200} height={200} />
            </div>

            <div className="flex flex-col justify-center gap-3">
                <ArmWidgetCell
                    jointValue={0.5}
                    label={<>J4</>}
                />
                <ArmWidgetCell
                    jointValue={0.5}
                    label={<>J5</>}
                />
                <ArmWidgetCell
                    jointValue={0.5}
                    label={<>J6</>}
                />
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










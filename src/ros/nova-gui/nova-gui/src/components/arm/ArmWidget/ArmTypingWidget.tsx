import {
    Card,
    CardHeader,
    CardBody,
    CardProps,
} from "@nextui-org/react";
import React, { useEffect } from "react";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RootState } from "../../../redux/RootState.ts";
import { useSelector } from "react-redux";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";
import ArmWidgetCell from "./ArmWidgetCell.tsx";

export interface IArmTypingWidgetProps extends CardProps { }

/** 
 * A component that displays arm telemetry.
 */

const ArmTypingWidget: React.FC<IArmTypingWidgetProps> = (
    props: IArmTypingWidgetProps
) => {
    const CURRENT_FACTOR = 10
    const bifrostArm = useBifrost({ topic: RosTopic.ARM_TELEMETRY });

    const jointValues = useSelector((state: RootState) => state.armTelemetryStore.arm_motors);
    const jointValuesCurrents = jointValues.map((j) => Math.abs(j.current/CURRENT_FACTOR));

    useEffect(() => {
        bifrostArm.syncWithTopic();
    }, [bifrostArm]);

    const armDataCardBody = (
        <CardBody className="gap-2">
            <div className="grid grid-cols-2 gap-1">
                <div className="flex flex-col justify-center gap-1">
                    <ArmWidgetCell
                        jointValue={jointValuesCurrents[0]}
                        label={<>J1</>}
                    />
                    <ArmWidgetCell
                        jointValue={jointValuesCurrents[1]}
                        label={<>J2</>}
                    />
                    <ArmWidgetCell
                        jointValue={jointValuesCurrents[2]}
                        label={<>J3</>}
                    />
                </div>
                <div className="flex flex-col justify-center gap-1">
                    <ArmWidgetCell
                        jointValue={jointValuesCurrents[3]}
                        label={<>J4</>}
                    />
                    <ArmWidgetCell
                        jointValue={jointValuesCurrents[4]}
                        label={<>J5</>}
                    />
                    <ArmWidgetCell
                        jointValue={jointValuesCurrents[5]}
                        label={<>J6</>}
                    />
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

export default ArmTypingWidget;


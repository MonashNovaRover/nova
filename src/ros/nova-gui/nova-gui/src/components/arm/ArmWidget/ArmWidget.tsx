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

export interface IArmWidgetProps extends CardProps { }

/** 
 * A component that displays arm telemetry.
 */

const PROGRESS_MAX_VELOCITY = 0.25;

const ArmWidget: React.FC<IArmWidgetProps> = (
    props: IArmWidgetProps
) => {
    const CURRENT_FACTOR = 10
    const bifrostArm = useBifrost({ topic: RosTopic.ARM_TELEMETRY });

    const jointValues = useSelector((state: RootState) => state.armTelemetryStore.arm_motors);
    const jointValuesCurrents = jointValues.map((j) => Math.abs(j.current/CURRENT_FACTOR));

    useEffect(() => {
        bifrostArm.syncWithTopic();
    }, [bifrostArm]);

    const armDataCardBody = (
        <CardBody className="grid auto-cols-fr grid-flow-col gap-2 p-2">
            <div className="flex flex-col justify-center gap-2">
                <ArmWidgetCell
                    jointCurrent={jointValuesCurrents[0]}
                    jointVelocity={0}
                    progressMaxVelocity={PROGRESS_MAX_VELOCITY}
                    label={<>J1</>}
                />
                <ArmWidgetCell
                    jointCurrent={jointValuesCurrents[3]}
                    jointVelocity={0}
                    progressMaxVelocity={PROGRESS_MAX_VELOCITY}
                    label={<>J4</>}
                />
            </div>
            <div className="flex flex-col justify-center gap-2">
                <ArmWidgetCell
                    jointCurrent={jointValuesCurrents[1]}
                    jointVelocity={0}
                    progressMaxVelocity={PROGRESS_MAX_VELOCITY}
                    label={<>J2</>}
                />
                <ArmWidgetCell
                    jointCurrent={jointValuesCurrents[4]}
                    jointVelocity={0}
                    progressMaxVelocity={PROGRESS_MAX_VELOCITY}
                    label={<>J5</>}
                />
            </div>
            <div className="flex flex-col justify-center gap-2">
                <ArmWidgetCell
                    jointCurrent={jointValuesCurrents[2]}
                    jointVelocity={0}
                    progressMaxVelocity={PROGRESS_MAX_VELOCITY}
                    label={<>J3</>}
                />
                <ArmWidgetCell
                    jointCurrent={jointValuesCurrents[5]}
                    jointVelocity={0}
                    progressMaxVelocity={PROGRESS_MAX_VELOCITY}
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


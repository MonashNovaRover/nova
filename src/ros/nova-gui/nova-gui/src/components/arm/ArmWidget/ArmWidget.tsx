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
import {getJointEffort, getJointVelocity} from "../../../utils.ts";
import {IRosSensorMsgsJointState} from "../../../ros/rosTypes.ts";

export interface IArmWidgetProps extends CardProps { }

/** 
 * A component that displays arm telemetry.
 */

const CURRENT_FACTOR = 0.15;
const PROGRESS_MAX_VELOCITY = 1.05;

const getProcessedJointCurrent = (joint: string, jointState: IRosSensorMsgsJointState) => Math.abs(getJointEffort(joint, jointState)) / CURRENT_FACTOR;
const getProcessedJointVelocity = (joint: string, jointState: IRosSensorMsgsJointState) => Math.abs(getJointVelocity(joint, jointState));

const ArmWidget: React.FC<IArmWidgetProps> = (
    props: IArmWidgetProps
) => {

    const bifrostArm = useBifrost({ topic: RosTopic.ARM_TELEMETRY_JOINT_STATES });
    const jointStates = useSelector((state: RootState) => state.armTelemetryJointStateStore);

    useEffect(() => {
        bifrostArm.syncWithTopic();
    }, [bifrostArm]);

    const armDataCardBody = (
        <CardBody className="grid auto-cols-fr grid-flow-col gap-2 p-2">
            <div className="flex flex-col justify-center gap-2">
                <ArmWidgetCell
                    jointCurrent={getProcessedJointCurrent("j1", jointStates)}
                    jointVelocity={getProcessedJointVelocity("j1", jointStates)}
                    progressMaxVelocity={PROGRESS_MAX_VELOCITY}
                    label={<>J1</>}
                />
                <ArmWidgetCell
                    jointCurrent={getProcessedJointCurrent("j4", jointStates)}
                    jointVelocity={getProcessedJointVelocity("j4", jointStates)}
                    progressMaxVelocity={PROGRESS_MAX_VELOCITY}
                    label={<>J4</>}
                />
            </div>
            <div className="flex flex-col justify-center gap-2">
                <ArmWidgetCell
                    jointCurrent={getProcessedJointCurrent("j2", jointStates)}
                    jointVelocity={getProcessedJointVelocity("j2", jointStates)}
                    progressMaxVelocity={PROGRESS_MAX_VELOCITY}
                    label={<>J2</>}
                />
                <ArmWidgetCell
                    jointCurrent={getProcessedJointCurrent("j5", jointStates)}
                    jointVelocity={getProcessedJointVelocity("j5", jointStates)}
                    progressMaxVelocity={PROGRESS_MAX_VELOCITY}
                    label={<>PITCH</>}
                />
            </div>
            <div className="flex flex-col justify-center gap-2">
                <ArmWidgetCell
                    jointCurrent={getProcessedJointCurrent("j3", jointStates)}
                    jointVelocity={getProcessedJointVelocity("j3", jointStates)}
                    progressMaxVelocity={PROGRESS_MAX_VELOCITY}
                    label={<>J3</>}
                />
                <ArmWidgetCell
                    jointCurrent={getProcessedJointCurrent("j6", jointStates)}
                    jointVelocity={getProcessedJointVelocity("j6", jointStates)}
                    progressMaxVelocity={PROGRESS_MAX_VELOCITY}
                    label={<>ROLL</>}
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


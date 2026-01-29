import {
  Card,
  CardHeader,
  CardBody,
  CardProps,
  Image,
} from "@nextui-org/react";
import React, { useEffect } from "react";
import '../DriveModeWidget/DriveWidget.css';
import './WheelTelemetryWidget.css';
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RootState } from "../../../redux/RootState.ts";
import { useSelector } from "react-redux";
import { ChevronUp } from "react-feather";
import WheelTelemetryWidgetCell, { IWheelTelemetryWidgetCellProps } from "./WheelTelemetryWidgetCell.tsx";
import RoverTopDownImage from "../../../assets/rover-top-down-dark.png";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";

// Properties for the WheelTelemetryWidget component.
export interface IDriveWheelWidgetProps extends CardProps {
  hideImage?: boolean
}

/**
 * A component that displays wheel telemetry.
 */
const WheelTelemetryWidget: React.FC<IDriveWheelWidgetProps> = (
  props: IDriveWheelWidgetProps
) => {
  const pivotEffortMultiplier = 1;
  const wheelEffortMultiplier = 1;

  const bifrost = useBifrost({ topic: RosTopic.DRIVE_JOINT_STATES });

  const names = useSelector((state: RootState) => state.driveJointStateStore.name);
  const efforts = useSelector((state: RootState) => state.driveJointStateStore.effort);

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const getEffort = (name: string) => {
    const index = names.indexOf(name)
    if (index >= 0) {
      return efforts[index];
    }
    else {
      return -1;
    }
  }

  // Props to give to each cell. The order of names matches the order of SingleTelemetry entries in the Telemetry arrays
  const cellProps: IWheelTelemetryWidgetCellProps[] = [
    {
      label: <>Front Left</>,
      className: "row-start-1 col-start-1",
      wheelValue: getEffort("flw"),
      pivotValue: getEffort("flp"),
    } as IWheelTelemetryWidgetCellProps,
    {
      label: <>Back Left</>,
      className: "row-start-2 col-start-1",
      wheelValue: getEffort("blw"),
      pivotValue: getEffort("blp"),
    } as IWheelTelemetryWidgetCellProps,
    {
      label: <>Back Right</>,
      className: "row-start-2 col-start-3",
      wheelValue: getEffort("brw"),
      pivotValue: getEffort("brp"),
    } as IWheelTelemetryWidgetCellProps,
    {
      label: <>Front Right</>,
      className: "row-start-1 col-start-3",
      wheelValue: getEffort("frw"),
      pivotValue: getEffort("frp"),
    } as IWheelTelemetryWidgetCellProps,
  ];

  // Bottom Section of the component for displaying wheel and pivot telemetry
  const wheelDataCardBody = (
    <CardBody className="flex flex-col gap-3">
      <div className="DriveWheelWidgetGrid gap-2">
        <div className="DriveWheelWidgetGridImage row-end-3 row-start-1 flex justify-center flex-col align-middle">
          <span className="text-default-300 text-opacity-80">
            <ChevronUp size={20}></ChevronUp>
          </span>
          {!props.hideImage && <Image className="mx-2" radius="none" src={RoverTopDownImage}></Image>}
        </div>
        {cellProps.map((cellProp, index) => (
          <WheelTelemetryWidgetCell
            {...cellProp}
            key={index}
            wheelValue={cellProp.wheelValue * wheelEffortMultiplier}
            pivotValue={cellProp.pivotValue * pivotEffortMultiplier}
          />
        ))}
      </div>
    </CardBody>
  );

  // Finally, put the body into a card
  return (
    <Card {...props}>
      <CardHeader className="text-h1 pb-0">Wheel Telemetry</CardHeader>
      {wheelDataCardBody}
    </Card>
  );
};

export default WheelTelemetryWidget;

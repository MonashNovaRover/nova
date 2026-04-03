import {
  Card,
  CardHeader,
  CardBody,
  CardProps,
  Image,
} from "@nextui-org/react";
import React, { useEffect } from "react";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RootState } from "../../../redux/RootState.ts";
import { useSelector } from "react-redux";
import WheelTelemetryWidgetCell from "./WheelTelemetryWidgetCell.tsx";
import RoverTopDownImage from "../../../assets/rover-top-down.png";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";
import {PIVOT_CURRENT_MAX, WHEEL_CURRENT_MAX} from "../../../constants.ts";
import {getJointEffort} from "../../../utils.ts";

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

  const bifrost = useBifrost({ topic: RosTopic.DRIVE_JOINT_STATES });

  const jointState = useSelector((state: RootState) => state.driveJointStateStore);

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  // Props to give to each cell. The order of names matches the order of SingleTelemetry entries in the Telemetry arrays
  // const cellProps: IWheelTelemetryWidgetCellProps[] = [
  //   {
  //     label: <>Front Left</>,
  //     className: "row-start-1 col-start-1",
  //     wheelName: "flw",
  //     pivotName: "flp",
  //   } as IWheelTelemetryWidgetCellProps,
  //   {
  //     label: <>Back Left</>,
  //     className: "row-start-2 col-start-1",
  //     wheelName: "blw",
  //     pivotName: "blp",
  //   } as IWheelTelemetryWidgetCellProps,
  //   {
  //     label: <>Back Right</>,
  //     className: "row-start-2 col-start-3",
  //     wheelName: "brw",
  //     pivotName: "brp",
  //   } as IWheelTelemetryWidgetCellProps,
  //   {
  //     label: <>Front Right</>,
  //     className: "row-start-1 col-start-3",
  //     wheelName: "frw",
  //     pivotName: "frp",
  //   } as IWheelTelemetryWidgetCellProps,
  // ];

  // Bottom Section of the component for displaying wheel and pivot telemetry
  const wheelDataCardBody = (
    <CardBody className="grid grid-cols-3 gap-2 justify-items-center place-items-center">
      <div className="flex flex-col col-span-1 gap-3 w-full">
        <WheelTelemetryWidgetCell
          label={<>Front Left</>}
          wheelName="flw"
          pivotName="flp"
          wheelValue={Math.abs(getJointEffort("flw", jointState)) / WHEEL_CURRENT_MAX}
          pivotValue={Math.abs(getJointEffort("flp", jointState)) / PIVOT_CURRENT_MAX}
        />
        <WheelTelemetryWidgetCell
          label={<>Back Left</>}
          wheelName="blw"
          pivotName="blp"
          wheelValue={Math.abs(getJointEffort("blw", jointState)) / WHEEL_CURRENT_MAX}
          pivotValue={Math.abs(getJointEffort("blp", jointState)) / PIVOT_CURRENT_MAX}
        />
      </div>

      <Image className="" radius="none" src={RoverTopDownImage}></Image>

      <div className="flex flex-col col-span-1 gap-3 w-full">
        <WheelTelemetryWidgetCell
          label={<>Front Right</>}
          wheelName="frw"
          pivotName="frp"
          wheelValue={Math.abs(getJointEffort("frw", jointState)) / WHEEL_CURRENT_MAX}
          pivotValue={Math.abs(getJointEffort("frp", jointState)) / PIVOT_CURRENT_MAX}
        />
        <WheelTelemetryWidgetCell
          label={<>Back Right</>}
          wheelName="brw"
          pivotName="brp"
          wheelValue={Math.abs(getJointEffort("brw", jointState)) / WHEEL_CURRENT_MAX}
          pivotValue={Math.abs(getJointEffort("brp", jointState)) / PIVOT_CURRENT_MAX}
        />
      </div>

      {/*<div className="gap-2">*/}
      {/*  <div className="row-end-3 row-start-1 flex justify-center flex-col align-middle">*/}
      {/*    <Image className="mx-2 rotate-180" radius="none" src={RoverTopDownImage}></Image>*/}
      {/*  </div>*/}
      {/*  {cellProps.map((cellProp, index) => (*/}
      {/*    <WheelTelemetryWidgetCell*/}
      {/*      {...cellProp}*/}
      {/*      key={index}*/}
      {/*      wheelValue={Math.abs(getJointEffort(cellProp.wheelName, jointState)) / WHEEL_CURRENT_MAX}*/}
      {/*      pivotValue={Math.abs(getJointEffort(cellProp.pivotName, jointState)) / PIVOT_CURRENT_MAX}*/}
      {/*    />*/}
      {/*  ))}*/}
      {/*</div>*/}
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

import {
  Card,
  CardHeader,
  CardBody,
  CardProps
} from "@nextui-org/react";
import React, { ReactNode } from "react";
import '../DriveModeWidget/DriveWidget.css';
import './DriveWheelWidget.css';
import { DriveProgress } from "../DriveModeWidget/DriveProgress.tsx";

// Properties for the DriveModeWidget component.
export interface IDriveWheelWidgetProps extends CardProps {

}

// Properties for the DriveWidgetWheelData component.
export interface IDriveWheelWidgetCellProps {
  wheelValue: number,
  pivotValue: number,
  name: ReactNode
}

/**
 * A component for displaying the wheel data for the DriveModeWidget
 */
const DriveWheelWidgetCell: React.FC<IDriveWheelWidgetCellProps> = (props: IDriveWheelWidgetCellProps) => {
  const wheelProgress = (
    <DriveProgress size="lg" value={props.wheelValue} maxValue={1} aria-label="Wheel Amount">
      <div className="grid grid-flow-col gap-3 auto-cols-fr text-small">
        <span>WHEEL</span>
        <span>{`${(props.wheelValue * 100).toFixed(0)}%`}</span>
      </div>
    </DriveProgress>
  );

  const pivotProgress = (
    <DriveProgress size="lg" value={props.pivotValue} maxValue={1} aria-label="Pivot Amount">
      <div className="grid grid-flow-col gap-3 auto-cols-fr text-small">
        <span>PIVOT</span>
        <span>{`${(props.pivotValue * 100).toFixed(0)}%`}</span>
      </div>
    </DriveProgress>
  );

  return <Card shadow="sm" className="bg-content2">
    <CardBody className="grid grid-rows-1 grid-cols-4 content-center place-content-stretch">
      <div className="flex flex-col justify-center">
        <span className="align-middle">{props.name}</span>
      </div>
      <div className="flex flex-col gap-2 col-span-3">
        {wheelProgress}
        {pivotProgress}
      </div>
    </CardBody>
  </Card>
}

/**
 * A component that displays data for driving the rover.
 */
const DriveWheelWidget: React.FC<IDriveWheelWidgetProps> = (props: IDriveWheelWidgetProps) => {
  // Bottom Section of the component for displaying wheel motor telemetry
  const wheelDataCardBody = (
    <CardBody className="flex flex-col gap-3">
      <div className="grid grid-rows-2 grid-cols-2 gap-2">
        <DriveWheelWidgetCell wheelValue={0.2} pivotValue={0.5} name={<>Front<br/>Left</>}/>
        <DriveWheelWidgetCell wheelValue={0.8} pivotValue={0.5} name={<>Front<br/>Right</>}/>



        <DriveWheelWidgetCell wheelValue={0.2} pivotValue={0.7} name={<>Back<br/>Left</>}/>
        <DriveWheelWidgetCell wheelValue={0.5} pivotValue={0.1} name={<>Back<br/>Right</>}/>
      </div>
    </CardBody>
  );

  // Finally, put the two card bodies into a card
  return (
    <Card {...props} >
      <CardHeader className="text-h1">
        Wheel Data
      </CardHeader>
      {wheelDataCardBody}
    </Card>
  )
};

export default DriveWheelWidget;
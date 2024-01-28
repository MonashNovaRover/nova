import {
  Card,
  CardBody,
  CardProps
} from "@nextui-org/react";
import React, {ReactNode} from "react";
import '../DriveModeWidget/DriveWidget.css';
import './WheelTelemetryWidget.css';
import { DriveProgress } from "../DriveSpeedWidget/DriveProgress.tsx";

// Properties for the DriveWidgetWheelData component.
export interface IWheelTelemetryWidgetCellProps extends CardProps {
  wheelValue: number,
  pivotValue: number,
  label: ReactNode
}

/**
 * A component for displaying telemetry for a single wheel
 */
const WheelTelemetryWidgetCell: React.FC<IWheelTelemetryWidgetCellProps> = (props: IWheelTelemetryWidgetCellProps) => {

  const wheelProgress = (
    <DriveProgress size="lg"
                   value={props.wheelValue}
                   maxValue={1}
                   aria-label="Wheel Amount"
                   autoColor={true}
                   disableAnimation={true}>
      <div className="grid grid-flow-col gap-3 auto-cols-fr text-small">
        <span>WHEEL</span>
        <span>{`${(props.wheelValue * 100).toFixed(0)}%`}</span>
      </div>
    </DriveProgress>
  );

  const pivotProgress = (
    <DriveProgress size="lg"
                   value={props.pivotValue}
                   maxValue={1}
                   aria-label="Pivot Amount"
                   autoColor={true}
                   disableAnimation={true}>
      <div className="grid grid-flow-col gap-3 auto-cols-fr text-small">
        <span>PIVOT</span>
        <span>{`${(props.pivotValue * 100).toFixed(0)}%`}</span>
      </div>
    </DriveProgress>
  );

  const label = (
    <span className="text-sm uppercase tracking-widest text-center text-default-900 text-opacity-80">
      {props.label}
    </span>
  )

  return <Card shadow="sm" {...props} >
    <CardBody className="pt-1 flex gap-1 font-semibold flex-col content-center bg-content2">
      {label}
      <div className="flex flex-col gap-2 content-center">
        {wheelProgress}
        {pivotProgress}
      </div>
    </CardBody>
  </Card>
}

export default WheelTelemetryWidgetCell;
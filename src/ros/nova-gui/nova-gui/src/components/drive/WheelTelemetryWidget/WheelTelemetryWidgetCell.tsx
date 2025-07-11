import {
  Card,
  CardBody,
  CardProps
} from "@nextui-org/react";
import React, {ReactNode} from "react";
import '../DriveModeWidget/DriveWidget.css';
import './WheelTelemetryWidget.css';
import { SubCardLabel } from "../../shared/components/Labels.tsx";
import { OverlayedProgress } from "../../shared/components/OverlayedProgress/OverlayedProgress.tsx";

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
    <OverlayedProgress size="lg"
                       value={props.wheelValue}
                       maxValue={1}
                       aria-label="Wheel Amount"
                       autoColor={true}
                       disableAnimation={false}>
      <div className="grid grid-flow-col gap-3 auto-cols-fr text-small">
        <span>WHEEL</span>
        <span>{`${(props.wheelValue * 100).toFixed(0)}%`}</span>
      </div>
    </OverlayedProgress>
  );

  const pivotProgress = (
    <OverlayedProgress size="lg"
                       value={props.pivotValue}
                       maxValue={1}
                       aria-label="Pivot Amount"
                       autoColor={true}
                       disableAnimation={false}>
      <div className="grid grid-flow-col gap-3 auto-cols-fr text-small">
        <span>PIVOT</span>
        <span>{`${(props.pivotValue * 100).toFixed(0)}%`}</span>
      </div>
    </OverlayedProgress>
  );



  return <Card shadow="sm" {...props} >
    <CardBody className="pt-1 flex gap-1 flex-col content-center bg-content2">
      <SubCardLabel>{props.label}</SubCardLabel>
      <div className="flex flex-col gap-2 content-center">
        {wheelProgress}
        {pivotProgress}
      </div>
    </CardBody>
  </Card>
}

export default WheelTelemetryWidgetCell;
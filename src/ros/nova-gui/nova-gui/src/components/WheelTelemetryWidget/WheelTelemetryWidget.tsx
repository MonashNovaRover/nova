import {
  Card,
  CardHeader,
  CardBody,
  CardProps
} from "@nextui-org/react";
import React, {ReactNode, useEffect} from "react";
import '../DriveModeWidget/DriveWidget.css';
import './WheelTelemetryWidget.css';
import { DriveProgress } from "../DriveModeWidget/DriveProgress.tsx";
import {useBifrost} from "../../redux/actions/useBifrostAction";
import {RosTopics} from "../../ros/rosTopics";
import {RootState} from "../../redux/RootState";
import {useSelector} from "react-redux";

// Properties for the DriveModeWidget component.
export interface IDriveWheelWidgetProps extends CardProps {

}

// Properties for the DriveWidgetWheelData component.
export interface IWheelTelemetryWidgetCellProps {
  wheelValue: number,
  pivotValue: number,
  name: ReactNode
}

/**
 * A component for displaying the wheel data for the DriveModeWidget
 */
const WheelTelemetryWidgetCell: React.FC<IWheelTelemetryWidgetCellProps> = (props: IWheelTelemetryWidgetCellProps) => {
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

  console.log(`${props.name} has ${props.pivotValue} and ${props.wheelValue}`);

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
const WheelTelemetryWidget: React.FC<IDriveWheelWidgetProps> = (props: IDriveWheelWidgetProps) => {
  const bifrost = useBifrost(RosTopics.TELEMETRTY);
  const pivotCurrents = useSelector((state: RootState) => state.telemetryStore.pivots.map(p => p.q_current));
  const wheelCurrents = useSelector((state: RootState) => state.telemetryStore.wheels.map(w => w.q_current));

  useEffect(() => {
    bifrost.syncWithRover();
  }, [bifrost]);

  // The max current of a motor in the wheel, considered 100% on the progress bar, in amps
  const currentMax = 0.45;

  // Names to give to each cell
  const cellNames = [
    <>Front<br/>Left</>,
    <>Front<br/>Right</>,
    <>Back<br/>Left</>,
    <>Back<br/>Right</>
  ]

  // Bottom Section of the component for displaying wheel motor telemetry
  const wheelDataCardBody = (
    <CardBody className="flex flex-col gap-3">
      <div className="grid grid-rows-2 grid-cols-2 gap-2">
        {cellNames.map((cellName, index) => (
          <WheelTelemetryWidgetCell
            wheelValue={wheelCurrents[index] / currentMax}
            pivotValue={pivotCurrents[index] / currentMax}
            name={cellName}
          />
        ))}
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

export default WheelTelemetryWidget;
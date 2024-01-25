import {
  Card,
  CardHeader,
  CardBody,
  CardProps,
  Image
} from "@nextui-org/react";
import React, {ReactNode, useEffect} from "react";
import '../DriveModeWidget/DriveWidget.css';
import './WheelTelemetryWidget.css';
import { DriveProgress } from "../DriveSpeedWidget/DriveProgress.tsx";
import {useBifrost} from "../../redux/actions/useBifrostAction";
import {RosTopics} from "../../ros/rosTopics";
import {RootState} from "../../redux/RootState";
import {useSelector} from "react-redux";
import RoverTopDownImage from "../../assets/rover-top-down.png";
import {ChevronUp} from "react-feather";

// Properties for the DriveModeWidget component.
export interface IDriveWheelWidgetProps extends CardProps {

}

// Properties for the DriveWidgetWheelData component.
export interface IWheelTelemetryWidgetCellProps extends CardProps {
  wheelValue: number,
  pivotValue: number,
  label: ReactNode
}

/**
 * A component for displaying the wheel data for the DriveModeWidget
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
    <span className="text-sm uppercase tracking-widest text-center text-default-300 text-opacity-80">
      {props.label}
    </span>
  )

  return <Card shadow="sm" className="bg-content2" {...props}>
    <CardBody className="pt-1 flex gap-1 font-semibold flex-col content-center">
      {label}
      <div className="flex flex-col gap-2 content-center">
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
  const pivotCurrents = useSelector((state: RootState) => state.telemetryStore.pivots
      .map(p => p.q_current));
  const wheelCurrents = useSelector((state: RootState) => state.telemetryStore.wheels
      .map(w => w.q_current));

  useEffect(() => {
    bifrost.syncWithRover();
  }, [bifrost]);

  // The max current of a motor in the wheel, considered 100% on the progress bar, in amps
  const currentMax = 0.45;

  // Props to give to each cell. The order of names matches the order of SingleTelemetry entries in the Telemetry arrays
  const cellProps : IWheelTelemetryWidgetCellProps[] = [
    {
      label: <>Front Left</>,
      className: "row-start-1 col-start-1",
    } as IWheelTelemetryWidgetCellProps,
    {
      label: <>Back Left</>,
      className: "row-start-2 col-start-1",
    } as IWheelTelemetryWidgetCellProps,
    {
      label: <>Back Right</>,
      className: "row-start-2 col-start-3",
    } as IWheelTelemetryWidgetCellProps,
    {
      label: <>Front Right</>,
      className: "row-start-1 col-start-3",
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
          <Image className="" src={RoverTopDownImage}></Image>
        </div>
        {cellProps.map((cellProp, index) => (
            <WheelTelemetryWidgetCell
                key={index}
                {...cellProp}
                wheelValue={wheelCurrents[index] / currentMax}
                pivotValue={pivotCurrents[index] / currentMax}
            />
        ))}
      </div>
    </CardBody>
  );

  // Finally, put the body into a card
  return (
      <Card {...props} >
        <CardHeader className="text-h1">
          Wheel Telemetry
        </CardHeader>
        {wheelDataCardBody}
      </Card>
  )
};

export default WheelTelemetryWidget;
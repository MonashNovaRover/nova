import {
  Card,
  CardHeader,
  CardBody,
  Divider,
  CardProps
} from "@nextui-org/react";
import { ReactNode } from "react";
import './DriveWidget.css';
import { DriveModeButton } from "./DriveModeButton";
import {DriveProgress} from "./DriveProgress";
import { driveModes } from "./DriveModeDisplayData";

// Properties for the DriveWidget component.
export interface IDriveWidgetProps extends CardProps {
  driveModeIndex: string,
  handleDriveModeSelectChange: (e: React.ChangeEvent<HTMLSelectElement>) => void,
  setDriveModeIndex: (content: string) => void
}

// Properties for the DriveWidgetWheelData component.
export interface IDriveWidgetWheelDataProps {
  wheelValue: number,
  pivotValue: number,
  name: ReactNode
}

/**
 * A component for displaying the wheel data for the DriveWidget
 */
const DriveWidgetWheelData: React.FC<IDriveWidgetWheelDataProps> = (props: IDriveWidgetWheelDataProps) => {
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
const DriveWidget: React.FC<IDriveWidgetProps> = (props: IDriveWidgetProps) => {
  // Text to display for the current drive mode
  const driveModeLabelText = driveModes[+props.driveModeIndex].name;

  // Helper function for creating labels in the driveInfoCardBody
  const createLabelCell = (content: ReactNode) => (
    <div className="flex flex-col justify-end items-center">{content}</div>
  )

  // The top half of the card, containing drive mode info data
  const driveInfoCardBody = (
    <CardBody className="grid DriveWidgetTopGrid gap-y-2.5 gap-x-5">
      { createLabelCell(<>Average Velocity</>) }
      { createLabelCell(<>{driveModeLabelText} Mode</>) }
      { createLabelCell(<>Speed Control</>) }

      <div>
        <DriveProgress size="lg" value={0.314} maxValue={1} aria-label="Average Velocity">
          31.4%
        </DriveProgress>
      </div>

      <div className="flex gap-3 items-center justify-center">
        {driveModes.map((mode, index) => (
          <DriveModeButton
            key={index}
            driveModeData={mode}
            tooltipPlacement="bottom"
            driveModeActive={props.driveModeIndex === `${index}`}
            onPress={() => props.setDriveModeIndex(`${index}`)}
            iconClassName="w-5 h-5"
          />
        ))}
      </div>

      <div>
        <DriveProgress size="lg" value={0.8} maxValue={1} aria-label="Speed Control">
          80%
        </DriveProgress>
      </div>
    </CardBody>
  );

  // Bottom Section of the component for displaying wheel motor telemetry
  const wheelDataCardBody = (
    <CardBody className="flex flex-col gap-3">
      <div className="grid grid-cols-2 grid-rows-2 gap-2">
        <DriveWidgetWheelData wheelValue={0.2} pivotValue={0.5} name={<>Front<br/>Left</>}/>
        <DriveWidgetWheelData wheelValue={0.8} pivotValue={0.5} name={<>Front<br/>Right</>}/>
        <DriveWidgetWheelData wheelValue={0.2} pivotValue={0.7} name={<>Back<br/>Left</>}/>
        <DriveWidgetWheelData wheelValue={0.7} pivotValue={0.1} name={<>Back<br/>Right</>}/>
      </div>
    </CardBody>
  );

  // Finally, put the two card bodies into a card
  return (
    <Card {...props} >
      <CardHeader className="text-h1">
        Drive
      </CardHeader>
      {driveInfoCardBody}
      <Divider/>
      {wheelDataCardBody}
    </Card>
  )
};

export default DriveWidget;

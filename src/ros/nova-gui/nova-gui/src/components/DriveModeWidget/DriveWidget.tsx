import {
  Card,
  CardHeader,
  CardBody,
  CardProps
} from "@nextui-org/react";
import { ReactNode } from "react";
import './DriveWidget.css';
import {DriveProgress} from "./DriveProgress";

// Properties for the DriveModeWidget component.
export interface IDriveWidgetProps extends CardProps {
  driveModeIndex: string,
  handleDriveModeSelectChange: (e: React.ChangeEvent<HTMLSelectElement>) => void,
  setDriveModeIndex: (content: string) => void
}

/**
 * A component that displays data for driving the rover.
 */
const DriveWidget: React.FC<IDriveWidgetProps> = (props: IDriveWidgetProps) => {
  // Text to display for the current drive mode
  // const driveModeLabelText = driveModes[+props.driveModeIndex].name;

  // Helper function for creating labels in the driveInfoCardBody
  const createLabelCell = (content: ReactNode) => (
    <div className="flex flex-col justify-end items-center">{content}</div>
  )

  // The top half of the card, containing drive mode info data
  const driveInfoCardBody = (
    <CardBody className="grid DriveWidgetTopGrid gap-y-2.5 gap-x-5">
      { createLabelCell(<>Average Velocity</>) }
      { createLabelCell(<>Speed Control</>) }

      <div>
        <DriveProgress size="lg" value={0.314} maxValue={1} aria-label="Average Velocity">
          31.4%
        </DriveProgress>
      </div>

      <div>
        <DriveProgress size="lg" value={0.8} maxValue={1} aria-label="Speed Control">
          80%
        </DriveProgress>
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
    </Card>
  )
};

export default DriveWidget;

import {
  Card,
  CardHeader,
  CardBody,
  CardProps
} from "@nextui-org/react";
import {ReactNode, useEffect} from "react";
import '../DriveModeWidget/DriveWidget.css';
import {DriveProgress} from "./DriveProgress";
import {useBifrost} from "../../redux/actions/useBifrostAction";
import {RosTopics} from "../../ros/rosTopics";
import {RootState} from "../../redux/RootState";
import {useSelector} from "react-redux";

// Properties for the DriveModeWidget component.
export interface IDriveWidgetProps extends CardProps {

}

/**
 * A component that displays data for driving the rover.
 */
const DriveSpeedWidget: React.FC<IDriveWidgetProps> = (props: IDriveWidgetProps) => {
  const bifrostDrive = useBifrost(RosTopics.DRIVE_INFO);
  const driveMultiplier = useSelector((state: RootState) => state.driveStore.multiplier);

  const bifrostTelemetry = useBifrost(RosTopics.TELEMETRY);
  const wheelsData = useSelector((state: RootState) =>
    state.telemetryStore.wheels
  );
  const averageWheelAngularVelocity = wheelsData
    .map((w) => w.rotor_velocity)
    .reduce((v: number, acc: number) => v + acc, 0)
  / 4

  // This is the value for 100% from the original wombatx GUI
  const maxAverageWheelAngularVelocity = 0.35;

  useEffect(() => {
    bifrostDrive.syncWithRover();
  }, [bifrostDrive, bifrostTelemetry]);

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
        <DriveProgress size="lg" aria-label="Average Velocity"
                       value={Math.abs(averageWheelAngularVelocity)}
                       maxValue={maxAverageWheelAngularVelocity}>
          {averageWheelAngularVelocity.toFixed(2)} rad/s
        </DriveProgress>
      </div>

      <div>
        <DriveProgress size="lg" value={driveMultiplier} maxValue={1} aria-label="Speed Control">
          {(driveMultiplier * 100).toFixed(0)} %
        </DriveProgress>
      </div>
    </CardBody>
  );

  // Finally, put the body into a card
  return (
    <Card {...props} >
      <CardHeader className="text-h1 pb-0">
        Drive Speed
      </CardHeader>
      {driveInfoCardBody}
    </Card>
  )
};

export default DriveSpeedWidget;

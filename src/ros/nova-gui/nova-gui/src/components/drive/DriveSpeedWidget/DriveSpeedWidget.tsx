import { Card, CardHeader, CardBody, CardProps } from "@nextui-org/react";
import { ReactNode, useEffect } from "react";
import "../DriveModeWidget/DriveWidget.css";
import { OverlayedProgress } from "../../shared/components/OverlayedProgress/OverlayedProgress.tsx";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { RootState } from "../../../redux/RootState.ts";
import { useSelector } from "react-redux";
import { DRIVE_VEL_MAX } from "../../../constants.ts";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";

// Properties for the DriveModeWidget component.
export interface IDriveWidgetProps extends CardProps {}

/**
 * A component that displays data for driving the rover.
 */
const DriveSpeedWidget: React.FC<IDriveWidgetProps> = (
  props: IDriveWidgetProps
) => {
  const bifrostDrive = useBifrost({ topic: RosTopic.DRIVE_INFO });
  const driveMultiplier = useSelector(
    (state: RootState) => state.driveStore.multiplier
  );

  const bifrostTelemetry = useBifrost({ topic: RosTopic.DRIVE_TELEMETRY });
  const wheelsData = useSelector(
    (state: RootState) => state.driveTelemetryStore.wheels
  );
  const averageWheelAngularVelocity =
    wheelsData
      .map((w) => Math.abs(w.rotor_velocity))
      .reduce((v: number, acc: number) => v + acc, 0) / 4;

  useEffect(() => {
    bifrostDrive.syncWithTopic();
  }, [bifrostDrive, bifrostTelemetry]);

  // Helper function for creating labels in the driveInfoCardBody
  const createLabelCell = (content: ReactNode) => (
    <div className="flex flex-col justify-end items-center">{content}</div>
  );

  // The top half of the card, containing drive mode info data
  const driveInfoCardBody = (
    <CardBody className="grid DriveWidgetTopGrid gap-y-2.5 gap-x-5">
      {createLabelCell(<>Average Velocity</>)}
      {createLabelCell(<>Speed Control</>)}

      <div>
        <OverlayedProgress
          size="lg"
          aria-label="Average Velocity"
          value={Math.abs(averageWheelAngularVelocity)}
          maxValue={DRIVE_VEL_MAX}
        >
          {averageWheelAngularVelocity.toFixed(2)} rad/s
        </OverlayedProgress>
      </div>

      <div>
        <OverlayedProgress
          size="lg"
          value={driveMultiplier}
          maxValue={1}
          aria-label="Speed Control"
        >
          {(driveMultiplier * 100).toFixed(0)} %
        </OverlayedProgress>
      </div>
    </CardBody>
  );

  // Finally, put the body into a card
  return (
    <Card {...props}>
      <CardHeader className="text-h1 pb-0">Drive Speed</CardHeader>
      {driveInfoCardBody}
    </Card>
  );
};

export default DriveSpeedWidget;

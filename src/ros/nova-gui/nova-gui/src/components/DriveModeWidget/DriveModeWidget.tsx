import {Card, CardHeader, CardBody, Kbd, CardProps} from "@nextui-org/react";
import './DriveWidget.css';
import { driveModes } from "./DriveModeDisplayData";
import { DriveModeButton } from "./DriveModeButton";
import {useBifrost} from "../../redux/actions/useBifrostAction.ts";
import {RosTopics} from "../../ros/rosTopics.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../redux/RootState.ts";
import {DriveMode} from "../../ros/rosMessageTypes.ts";
import {useEffect} from "react";

// Properties for the DriveModeWidget component.
export interface IDriveModeWidgetProps extends CardProps {

}

/**
 * A component that displays the current drive mode of the rover, and provides functionality to change drive mode
 * through the UI for redundancy.
 */
const DriveModeWidget: React.FC<IDriveModeWidgetProps> = (props: IDriveModeWidgetProps) => {
  const bifrost = useBifrost(RosTopics.DRIVE_INFO);
  const driveMode = useSelector((state: RootState) => state.driveStore.drive_mode);

  useEffect(() => {
    bifrost.syncWithRover();
  }, [bifrost]);

  // TODO: Replace with a service call
  const setDriveMode = (newDriveMode: DriveMode) => {
    console.log(`Set drive mode to ${newDriveMode}`);
  }

  // Finally, put the two card bodies into a card
  return (
    <Card {...props}>
      <CardHeader>Drive Mode</CardHeader>
      <CardBody className="flex justify-center flex-col content-center">
        <div>
          <div className="grid grid-flow-col gap-3 auto-cols-fr">
            {driveModes.map((mode, index) => (
              <DriveModeButton
                key={index}
                driveModeData={mode}
                driveModeActive={driveMode === mode.driveMode}
                onPress={() => setDriveMode(index as DriveMode)}
                iconClassName="w-5 h-5"
                tooltipPlacement="right"
                hideTooltip hideKeybind
                className="grow w-1/4 justify-start"
                keybindPlacement="top-left"
              >
                <span className="ml-0.5">{mode.shortName ?? mode.name}</span>
                <div className="grow"></div>
                <Kbd className="mx-1.5">{mode.keybind}</Kbd>

              </DriveModeButton>
            ))}
          </div>
        </div>
      </CardBody>
    </Card>
  )
};

export default DriveModeWidget;
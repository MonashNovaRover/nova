import { Card, CardHeader, CardBody, Kbd, CardProps } from "@nextui-org/react";
import "./DriveWidget.css";
import { DriveMode, driveModes } from "./DriveModeDisplayData.tsx";
import { DriveModeButton } from "./DriveModeButton.tsx";
import { useBifrost } from "../../../redux/actions/bifrost/useBifrostAction.ts";
import { useSelector } from "react-redux";
import { RootState } from "../../../redux/RootState.ts";
import { useEffect } from "react";
import { Info, Lock } from "react-feather";
import { RosTopic } from "../../../ros/topics/rosTopic.ts";

// Properties for the DriveModeWidget component.
export interface IDriveModeWidgetProps extends CardProps {}

/**
 * A component that displays the current drive mode of the rover, and provides functionality to change drive mode
 * through the UI for redundancy.
 */
const DriveModeWidget: React.FC<IDriveModeWidgetProps> = (
  props: IDriveModeWidgetProps
) => {
  const bifrost = useBifrost({ topic: RosTopic.DRIVE_INFO });
  const driveMode = useSelector(
    (state: RootState) => state.driveStore.drive_mode
  );

  const isLocked = useSelector((state: RootState) => state.driveStore.locked);
  const isConnected = useSelector(
    (state: RootState) => state.driveStore.connected
  );

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  // TODO: Replace with a service call
  const setDriveMode = (newDriveMode: DriveMode) => {
    console.log(`Set drive mode to ${newDriveMode}`);
  };

  // Message to show when disconnected
  const disconnectedMessage = (
    <div className="flex flex-row justify-center gap-2">
      <Info /> <span>Disconnected</span>
    </div>
  );

  // Message to show when locked
  const lockedMessage = (
    <div className="flex flex-row justify-center gap-2">
      <Lock /> <span>Controller is locked</span>
    </div>
  );

  // Blur put over buttons when disconnected or locked
  const blurOverlay = (
    <div className="DriveModeWidgetOverlay flex flex-col justify-center content-center backdrop-blur-[1px] wrap-none" />
  );

  // Finally, put everything into a card
  return (
    <Card {...props}>
      <CardHeader className="pb-0 flex flex-row content-center gap-5">
        <span>Drive Mode</span>
        <div className="grow" />
        {!isConnected ? disconnectedMessage : isLocked ? lockedMessage : <></>}
      </CardHeader>
      <CardBody className="flex justify-center flex-col content-center">
        <div>
          <div className="grid grid-flow-row grid-cols-[repeat(auto-fit,_minmax(10em,_1fr))] gap-3">
            {driveModes.map((mode, index) => (
              <DriveModeButton
                key={index}
                driveModeData={mode}
                driveModeActive={driveMode === mode.driveMode}
                onPress={() => setDriveMode(index as DriveMode)}
                iconClassName="w-5 h-5"
                tooltipPlacement="right"
                hideTooltip
                hideKeybind
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
        {!isConnected || isLocked ? blurOverlay : <></>}
      </CardBody>
    </Card>
  );
};

export default DriveModeWidget;

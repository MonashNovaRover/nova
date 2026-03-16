import {Button, Card, CardBody, CardHeader, CardProps} from "@nextui-org/react";
import React, {useEffect, useState} from "react";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import {RosService} from "../../../ros/services/rosService.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../../redux/RootState.ts";
import {Dot} from "react-bootstrap-icons";
import {IRosRclInterfacesSetParametersResponse} from "../../../ros/rosTypes.ts";
import toast from "react-hot-toast";

export interface IDriveControlWidgetProps extends CardProps {}

/**
 * A component that displays various drive control statuses (e.g. locked, handbrake, etc.)
 * originating from teleop drive. Also has functionality to toggle drive Autolock as well.
 */

const DriveControlWidget: React.FC<IDriveControlWidgetProps> = (
  props: IDriveControlWidgetProps) => {

  const bifrost = useBifrost({
    topic: RosTopic.DRIVE_INFO, service: RosService.TELEOP_DRIVE_SET_PARAMS });
  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const driveInfo = useSelector(
    (state: RootState) => state.driveStore);

  const lockedReasons: Record<string, string> = {
    "1": "Initial State",
    "2": "Lock Button Pressed",
    "3": "Excessive BLCMD Errors",
    "4": "Gamepad Disconnected",
  };
  const suppressLockedReasonToast = [1, 2];

  const [autolockOverride, setAutolockOverride] = useState(false);
  const currentLockedReason = lockedReasons[driveInfo.locked_reason.toString()] ?? "Reason Unknown";

  // show toast when locked unexpectedly
  useEffect(() => {
    if (!suppressLockedReasonToast.includes(driveInfo.locked_reason) && driveInfo.locked) {
      toast.error(`Drive Locked due to: ${currentLockedReason}`);
    }
  }, [driveInfo.locked_reason, driveInfo.locked]);

  // callback to toggle autolock override in teleop drive
  const onAutolockButtonPress = () => {

    const newOverrideState = !autolockOverride;

    const toastId = toast.loading(newOverrideState ? "Overriding Autolock..." : "Resuming Autolock...", {
      duration: 10000,
    });

    bifrost.callService(
      {
        parameters: [
          {
            name: "autolock_override",
            value: {
              type: 1,
              bool_value: newOverrideState,
            }
          }
        ]
      },
      {
        sendToRedux: true,
        responseToast: false,
        handleResponse: (response) => {
          const setParamsResponse = response as IRosRclInterfacesSetParametersResponse;

          if (setParamsResponse.results.length != 1) {
            toast.error(`Expected one result but got ${setParamsResponse.results.length} instead`, {
              id: toastId,
              duration: 4000,
            })
            return;
          }

          const result = setParamsResponse.results[0];

          if (result.successful) {
            toast.success(newOverrideState ? "Autolock Overridden" : "Autolock Resumed", {
              id: toastId,
              duration: 2000,
            })
            setAutolockOverride(newOverrideState);
          }
          else {
            toast.error(`Failed to toggle Autolock due to: ${result.reason}`, {
              id: toastId,
              duration: 4000,
            })
            return;
          }
        }
      }
    );
  }

  const lockedMessage = (
    <div className="flex flex-row items-center gap-1 text-base font-bold">
      Drive Locked
      <Dot className="text-xl"/>
      {currentLockedReason}
    </div>
  );

  const unlockedMessage = (
    <div className="flex flex-row items-center text-xl font-semibold">
      Drive Unlocked
    </div>
  );

  const overrideDeactivatedMessage = (
    <div>
      Override Autolock
    </div>
  );

  const overrideActivatedMessage = (
    <div>
      Resume Autolock
    </div>
  );

  return (
    <Card {...props}>
      <CardHeader className="pb-0">Drive Controls</CardHeader>
      <CardBody className="grid grid-flow-row gap-4 grid-cols-2">

        {/*display locked status of drive*/}
        <Button className="col-span-full opacity-100" isDisabled variant="shadow" radius="full" size="sm" color={driveInfo.locked ? "danger" : "success"} >
          {driveInfo.locked ? lockedMessage : unlockedMessage}
        </Button>

        {/*allow toggling of autolock override*/}
        <Button className="text-sm font-semibold" radius="sm" size="sm" color={autolockOverride ? "warning" : "primary"} variant={autolockOverride ? "solid" : "ghost"} onPress={onAutolockButtonPress}>
          {autolockOverride ? overrideActivatedMessage : overrideDeactivatedMessage}
        </Button>

        {/*display connection status (of game pad)*/}
        <Button className="opacity-100 text-sm font-semibold" isDisabled radius="full" size="sm" color={driveInfo.connected ? "default" : "danger"} variant={driveInfo.connected ? "bordered" : "solid"}>
          {driveInfo.connected ? "Connected" : "Disconnected"}
        </Button>

      </CardBody>
    </Card>
  )

}

export default DriveControlWidget;
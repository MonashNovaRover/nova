import { Card, CardHeader, CardBody, CardProps } from "@nextui-org/react";
import { useBifrost } from "../../../../redux/actions/bifrost/useBifrostAction.ts";
import {useEffect, useState} from "react";
import { RosTopic } from "../../../../ros/topics/rosTopic.ts";
import {ActivatedNodeConfig} from "./ActivatedNodeWidgetConfig.tsx";
import {ActivatedNodeButton} from "./ActivatedNodeButton.tsx";
import {useSelector} from "react-redux";
import {RootState} from "../../../../redux/RootState.ts";
import {Lock} from "react-feather";

// Properties for the URCActivatedNodeWidget component.
interface ActivatedNodeWidgetProps extends CardProps {
  config: ActivatedNodeConfig[]
}

/**
 * A component that displays whether activated nodes are active or inactive
 */
const ActivatedNodeWidget: React.FC<ActivatedNodeWidgetProps> = (
  props: ActivatedNodeWidgetProps
) => {
  const activeStatusBifrost = useBifrost({ topic: RosTopic.ACTIVATED_NODES });
  const lockedStatusBifrost = useBifrost({ topic: RosTopic.LOCKED_STATUS });

  const activeStatusMessage = useSelector((state: RootState) => state.activeStatusStore);
  const lockedStatusMessage = useSelector((state: RootState) => state.lockedStatusStore);

  const [currentStatus, setCurrentStatus] = useState(props.config.map(_ => false))

  useEffect(() => {
    activeStatusBifrost.syncWithTopic();
    lockedStatusBifrost.syncWithTopic();
  }, [activeStatusBifrost, lockedStatusBifrost]);

  // update currentStatus with every new message
  useEffect(() => {
    props.config.forEach((value, index) => {
      if (value.name === activeStatusMessage.name) {
        setCurrentStatus(currentStatus.map((v, i) => i === index ? activeStatusMessage.active : v))
      }
    })
  }, [activeStatusMessage, currentStatus, setCurrentStatus, props.config]);

  // Message to show when locked
  const lockedMessage = (
    <div className="flex flex-row justify-center gap-2">
      <Lock /> <span>Locked</span>
    </div>
  );

  // Blur put over buttons when disconnected or locked
  const blurOverlay = (
    <div className="absolute inset-0 flex flex-col justify-center items-center backdrop-blur-[1px] z-10" />
  );

  return (
    <Card {...props}>
      <CardHeader className="pb-0 flex flex-row content-center gap-5">
        <span>Active Controllers</span>
        <div className="grow" />
        {lockedStatusMessage.locked ? lockedMessage : <></>}
      </CardHeader>
      <CardBody>
        <div className="grid grid-cols-2 gap-3">
          {props.config.map((data, i) => (
            <ActivatedNodeButton
              text={data.displayName}
              icon={data.icon}
              isSelected={currentStatus[i]}
            />
          ))}
        </div>
        {lockedStatusMessage.locked ? blurOverlay : <></>}
      </CardBody>
    </Card>
  );
};

export default ActivatedNodeWidget;

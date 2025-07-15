import { Card, CardHeader, CardBody, CardProps } from "@nextui-org/react";
import { useBifrost } from "../../../../redux/actions/bifrost/useBifrostAction.ts";
import {useEffect, useState} from "react";
import { RosTopic } from "../../../../ros/topics/rosTopic.ts";
import {ActivatedNodeConfig} from "./ActivatedNodeWidgetConfig.tsx";
import {ActivatedNodeButton} from "./ActivatedNodeButton.tsx";
import {useSelector} from "react-redux";
import {RootState} from "../../../../redux/RootState.ts";

// Properties for the URCActivatedNodeWidget component.
export interface URCActivatedNodeWidgetProps extends CardProps {
  config: ActivatedNodeConfig[]
}

/**
 * A component that displays whether activated nodes are active or inactive
 */
const ActivatedNodeWidget: React.FC<URCActivatedNodeWidgetProps> = (
  props: URCActivatedNodeWidgetProps
) => {
  const bifrost = useBifrost({ topic: RosTopic.ACTIVATED_NODES });
  const lastMessage = useSelector(
    (state: RootState) => state.activeStatusStore
  );

  const [currentStatus, setCurrentStatus] = useState(props.config.map(_ => false))
  const [currentLockedStatus, setCurrentLockedStatus] = useState(props.config.map(_ => true))

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  // update currentStatus with every new message
  useEffect(() => {
    props.config.map((value, index) => value.name === lastMessage.name ? setCurrentStatus(
      currentStatus.map((v, i) => i === index ? lastMessage.active : v)
    ) : null)
  }, [lastMessage, setCurrentStatus, props.config]);

  // update currentLockedStatus with every new message
  useEffect(() => {
    props.config.map((value, index) => value.name === lastMessage.name ? setCurrentLockedStatus(
      currentLockedStatus.map((v, i) => i === index ? lastMessage.locked : v)
    ) : null)
  }, [lastMessage, setCurrentLockedStatus, props.config]);

  return (
    <Card {...props}>
      <CardHeader className="pb-0 flex flex-row content-center gap-5">
        <span>Active Controllers</span>
      </CardHeader>
      <CardBody>
        <div className="grid grid-cols-2 gap-3">
          {props.config.map((data, i) => (
            <ActivatedNodeButton
              text={data.displayName}
              icon={data.icon}
              isSelected={currentStatus[i]}
              isLocked={currentLockedStatus[i]}
            />
          ))}
        </div>
      </CardBody>
    </Card>
  );
};

export default ActivatedNodeWidget;

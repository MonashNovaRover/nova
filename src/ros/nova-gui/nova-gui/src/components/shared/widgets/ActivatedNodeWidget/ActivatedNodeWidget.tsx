import { Card, CardHeader, CardBody, CardProps } from "@nextui-org/react";
import { useBifrost } from "../../../../redux/actions/bifrost/useBifrostAction.ts";
import {useEffect, useState} from "react";
import { RosTopic } from "../../../../ros/topics/rosTopic.ts";
import {ActivatedNodeConfig} from "./ActivatedNodeWidgetConfig.tsx";
import {ActivatedNodeButton} from "./ActivatedNodeButton.tsx";
import {useSelector} from "react-redux";
import {RootState} from "../../../../redux/RootState.ts";

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
  const lockedStatusBifrost = useBifrost({ topic: RosTopic.ACTIVATED_NODES });

  // TODO: Change to 2 different selectors (LOCKED STATUS STORE DOESNT EXIST YET!!)
  const activeStatusMessage = useSelector((state: RootState) => state.activeStatusStore);
  const lockedStatusMessage = useSelector((state: RootState) => state.lockedStatusStore);

  const [currentStatus, setCurrentStatus] = useState(props.config.map(_ => false))

  useEffect(() => {
    activeStatusBifrost.syncWithTopic();
    lockedStatusBifrost.syncWithTopic();
  }, [activeStatusBifrost, lockedStatusBifrost]);

  // update currentStatus with every new message
  useEffect(() => {
    props.config.map((value, index) => value.name === activeStatusMessage.name ? setCurrentStatus(
      currentStatus.map((v, i) => i === index ? activeStatusMessage.active : v)
    ) : null)
  }, [activeStatusMessage, setCurrentStatus, props.config]);

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
              isLocked={lockedStatusMessage.locked}
            />
          ))}
        </div>
      </CardBody>
    </Card>
  );
};

export default ActivatedNodeWidget;

import { Card, CardHeader, CardBody, CardProps } from "@nextui-org/react";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction.ts";
import {useEffect, useState} from "react";
import { RosTopic } from "../../ros/topics/rosTopic.ts";
import {ActivatedNodeConfig} from "./ActivatedNodeWidgetConfig.tsx";
import {ActivatedNodeButton} from "./ActivatedNodeButton.tsx";

// Properties for the URCActivatedNodeWidget component.
export interface URCActivatedNodeWidgetProps extends CardProps {
  config: ActivatedNodeConfig[]
}

/**
 * A component that displays whether activated nodes are active or inactive
 */
const URCActivatedNodeWidget: React.FC<URCActivatedNodeWidgetProps> = (
  props: URCActivatedNodeWidgetProps
) => {
  const bifrost = useBifrost({ topic: RosTopic.DRIVE_INFO });
  // const lastMessage = useSelector(
  //   (state: RootState) => state.driveStore.drive_mode
  // );

  const [currentStatus, _] = useState([false, true, true, false])

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

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
            />
          ))}
        </div>
      </CardBody>
    </Card>
  );
};

export default URCActivatedNodeWidget;

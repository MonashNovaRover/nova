import React, {useEffect} from "react";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {Card, CardBody, CardProps} from "@nextui-org/react";
import {RosService} from "../../../ros/services/rosService.ts";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../../redux/RootState.ts";
import EffortControl from "./EffortControl.tsx";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";
import { IRosScienceInterfacesEffortStatus } from "../../../ros/rosTypes.ts";

export interface EffortWidgetWidgetProps extends CardProps {
  label: string
  topic: RosTopic
  service: RosService
  statusSelector: (state: RootState) => IRosScienceInterfacesEffortStatus
  storeName: string
}

/**
 * Effort control widget (repurposed from HeaterWidget.tsx)
 * @param props
 * @constructor
 */
const EffortWidget: React.FC<EffortWidgetWidgetProps> = (props) => {
  const bifrost = useBifrost({topic: props.topic, service: props.service});
  const effortStatus = useSelector(props.statusSelector);
  const [currentEffort, setEffort] = useGenericStore<number>(props.storeName);

  const sendCommand = (state: boolean, effort: number) => bifrost.callService({state: state, level: effort/100});

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const updateEffort = (effort: number) => {
    setEffort(effort)
    sendCommand(effortStatus.state, effort)
  }

  const setEffortStatus = (state: boolean) => sendCommand(state, currentEffort)

  return <Card {...props}>
    <CardBody>
      <EffortControl
        controlName={props.label}
        currentStatus={effortStatus.state}
        setStatus={setEffortStatus}
        currentEffort={currentEffort}
        setEffort={updateEffort}
      />
    </CardBody>
  </Card>
}

export default EffortWidget
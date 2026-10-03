/*
 * SOLUTION for src/components/science/SweeperWidget/SweeperWidget.tsx.
 */
import React, {useEffect} from "react";
import {Card, CardBody, CardProps} from "@nextui-org/react";
import {useSelector} from "react-redux";

import SweeperControl from "./SweeperControl.tsx";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosService} from "../../../ros/services/rosService.ts";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import {RootState} from "../../../redux/RootState.ts";
import {useGenericStore} from "../../../hooks/useGenericStore.ts";

/**
 * Sweeper control widget. Sends EffortCommands to the sweeper node, and shows the
 * status the node publishes back.
 *
 * @param props card props, forwarded to the surrounding Card
 * @constructor
 */
const SweeperWidget: React.FC<CardProps> = (props) => {
  const bifrost = useBifrost({topic: RosTopic.SWEEPER_STATUS, service: RosService.SWEEPER_COMMAND});
  const sweeperStatus = useSelector((state: RootState) => state.sweeperStatus);
  const [currentEffort, setCurrentEffort] = useGenericStore<number>("sweeperEffort");

  // Subscribe to the status topic
  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  // EffortCommand.level is 0..1, the slider speaks percent
  const sendCommand = (state: boolean, effort: number) => bifrost.callService({state: state, level: effort / 100});

  const setStatus = (state: boolean) => sendCommand(state, currentEffort);

  const setEffort = (effort: number) => {
    setCurrentEffort(effort);
    sendCommand(sweeperStatus.state, effort);
  }

  return (
    <Card {...props}>
      <CardBody>
        <SweeperControl
          controlName="Sweeper"
          currentStatus={sweeperStatus.state}
          setStatus={setStatus}
          currentEffort={currentEffort}
          setEffort={setEffort}
        />
      </CardBody>
    </Card>
  );
}

export default SweeperWidget;

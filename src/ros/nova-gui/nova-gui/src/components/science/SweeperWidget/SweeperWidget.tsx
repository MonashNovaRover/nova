import React, {useState} from "react";
import {Card, CardBody, CardProps} from "@nextui-org/react";
import SweeperControl from "./SweeperControl.tsx";

/**
 * Sweeper control widget.
 *
 * @param props card props, forwarded to the surrounding Card
 * @constructor
 */
const SweeperWidget: React.FC<CardProps> = (props) => {
  // TODO 4.3.2: subscribe and call the service with useBifrost
  // TODO 4.3.3: this should come from state.sweeperStatus, not from React
  const [sweeperStatus, setSweeperStatus] = useState<boolean>(false);

  const setStatus = (state: boolean) => {
    // TODO 4.3.2: send an EffortCommand here
    setSweeperStatus(state);
  }

  return (
    <Card {...props}>
      <CardBody>
        <SweeperControl
          controlName="Sweeper"
          currentStatus={sweeperStatus}
          setStatus={setStatus}
        />
      </CardBody>
    </Card>
  );
}

export default SweeperWidget;

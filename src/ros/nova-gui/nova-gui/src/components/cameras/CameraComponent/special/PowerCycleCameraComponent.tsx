import {BaseCameraComponentProps} from "../CameraComponent.tsx";
import {FC} from "react";
import {Card, CardBody, CardHeader} from "@nextui-org/react";
import PowerCycle from "../../../science/PowerCycle/PowerCycle.tsx";

export const PowerCycleCameraComponent: FC<BaseCameraComponentProps> = (_: BaseCameraComponentProps) => {
  return (
    <Card className="self-start">
      <CardHeader>
        Power Cycle Voltage Rails
      </CardHeader>
      <CardBody className="pt-0">
        <PowerCycle/>
      </CardBody>
    </Card>
  )
}

export default PowerCycleCameraComponent;

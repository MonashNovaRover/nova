import {BaseCameraComponentProps} from "../CameraComponent.tsx";
import {FC} from "react";
import {Card, CardContent, CardHeader} from "@heroui/react";
import PowerCycle from "../../../science/PowerCycle/PowerCycle.tsx";

export const PowerCycleCameraComponent: FC<BaseCameraComponentProps> = (_: BaseCameraComponentProps) => {
  return (
    <Card className="self-start">
      <CardHeader>
        Power Cycle Science Voltage Rails
      </CardHeader>
      <CardContent className="pt-0">
        <PowerCycle/>
      </CardContent>
    </Card>
  )
}

export default PowerCycleCameraComponent;

import {Button, Card, CardBody, CardHeader, CardProps} from "@nextui-org/react";
import React from "react";
import CopyableOutput from "../../CopyableOutput/CopyableOutput.tsx";


interface INIRProbeValueWidgetProps extends CardProps {

}


const NIRProbeValueWidget: React.FC<INIRProbeValueWidgetProps> = ({...cardProps}) => {


  return (
    <Card {...cardProps}>
      <CardHeader className="pb-0">
        NIR Probe Output
      </CardHeader>
      <CardBody className="flex flex-col gap-3">
        <CopyableOutput className="tracking-wide" classNames={{pre: "text-lg pt-1"}}>
          1000
        </CopyableOutput>
        <Button color="primary">
          Save
        </Button>
      </CardBody>
    </Card>
  )
}

export default NIRProbeValueWidget;
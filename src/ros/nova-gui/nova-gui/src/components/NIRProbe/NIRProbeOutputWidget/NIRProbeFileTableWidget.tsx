import {
  Card,
  CardBody,
  CardHeader,
  CardProps,
} from "@nextui-org/react";
import React from "react";
import NIRProbeFileTable from "./NIRProbeFileTable.tsx";
import NIRProbeCalcTable from "./NIRProbeCalcTable.tsx";

export interface NIRProbeFileTableWidgetProps extends CardProps {
  showAdvanced : boolean,
}

const NIRProbeFileTableWidget: React.FC<NIRProbeFileTableWidgetProps> = ({
  showAdvanced, ...cardProps
}) => {

  return (
    <Card {...cardProps}>
      <CardHeader className="pb-0">
        NIR Probe File Table
      </CardHeader>
      <CardBody className="flex flex-col gap-3 p-3">
        <NIRProbeCalcTable/>
        <NIRProbeFileTable showAdvanced={showAdvanced}/>
      </CardBody>
    </Card>
  );
}

export default NIRProbeFileTableWidget;
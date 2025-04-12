import {
  Button,
  Card,
  CardBody,
  CardHeader,
  CardProps,
} from "@nextui-org/react";
import React, {ReactNode, useState} from "react";
import NIRProbeFileTable from "./NIRProbeFileTable.tsx";
import {MoreHorizontal} from "react-feather";
import NIRCalibrationSettingsModal from "./NIRCalibrationSettingsModal.tsx";

export interface NIRProbeFileTableWidgetProps extends CardProps {
  headerTable: ReactNode
}

/**
 * Widget containing the saved NIR Probe readings and the averages
 * @param headerTable the table to sit at the top of the widget
 * @param cardProps
 * @constructor
 */
const NIRProbeFileTableWidget: React.FC<NIRProbeFileTableWidgetProps> = ({
  headerTable, ...cardProps
}: NIRProbeFileTableWidgetProps) => {
  const [calibrationModalIsOpen, setCalibrationModalIsOpen] = useState<boolean>(false)

  return (
    <Card {...cardProps}>
      <CardHeader className="pb-0 flex flex-row justify-center">
        <div className="grow">NIR Probe File Table</div>
        <Button
          variant={"light"}
          isIconOnly
          onPress={() => setCalibrationModalIsOpen(true)}
        >
          <MoreHorizontal/>
        </Button>
      </CardHeader>
      <CardBody className="flex flex-col gap-3 p-3">
        {headerTable}
        <NIRProbeFileTable/>
      </CardBody>
      <NIRCalibrationSettingsModal
        isOpen={calibrationModalIsOpen}
        setIsOpen={setCalibrationModalIsOpen}
      />
    </Card>
  );
}

export default NIRProbeFileTableWidget;
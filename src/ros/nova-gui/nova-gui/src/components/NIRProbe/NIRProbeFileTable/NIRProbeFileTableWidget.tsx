import {
  Button,
  Card,
  CardBody,
  CardHeader,
  CardProps,
} from "@nextui-org/react";
import React, {useState} from "react";
import NIRProbeFileTable from "./NIRProbeFileTable.tsx";
import NIRProbeCalcTable from "./NIRProbeCalcTable.tsx";
import {MoreHorizontal} from "react-feather";
import NIRCalibrationSettingsModal from "./NIRCalibrationSettingsModal.tsx";

export interface NIRProbeFileTableWidgetProps extends CardProps {
}

const NIRProbeFileTableWidget: React.FC<NIRProbeFileTableWidgetProps> = ({
  ...cardProps
}) => {
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
        <NIRProbeCalcTable/>
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
import {Button, Card, CardBody, CardHeader, CardProps} from "@nextui-org/react";
import React, {useEffect} from "react";
import CopyableOutput from "../../CopyableOutput/CopyableOutput.tsx";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../../redux/RootState.ts";


interface INIRProbeValueWidgetProps extends CardProps {

}


const NIRProbeValueWidget: React.FC<INIRProbeValueWidgetProps> = ({...cardProps}) => {
  const bifrost = useBifrost({ topic: RosTopic.NIR_DATA });
  const nirData = useSelector((state: RootState) => state.nirStore.data);

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  return (
    <Card {...cardProps}>
      <CardHeader className="pb-0">
        NIR Probe Output
      </CardHeader>
      <CardBody className="flex flex-col gap-3">
        <CopyableOutput className="tracking-wide" classNames={{pre: "text-lg pt-1"}}>
          {nirData}
        </CopyableOutput>
        <Button color="primary">
          Save
        </Button>
      </CardBody>
    </Card>
  )
}

export default NIRProbeValueWidget;
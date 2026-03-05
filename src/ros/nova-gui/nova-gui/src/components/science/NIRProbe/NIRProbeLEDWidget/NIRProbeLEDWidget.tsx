import {Card, CardBody, CardHeader, CardProps} from "@nextui-org/react";
import {useEffect} from "react";
import {useBifrost} from "../../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../../../ros/topics/rosTopic.ts";
import {RosService} from "../../../../ros/services/rosService.ts";
import SpinnerButton from "../../../shared/components/buttons/SpinnerButton.tsx";
import {NIRProbeReadingTypeInfo} from "../SpaceResourcesSiteType.tsx";
import { useSelector } from "react-redux";
import { RootState } from "../../../../redux/RootState.ts";


interface INIRProbeLEDWidgetProps extends CardProps {
  readingInfo: NIRProbeReadingTypeInfo[], // list of NIRProbeReadingTypeInfo: [off, PD1, PD2]
}

/**
 * Widget containing buttons to call the ros2 service "/science/take_nir_probe_reading"
 * @param readingInfo display information about each photodiode, should be of the form [off, PD1, PD2]
 * @param cardProps
 * @constructor
 */
const NIRProbeLEDWidget: React.FC<INIRProbeLEDWidgetProps> = ({readingInfo, ...cardProps}) => {
  const bifrost = useBifrost({ topic: RosTopic.NIR_DATA, service: RosService.TAKE_NIR_PROBE_READING });
  const taking_reading = useSelector((state: RootState) => state.nirStore.status);
  const takeReading = () => bifrost.callService({});


  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  return (
    <Card {...cardProps}>
      <CardHeader className="pb-0">
        NIR Probe LED
      </CardHeader>
      <CardBody className="">
        <SpinnerButton
          onPressStart={() => takeReading()}
          isLoading = {taking_reading}
        >
          Request LED Readings
        </SpinnerButton>
      </CardBody>
    </Card>
  );
}

export default NIRProbeLEDWidget;
import {Card, CardBody, CardHeader, CardProps} from "@nextui-org/react";
import {useEffect} from "react";
import SegmentedPicker from "../../SegmentedPicker/SegmentedPicker.tsx";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../../redux/RootState.ts";
import {RosService} from "../../../ros/services/rosService.ts";

interface INIRProbeLEDWidgetProps extends CardProps {
}

const NIRProbeLEDWidget: React.FC<INIRProbeLEDWidgetProps> = ({...cardProps}) => {
  const bifrost = useBifrost({ topic: RosTopic.NIR_DATA, service: RosService.SET_NIR_PROBE_LED });
  const led = useSelector((state: RootState) => state.nirStore.led);
  const setLed = (newLed: number) => bifrost.callService({led: newLed}, {});

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const picker = (
    <SegmentedPicker
      selectedIndex={led}
      onIndexChange={setLed}
      fullWidth
      color={led > 0 ? "primary" : "default"}
    >
      <>Off</>
      <>LED 1 On</>
      <>LED 2 On</>
    </SegmentedPicker>
  );

  return (
    <Card {...cardProps}>
      <CardHeader className="pb-0">
        NIR Probe LED
      </CardHeader>
      <CardBody>
        {picker}
      </CardBody>
    </Card>
  );
}

export default NIRProbeLEDWidget;
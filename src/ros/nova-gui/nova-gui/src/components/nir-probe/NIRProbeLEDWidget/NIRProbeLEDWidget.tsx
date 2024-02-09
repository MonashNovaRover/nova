import {Card, CardBody, CardHeader, CardProps} from "@nextui-org/react";
import {useEffect} from "react";
import SegmentedPicker from "../../SegmentedPicker/SegmentedPicker.tsx";
import {useBifrost} from "../../../redux/actions/bifrost/useBifrostAction.ts";
import {RosTopic} from "../../../ros/topics/rosTopic.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../../redux/RootState.ts";
import {IRosCoreNirDataConst} from "../../../ros/rosTypes.ts";

interface INIRProbeLEDWidgetProps extends CardProps {

}

const NIRProbeLEDWidget: React.FC<INIRProbeLEDWidgetProps> = ({...cardProps}) => {
  const bifrost = useBifrost({ topic: RosTopic.NIR_DATA });
  const led = useSelector((state: RootState) => state.nirStore.led);

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  // TODO: Replace with a service call
  const setLed = (led: IRosCoreNirDataConst) => {
    console.log(`Set led to ${led}`);
  }

  const picker = (
    <SegmentedPicker
      selectedIndex={led}
      onIndexChange={v => {
        setLed(v);
      }}
      fullWidth
      color={led > 0 ? "primary" : "default"}
    >
      <>Off</>
      <>Water</>
      <>Ilmenite</>
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
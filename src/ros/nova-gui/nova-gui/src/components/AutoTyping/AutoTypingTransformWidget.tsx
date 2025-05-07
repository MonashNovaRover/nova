import {Button, Card, CardBody, CardProps} from "@nextui-org/react";
import {useBifrost} from "../../redux/actions/bifrost/useBifrostAction.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../redux/RootState.ts";
import {RosService} from "../../ros/services/rosService.ts";
import { Square, Power } from "react-feather";
import {AlignTransformValues} from "./AutoTypingConstants.tsx";

interface IAutoTypingTransformWidgetProps extends CardProps {
}

/**
 * Widget controlling auto typing transforms on service "/arm/keyboard/transform_toggle"
 * @param cardProps
 * @constructor
 */
const AutoTypingTransformWidget: React.FC<IAutoTypingTransformWidgetProps> = () => {
  const keyboardTransform = useSelector(
      (state: RootState) => state.keyboardTFTrigger
    );

  const bifrost = useBifrost({ service: RosService.KEYBOARD_TF_TOGGLE });
  const toggleKeyboardState = () => bifrost.callServiceToRedux({value:keyboardTransform.success ? AlignTransformValues.STOP : AlignTransformValues.START});

  return (
    <div className="flex flex-row justify-between gap-5">
      <Card className={`w-2/3 ${keyboardTransform.success ? "bg-success" : "bg-danger"}`}>
        <CardBody className="pl-5 pr-5 text-center">
          {keyboardTransform.success ? "Keyboard Aligned" : "Keyboard Unaligned"}
        </CardBody>
      </Card>
      <Button className="w-1/3 text-h1 h-12" color="primary" 
        onPress={toggleKeyboardState}>
        {keyboardTransform.success ? "STOP TRANSFORM" : "START TRANSFORM"}
        {keyboardTransform.success ? <Square size="15" fill="white"/> : <Power size="15"/>}
      </Button>
    </div>
  );
}

export default AutoTypingTransformWidget;

    
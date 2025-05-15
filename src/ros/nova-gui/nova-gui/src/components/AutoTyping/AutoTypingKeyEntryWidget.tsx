import {Button, Input, CardProps} from "@nextui-org/react";
import {useBifrost} from "../../redux/actions/bifrost/useBifrostAction.ts";
import {useSelector} from "react-redux";
import {RootState} from "../../redux/RootState.ts";
import {RosService} from "../../ros/services/rosService.ts";
import { Square, Power } from "react-feather";
import {AlignTransformValues} from "./AutoTypingConstants.tsx";

interface IAutoTypingKeyEntryWidgetProps extends CardProps {
}

/**
 * Widget controlling auto typing transforms on service "/arm/keyboard/transform_toggle"
 * @param cardProps
 * @constructor
 */
const AutoTypingKeyEntryWidget: React.FC<IAutoTypingKeyEntryWidgetProps> = () => {
  const keyboardTransform = useSelector(
      (state: RootState) => state.keyboardTFTrigger
  );

  const bifrost = useBifrost({ service: RosService.KEYBOARD_TF_TOGGLE });
  const toggleKeyboardState = () => bifrost.callServiceToRedux({value:keyboardTransform.success ? AlignTransformValues.STOP : AlignTransformValues.START});

  return (
    <div className="flex flex-row justify-between gap-5">
      <Button className="w-1/3 text-h1 h-12" color="primary" 
        onPress={toggleKeyboardState}> {/** Change on press to start/stop auto typing via ros */}
        {false ? "STOP TYPING" : "START TYPING"}
        {false ? <Square size="15" fill="white"/> : <Power size="15"/>}
      </Button>
      <Input size="lg" placeholder="What do you want to type?" className="flex-grow" onValueChange={()=>{}}></Input>
    </div>
  );
}

export default AutoTypingKeyEntryWidget;

    
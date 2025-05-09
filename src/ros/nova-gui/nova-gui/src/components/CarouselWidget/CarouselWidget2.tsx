/**
 * Carousel GUI component
 * Author: Bailey Chessum
 * Date Created: 12/5/2024
 */
import React, {useEffect, useState} from "react";
import {
  Button,
  Card,
  CardBody,
  CardHeader,
  CardProps, Input,
  Switch
} from "@nextui-org/react";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { RosService } from "../../ros/services/rosService";

/**
 * Props for CarouselWidget
 */
export interface CarouselWidgetProps extends CardProps {
}

/**
 * Widget that holds controls for moving the carousel to different positions.
 * It assumes that cuvette names are in the form of some index starting from 0 to some N
 * @constructor
 */
const CarouselWidget2: React.FC<CarouselWidgetProps> = (props) => {
  const bifrost = useBifrost({ service: RosService.KILN_COMMAND });
  const callService = (state: boolean, step: number) => bifrost.callService({state: state, target: step});

  useEffect(() => {
    bifrost.syncWithTopic();
  }, [bifrost]);

  const [step, setStep] = useState(0)
  const [active, setActive] = useState(false)

  const onClick = () => {
    callService(active, step)
  }

  // Construct the widget
  return (
    <Card {...props}>
      <CardHeader className="pb-0 flex flex-row">
        <div className="flex-grow">Carousel</div>
      </CardHeader>
      <CardBody>
        <div className="flex flex-row gap-4">
          <span>Active:</span>
          <Switch isSelected={active} onValueChange={setActive}/>
          <Input value={step.toString()} type="number" step={1} onValueChange={(v) => setStep(Number(v))} label="Steps"/>
        </div>
        <Button className="mt-4" onClick={onClick}>
          Submit
        </Button>
      </CardBody>
    </Card>
  )
}

export default CarouselWidget2;



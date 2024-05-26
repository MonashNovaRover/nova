/**
 * Carousel GUI component
 * Author: Bailey Chessum
 * Date Created: 12/5/2024
 */
import React, {Key, useEffect, useState} from "react";
import {
  Button,
  Card,
  CardBody,
  CardHeader, 
  CardProps,
  Select,
  SelectItem
} from "@nextui-org/react";
import { RosAction } from "../../ros/actions/RosAction";
import StepperWidget from "../PlatformWidget/StepperWidget";

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
const CarouselWidget: React.FC<CarouselWidgetProps> = (props) => {
  const [ selectedCuvette, setSelectedCuvette ] = useState<Key>();
  const [ selectedCuvetteIndex, setSelectedCuvetteIndex ] = useState<number>(0);
  const [ disableSelector, setDisableSelector ] = useState<boolean>(true);
  const [ offset, setOffset ] = useState<number>();

  // What values should be sent to the ROS node as positions to go to?
  const instruments : { [key: string] : number} ={
    "Camera": 1,
    "UV Vis Spec": 6,
  };

  const cuvetteCount = 20;
  const cuvettes = Array.from({ length: cuvetteCount }, (_, i) => {
    return {
      display: String(i + 1),
      value: String(i + 1),
    }});

  const changeCuvette = (e: React.ChangeEvent<HTMLSelectElement>) => {
    console.log(e.target.value);
    e.target.value ? setSelectedCuvette(e.target.value) : setSelectedCuvette("1");
  }

  
  // Picker for the cuvette
  const cuvettePicker = (
    <Select 
      label="Cuvette" 
      placeholder="Select a Cuvette" 
      className="col-span-2" 
      isDisabled={disableSelector}
      onChange={changeCuvette}>
      {cuvettes.map((cuvette) => (
        <SelectItem 
          key={cuvette.value} >
          {cuvette.display}
        </SelectItem>
      ))}
    </Select>
  );

  const changeInstrument = (e: React.ChangeEvent<HTMLSelectElement>) => {
    console.log(e.target.value);
    e.target.value ? setOffset(Number(e.target.value)) : setOffset(1);
  }

  // Picker for where to move the selected cuvette to
  const instrumentPicker = (
    <Select 
      label="Instrument" 
      placeholder="Select an instrument" 
      className="col-span-2" 
      isDisabled={disableSelector}
      onChange={changeInstrument}>
      {Object.keys(instruments).map((instrument) => (
        <SelectItem 
          key={instruments[instrument]} 
          endContent={
            <span className="text-foreground-500">({instruments[instrument]})</span>
          }>
          {instrument}
        </SelectItem>
      ))}
    </Select>
  );


  useEffect(() => {
    if (!selectedCuvette || !offset) {
      return;
    }
    if (Number(selectedCuvette) >= 0 && Number(selectedCuvette) <= cuvetteCount) {
      setSelectedCuvetteIndex((Number(selectedCuvette)-1 + (offset-1)) % cuvetteCount);
    }
    console.log(`SelectedCuvette ${selectedCuvette}, SelectedCuvetteIndex`)
  }, [selectedCuvette, offset]);

  // Construct the widget
  return (
    <Card {...props}>
      <CardHeader className="pb-0 flex flex-row">
        <div className="flex-grow">Carousel</div>
      </CardHeader>
      <CardBody>
   
        <StepperWidget 
          rosActionType={RosAction.CAROUSEL_ACTION}
          locations={cuvettes}
          targetLocationIndex={selectedCuvetteIndex}
          setTargetLocationIndex={setSelectedCuvetteIndex}
          setDisableSelector={setDisableSelector}
          canSet>
          {cuvettePicker}
          {instrumentPicker}
        </StepperWidget>
      </CardBody>

    </Card>
  )
}

export default CarouselWidget;



import React, {useState} from "react";
import {Button, Card, CardBody, CardHeader, CardProps, Slider} from "@nextui-org/react";
import CarouselDial from "./CarouselDial.tsx";
import {isArray} from "lodash";

export interface CarouselWidgetProps extends CardProps{
}

const CarouselWidgetV2: React.FC<CarouselWidgetProps> = (props) => {
  const [rotation, setRotation] = useState(0)

  return <Card {...props}>
    <CardHeader>Carousel</CardHeader>
    <CardBody className="gap-3">
      <span>{rotation}</span>
      <Slider className="mb-3" maxValue={20} minValue={1} value={rotation}
              onChange={(v) => isArray(v) ? setRotation(v[0]) : setRotation(v)}/>
      <CarouselDial step={rotation}/>
      <Button onClick={() => setRotation(rotation-1)}>-1</Button>
      <Button onClick={() => setRotation(rotation+1)}>+1</Button>
    </CardBody>
  </Card>
}

export default CarouselWidgetV2
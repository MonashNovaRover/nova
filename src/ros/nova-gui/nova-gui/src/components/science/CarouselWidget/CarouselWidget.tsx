import React from "react";
import {Card, CardBody, CardHeader, CardProps} from "@nextui-org/react";
import CarouselDial from "./CarouselDial.tsx";

export interface CarouselWidgetProps extends CardProps{
}

const CarouselWidgetV2: React.FC<CarouselWidgetProps> = (props) => {


  return <Card {...props}>
    <CardHeader>Carousel</CardHeader>
    <CardBody>
      <CarouselDial rotation={0}/>
    </CardBody>
  </Card>
}

export default CarouselWidgetV2
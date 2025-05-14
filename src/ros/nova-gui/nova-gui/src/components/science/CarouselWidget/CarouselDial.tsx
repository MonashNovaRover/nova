import React from "react";
import {Image} from "@nextui-org/react";
import CarouselDialImage from "../../../assets/carousel-dial.png"
import {ChevronUp, Search} from "react-feather";

export interface CarouselDialProps {
  step: number
}

// the image has the line to the left of 1 pointing straight down
// there is an 18 degree difference for each cuvette
const offset = 9
const stepSize = 18

/**
 * A representation of the Carousel that spins so that the current step is at the bottom
 * @param props
 * @param step the current cuvette to display at the bottom
 * @constructor
 */
const CarouselDial: React.FC<CarouselDialProps> = (props) => {
  const rotateStyle: React.CSSProperties = {
    transform: `rotate(${props.step * stepSize - offset}deg)`,
    transition: 'transform 0.3s ease-in-out',
  };

  return <div>
    <div className="flex flex-col items-center overflow-hidden">
      <div className="flex flex-row items-center">
        <Search className="w-16 h-8 opacity-0"/>
        <div>
          <Image style={rotateStyle} src={CarouselDialImage}/>
        </div>
        <Search color="#17c964" className="w-16 h-8"/>
      </div>
      <ChevronUp color="#17c964" className="w-16 h-8"/>
    </div>
  </div>
}

export default CarouselDial
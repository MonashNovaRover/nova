import React, {useState} from "react";
import {Image, Slider} from "@nextui-org/react";
import CarouselDialImage from "../../../assets/carousel-dial.png"
import {isArray} from "lodash";

export interface CarouselDialProps {
  rotation: number
}

const offset = 9

const CarouselDial: React.FC<CarouselDialProps> = (props) => {
  const [rotation, setRotation] = useState(0)

  const rotateStyle: React.CSSProperties = {
    transform: `rotate(${rotation + offset}deg)`,
    transition: 'transform 0.3s ease-in-out',
  };

  return <div>
    <span>{rotation}</span>
    <Slider className="mb-3" maxValue={360} value={rotation}
            onChange={(v) => isArray(v) ? setRotation(v[0]) : setRotation(v)}/>
    <div className="p-5">
      <Image className="" style={rotateStyle} src={CarouselDialImage}/>
    </div>
  </div>
}

export default CarouselDial
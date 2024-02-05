import {Card, CardBody, CardHeader, CardProps} from "@nextui-org/react";
import {useState} from "react";
import SegmentedPicker from "../../SegmentedPicker/SegmentedPicker.tsx";

interface INIRProbeLEDWidgetProps extends CardProps {

}

const NIRProbeLEDWidget: React.FC<INIRProbeLEDWidgetProps> = ({...cardProps}) => {

  const [ledIndex, setLedIndex] = useState(0);

  return (
    <Card {...cardProps}>
      <CardHeader className="pb-0">
        NIR Probe LED
      </CardHeader>
      <CardBody>
        <SegmentedPicker
          selectedIndex={ledIndex}
          onIndexChange={v => {
            setLedIndex(v);
            console.log(`index changed to ${v}`);
          }}
          fullWidth
          color={ledIndex > 0 ? "primary" : "default"}
        >
          <span>Off</span>
          <span>Water</span>
          <span>Ilmenite</span>
        </SegmentedPicker>
      </CardBody>
    </Card>
  )
}

export default NIRProbeLEDWidget;
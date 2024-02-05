import {Card, CardBody, CardHeader, CardProps} from "@nextui-org/react";
import {useState} from "react";
import SegmentedPicker from "../../SegmentedPicker/SegmentedPicker.tsx";

interface INIRProbeLEDWidgetProps extends CardProps {

}

const NIRProbeLEDWidget: React.FC<INIRProbeLEDWidgetProps> = ({...cardProps}) => {

  const [ledIndex, setLedIndex] = useState(0);

  const tabs = [
    <span>Off</span>,
    <span>Water</span>,
    <span>Ilmenite</span>
  ];

  return (
    <Card {...cardProps}>
      <CardHeader className="pb-0">
        NIR Probe
      </CardHeader>
      <CardBody>
        <SegmentedPicker
          selectedIndex={ledIndex}
          onIndexChange={setLedIndex}
          fullWidth
          color={ledIndex > 0 ? "primary" : "default"}
        >
          {tabs}
        </SegmentedPicker>
      </CardBody>
    </Card>
  )
}

export default NIRProbeLEDWidget;
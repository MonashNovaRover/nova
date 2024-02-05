import {Card, CardBody, CardHeader, CardProps, Tab, Tabs} from "@nextui-org/react";
import {useState} from "react";
import SegmentedPicker from "../../SegmentedPicker/SegmentedPicker.tsx";


interface INIRProbeValueWidgetProps extends CardProps {

}


const NIRProbeValueWidget: React.FC<INIRProbeValueWidgetProps> = ({...cardProps}) => {

  const [ledIndex, setLedIndex] = useState(0);

  const tabs = [
    <span>Off</span>,
    <span>Water</span>,
    <span>Ilmenite</span>
  ]

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

export default NIRProbeValueWidget;
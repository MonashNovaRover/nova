import {Button, Card, CardBody, CardHeader, Switch} from "@nextui-org/react";
import {useState} from "react";
import SegmentedPicker from "../SegmentedPicker/SegmentedPicker.tsx";


export interface PlatformWidgetProps {

}


const PlatformWidget: React.FC<PlatformWidgetProps> = () => {

  const [targetLocationIndex, setTargetLocationIndex] = useState<number>(0);

  const locationNames = [
    "Cleaning sheath",
    "Sample Tray 2",
    "Sample Tray 1",
    "Retract Fully",
  ];

  const pickerRow = (
    <div className="mt-3">
      <div className="font-bold">Select catcher to extend to</div>
      <div className="flex flex-row mt-1.5 gap-3">
        <SegmentedPicker onIndexChange={setTargetLocationIndex} selectedIndex={targetLocationIndex}>
          {locationNames.map((name, index) => (
            <div key={index}>{name}</div>
          ))}
        </SegmentedPicker>
        <Button color="primary">
          Confirm
        </Button>
      </div>
    </div>
  );

  const ethanolPumpRow = (
    <div className="my-3">
      <div className="font-bold">Ethanol Pump</div>
      <Switch className="mt-1.5 mx-1.5"></Switch>
    </div>
  );

  return (
    <Card>
      <CardHeader className="pb-0">
      Platform
      </CardHeader>
      <CardBody>
        <div className="grid grid-cols-[auto_1fr]">
          <div className="font-bold pr-3">Current Location:</div>
          <div className="">Sample Tray 2</div>
          <div className="font-bold pr-3">Current Location:</div>
          <div className="">{locationNames[targetLocationIndex]}</div>
        </div>
        {pickerRow}
        {ethanolPumpRow}
      </CardBody>
    </Card>
  )

}

export default PlatformWidget;




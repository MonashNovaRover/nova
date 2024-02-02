import {Card, CardBody, CardHeader, CardProps, Tab, Tabs} from "@nextui-org/react";
import {useState} from "react";


interface INIRProbeLEDWidgetProps extends CardProps {

}


const NIRProbeLEDWidget: React.FC<INIRProbeLEDWidgetProps> = ({...cardProps}) => {

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
        <Tabs aria-label="LED Select" fullWidth={true}
              selectedKey={`${ledIndex}`}
              onSelectionChange={(key) => setLedIndex(+key)}
              color={ledIndex > 0 ? "primary" : "default"}>
          {tabs.map((tab, index) =>
            <Tab key={`${index}`} title={tab}/>
          )}
        </Tabs>
      </CardBody>
    </Card>
  )
}

export default NIRProbeLEDWidget;